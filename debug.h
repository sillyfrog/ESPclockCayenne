

#define DEBUG
// #undef DEBUG

#ifndef DEBUG
extern const char leds[] = { 16, 5, 4, 2, 14, 12, 13, 15, 3, 1 };
#define DebugStart()
#define DebugLn(s)
#define Debug(s)

#else
extern const char leds[] = { 16, 5, 4, 2, 14, 12, 13, 15 };
#define DebugStart() Serial.begin(115200)
#define DebugLn(s) Serial.println((s))
#define Debug(s) Serial.print((s))
#endif
