#pragma once
#include <cstdlib>
constexpr float WORLD_W = 4800;
constexpr float WORLD_H = 4800;
// Entrada/saida de forca sem base: ponto aleatorio na borda do mapa.
inline void edgePos(float& x, float& y) {
  int s = rand() % 4;
  int m = 90;
  if (s == 0) { x = m; y = m + rand() % (int)(WORLD_H - 2 * m); }
  else if (s == 1) { x = WORLD_W - m; y = m + rand() % (int)(WORLD_H - 2 * m); }
  else if (s == 2) { x = m + rand() % (int)(WORLD_W - 2 * m); y = m; }
  else { x = m + rand() % (int)(WORLD_W - 2 * m); y = WORLD_H - m; }
}
