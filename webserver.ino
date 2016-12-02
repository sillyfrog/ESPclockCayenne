

#include <ESP8266WebServer.h>
#include <Time.h>
#include <debug.h>

ESP8266WebServer webserver (80);

File fsUploadFile;

void setupWeb()
{
  //list directory
  webserver.on("/list", HTTP_GET, handleFileList);
  //load editor
  webserver.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) webserver.send(404, "text/plain", "FileNotFound");
  });
  //create file
  webserver.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  webserver.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  webserver.on("/edit", HTTP_POST, [](){ webserver.send(200, "text/plain", ""); }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  webserver.onNotFound([](){
    if(!handleFileRead(webserver.uri()))
      webserver.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  webserver.on("/all", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"analog\":"+String(analogRead(A0));
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    webserver.send(200, "text/json", json);
    json = String();
  });
  
  webserver.on("/config", handleSettingsConfig);
  webserver.on("/form", HTTP_POST, handleForm);
  
  webserver.on("/", handleRoot);
  webserver.on("/load", handleLoad);
  webserver.on("/save", handleSave);
  webserver.on("/timeserver", handleTimeServer);
  webserver.on("/nextion", handleNextion);
  webserver.on("/sendsignal", webSendSignal);
  webserver.on("/recordsignal", webRecordSignal);
  webserver.begin();
}

void loopWeb() 
{
    webserver.handleClient();
}


String getContentType(String filename){
  if(webserver.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  DebugLn("handleFileRead: " + path);
  if(path.endsWith("/")) {
    if (SPIFFS.exists(path + "index.htm")) {
      path += "index.htm";
    }
    else {
      return handleHTMLList(path);
    }
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = webserver.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(webserver.uri() != "/edit") return;
  HTTPUpload& upload = webserver.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Debug("handleFileUpload Name: "); DebugLn(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //Debug("handleFileUpload Data: "); DebugLn(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    Debug("handleFileUpload Size: "); DebugLn(upload.totalSize);
  }
}

void handleFileDelete(){
  if(webserver.args() == 0) return webserver.send(500, "text/plain", "BAD ARGS");
  String path = webserver.arg(0);
  DebugLn("handleFileDelete: " + path);
  if(path == "/")
    return webserver.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return webserver.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  webserver.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(webserver.args() == 0)
    return webserver.send(500, "text/plain", "BAD ARGS");
  String path = webserver.arg(0);
  DebugLn("handleFileCreate: " + path);
  if(path == "/")
    return webserver.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return webserver.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return webserver.send(500, "text/plain", "CREATE FAILED");
  webserver.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!webserver.hasArg("dir")) {webserver.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = webserver.arg("dir");
  DebugLn("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  webserver.send(200, "text/json", output);
}

bool handleHTMLList(String path) {
  DebugLn("handleHTMLList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "<html><body><table><tr><th>Name<th>size</tr>";
  while(dir.next()){
    File entry = dir.openFile("r");
    output += "<tr><td><a href=\"";
    output += entry.name();
    output += "\">";
    output += entry.name();
    output += "</a><td>";
    output += entry.size();
    output += "</tr>";
  }
  
  output += "</table>";
  webserver.send(200, "text/html", output);
  return true;
}

void handleRoot() {
  String page = getPage("/mainpage.htm");
  String content = "<pre>";
  content += "\nD0: ";
  content += D0;
  content += "\nD1: ";
  content += D1;
  content += "\nD2: ";
  content += D2;
  content += "\nD3: ";
  content += D3;
  content += "\nD4: ";
  content += D4;
  content += "\nD5: ";
  content += D5;
  content += "\nD6: ";
  content += D6;
  content += "\nD7: ";
  content += D7;
  content += "\nD8: ";
  content += D8;
  content += "</pre>";
  page.replace("@@CONTENT@@", content);
  //sprintf(buffer, "%sC %s%% %02d:%02d:%02d\n%d\n%d\n%d\n%d", str_temperature, str_humidity, hour(t), minute(t), second(t), D0, D1, D2, D3);
  webserver.send(200, "text/html", page);
}

void handleSave() {
  settings.Save();
  webserver.send(200, "text/plain", "Saved");
}

void handleLoad() {
  settings.Load();
  webserver.send(200, "text/plain", settings.timeserver);
}

void handleTimeServer() {
  String output = "settings.timeserver: ";
  output += settings.timeserver;
  output += " settings.timezone: ";
  output += settings.timezone;
  output += ":";
  webserver.send(200, "text/plain", output);
}

String httpUpdateResponse = "";

String getPage(String pagename) {
  File pageFile = SPIFFS.open(pagename, "r");
  String page = pageFile.readString();
  time_t t = now();
  page.replace("@@SSID@@", settings.ssid);
  page.replace("@@PSK@@", settings.psk);
  page.replace("@@TIMEZONE@@", String(settings.timezone));
  page.replace("@@TIMESERVER@@", settings.timeserver);
  page.replace("@@SYNCINTERVAL@@", String(settings.syncinterval));
  page.replace("@@BRIGHTNESS@@", String(settings.brightness));
  page.replace("@@SYNCSTATUS@@", timeStatus() == timeSet ? "OK" : "Overdue");
  page.replace("@@DEVNAME@@", settings.devname);
  page.replace("@@HOUR@@", String(hour(t)));
  page.replace("@@MIN@@", String(minute(t)));
  page.replace("@@SEC@@", String(second(t)));
  page.replace("@@YEAR@@", String(year(t)));
  page.replace("@@MONTH@@", String(month(t)));
  page.replace("@@DAY@@", String(day(t)));
  page.replace("@@EPOCH@@", String(t));
  page.replace("@@LUX@@", String(lux));
  page.replace("@@LVL0LUX@@", String(settings.lvl0lux));
  page.replace("@@LVL1LUX@@", String(settings.lvl1lux));
  page.replace("@@LVL2LUX@@", String(settings.lvl2lux));
  //page.replace("@@LVL3LUX@@", String(settings.lvl3lux));
  page.replace("@@LUXMARGIN@@", String(settings.luxmargin));
  
  page.replace("@@UUID@@", settings.uuid);
  page.replace("@@TOKEN@@", settings.token);
  return page;
}

void handleSettingsConfig() {
  String page = getPage("/configpage.htm");;
  page.replace("@@UPDATERESPONSE@@", httpUpdateResponse);
  //s.replace("@@MODE@@", getMode());
  httpUpdateResponse = "";
  webserver.send(200, "text/html", page);
}

void handleNextion() {
  String page = "<html><body><form method=post><input name='cmd'><br><input type=submit>";
  String cmd = webserver.arg("cmd");
  if (cmd.length() > 0) {
    char cmdb[50];
    cmd.toCharArray(cmdb, 50);
    page += "<p>";
    page += cmd;
    page += "</p>Command sent";
  }
  webserver.send(200, "text/html", page);
}

void handleForm() {
  bool wifiUpdated = false;
  settings.devname = webserver.arg("devname");
  if (webserver.arg("ssid") != settings.ssid || 
      webserver.arg("psk") != settings.psk) {
    DebugLn("Updating WiFi Config");
    webserver.arg("ssid").toCharArray(settings.ssid, FIELD_LENGTH);
    webserver.arg("psk").toCharArray(settings.psk, FIELD_LENGTH);
    wifiUpdated = true;
   }
  String sTmp = webserver.arg("timeserver");
  sTmp.toCharArray(settings.timeserver, FIELD_LENGTH);

  settings.timezone = webserver.arg("timezone").toInt();
  settings.syncinterval = webserver.arg("syncinterval").toInt();
  settings.brightness = webserver.arg("brightness").toInt();
  settings.lvl0lux = webserver.arg("lvl0lux").toFloat();
  settings.lvl1lux = webserver.arg("lvl1lux").toFloat();
  settings.lvl2lux = webserver.arg("lvl2lux").toFloat();
  //settings.lvl3lux = webserver.arg("lvl3lux").toFloat();
  settings.luxmargin = webserver.arg("luxmargin").toFloat();

  settings.uuid = webserver.arg("uuid");
  settings.token = webserver.arg("token");
  
  settings.Save();
  httpUpdateResponse = "Settings Saved";

  applySettings();
  
  webserver.sendHeader("Location", "/config");
  webserver.send(302, "text/plain", "Moved");
/*
  if (update_wifi == "1") {
    delay(500);
    setupWiFi(0);
  }
  */
}

void webRecordSignal() {
  String signame = webserver.arg("sig");
  String page = "Ready to record signal: ";
  page += signame;
  webserver.send(200, "text/plain", page);
  rf433.recordSignal(signame);
}

void webSendSignal() {
  String signame = webserver.arg("sig");
  String page = "Sending signal: ";
  page += signame;
  webserver.send(200, "text/plain", page);
  rf433.sendSignal(signame);
}

