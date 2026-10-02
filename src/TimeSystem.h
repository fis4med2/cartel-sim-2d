#pragma once
#include <string>
struct TimeSystem {
  float hour = 8.f;
  void update(float dt);
  float light() const;
  int overlay() const;
  std::string str() const;
  bool night() const;
};
