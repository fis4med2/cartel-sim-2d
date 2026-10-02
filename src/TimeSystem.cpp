#include "TimeSystem.h"
#include <cstdio>
void TimeSystem::update(float dt) {
  hour += dt * 0.2f;
  if (hour >= 24) hour -= 24;
}
float TimeSystem::light() const {
  if (hour < 5 || hour > 19) return 0.25f;
  if (hour < 7) return 0.25f + (hour - 5) * 0.375f;
  if (hour < 17) return 1.f;
  return 1.f - (hour - 17) * 0.375f;
}
int TimeSystem::overlay() const {
  float l = light();
  return (int)((1.f - l) * 150);
}
bool TimeSystem::night() const { return hour < 6 || hour >= 18; }
std::string TimeSystem::str() const {
  int h = (int)hour;
  int m = (int)((hour - h) * 60);
  char b[16];
  snprintf(b, sizeof b, "%02d:%02d %s", h, m, night() ? "NOITE" : "DIA");
  return b;
}
