#include <Adafruit_DS1841.h>

#include <ESP8266WiFi.h>
#include "Rotary.h"

static volatile bool sampleNow = true;
static volatile int sampleCount = 0;
static volatile int Count = 0;

ICACHE_RAM_ATTR void handleRotaryInterrupt();

void setupRotaryInterrupt()
{
  timer1_attachInterrupt(handleRotaryInterrupt);
  // TIM_DIV1 = 0,   // 80MHz (80 ticks/us - 104857.588 us max)
  // TIM_DIV16 = 1,  // 5MHz (5 ticks/us - 1677721.4 us max)
  // TIM_DIV256 = 3  // 312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  timer1_write(5000); // 1ms
  pinMode(ROTARY_CLOCK, INPUT_PULLUP);
  pinMode(ROTARY_DATA, INPUT_PULLUP);
}

bool getRotaryState(RotaryState &state)
{
  if (!sampleNow)
    return false;
    
  noInterrupts();
  
  state.delta = Count;
  Count = 0;
  
  sampleNow = false;
  
  interrupts();
  
  return true;
}

ICACHE_RAM_ATTR void handleRotaryInterrupt()
{
#if 0
  static int a0 = 1;
  int a = digitalRead(ROTARY_CLOCK);
  int b = digitalRead(ROTARY_DATA);
  if (a != a0) {
    a0 = a;
    if (a == b)
      --Count;
    else
      ++Count;
  }
#else  
  static int a1 = 0, b1 = 0;
  a1 = (a1 << 1) | digitalRead(ROTARY_CLOCK);
  b1 = (b1 << 1) | digitalRead(ROTARY_DATA);
  if ((a1 & 0x03) == 0x02 && (b1 & 0x03) == 0x00)
      --Count;
  else if ((b1 & 0x03) == 0x02 && (a1 & 0x03) == 0x00)
      ++Count;
#endif
  
  if (++sampleCount >= 10) {
    sampleNow = true;
    sampleCount = 0;
  }
}
