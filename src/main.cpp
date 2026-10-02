#include "Game.h"
int main() {
  Game g;
  if (!g.init()) return 1;
  g.loop();
  return 0;
}
