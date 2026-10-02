#include "Territory.h"
Territory::Territory() {
  owner.assign(gw * gh, -1); prog.assign(gw * gh, 0); who.assign(gw * gh, -1);
}
Territory::Territory(int a, int b, float w, float h) : gw(a), gh(b), cw(w / a), ch(h / b) {
  owner.assign(a * b, -1); prog.assign(a * b, 0); who.assign(a * b, -1);
}
int Territory::cellAt(float x, float y) const {
  int cx = (int)(x / cw), cy = (int)(y / ch);
  if (cx < 0 || cy < 0 || cx >= gw || cy >= gh) return -1;
  return cy * gw + cx;
}
