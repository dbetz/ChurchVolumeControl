#pragma once

#define ROTARY_CLOCK    14 // D5
#define ROTARY_DATA     12 // D6
#define ROTARY_BUTTON   13 // D7

struct RotaryState {
  int delta;
};

void setupRotaryInterrupt();
bool getRotaryState(RotaryState &state);
