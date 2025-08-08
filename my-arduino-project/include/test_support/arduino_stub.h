#pragma once
// Native unit test Arduino compatibility stub
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>

using String = std::string;

#ifndef HIGH
#define HIGH 0x1
#endif
#ifndef LOW
#define LOW  0x0
#endif

static unsigned long __fakeMillis = 0;
inline unsigned long millis() { return __fakeMillis; }
inline void delay(unsigned long ms) { __fakeMillis += ms; }
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline void ledcSetup(int,int,int) {}
inline void ledcAttachPin(int,int) {}
inline void ledcWrite(int,int) {}
inline long map(long x,long in_min,long in_max,long out_min,long out_max){return (x-in_min)*(out_max-out_min)/(in_max-in_min)+out_min;}

template <typename T> inline T constrainT(T x,T a,T b){return x<a?a:(x>b?b:x);} 
inline long constrain(long x,long a,long b){return constrainT<long>(x,a,b);} 
inline float constrain(float x,float a,float b){return constrainT<float>(x,a,b);} 
inline float fabsf_wrap(float v){return v<0?-v:v;}
#define fabs fabsf_wrap

#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(fmt,...)
