#pragma once

struct RotaryState {
  int delta;
};

void setupRotaryInterrupt(int clockPin, int dataPin);
bool getRotaryState(RotaryState &state);
