#pragma once
#include <vector>
struct Territory {
  int gw = 10, gh = 10;
  float cw = 320, ch = 320;
  std::vector<int> owner;
  std::vector<float> prog;
  std::vector<int> who;
  Territory();
  Territory(int a, int b, float w, float h);
  int cellAt(float x, float y) const;
};
