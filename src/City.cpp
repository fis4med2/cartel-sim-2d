#include "City.h"
#include "Renderer.h"
#include "Config.h"
#include <cstdlib>
#include <cmath>
bool City::inRoad(float x, float y, float w, float h) const {
  bool vx = x < roadX1 && x + w > roadX0;
  bool hy = y < roadY1 && y + h > roadY0;
  return vx || hy;
}
bool City::onRoad(float x, float y) const {
  return (x > roadX0 - 10 && x < roadX1 + 10) || (y > roadY0 - 10 && y < roadY1 + 10);
}
void City::roadPoint(float x, float y, float& ox, float& oy) const {
  float cx = (roadX0 + roadX1) / 2, cy = (roadY0 + roadY1) / 2;
  float dv = fabsf(x - cx), dh = fabsf(y - cy);
  if (dv < dh) { ox = cx; oy = y; }
  else { ox = x; oy = cy; }
  ox = fmaxf(0, fminf(WORLD_W, ox));
  oy = fmaxf(0, fminf(WORLD_H, oy));
}
void City::gen(const std::vector<std::pair<float, float>>& mans) {
  houses.clear();
  for (int i = 0; i < 1800 && houses.size() < 450; i++) {
    float w = 50 + rand() % 60, h = 50 + rand() % 60;
    float x = rand() % (int)(WORLD_W - w), y = rand() % (int)(WORLD_H - h);
    if (inRoad(x - 20, y - 20, w + 40, h + 40)) continue;
    bool bad = false;
    for (auto& m : mans) {
      float dx = x - m.first, dy = y - m.second;
      if (sqrtf(dx * dx + dy * dy) < 300) { bad = true; break; }
    }
    if (bad) continue;
    for (auto& o : houses)
      if (x < o.x + o.w + 12 && x + w + 12 > o.x && y < o.y + o.h + 12 && y + h + 12 > o.y) { bad = true; break; }
    if (bad) continue;
    houses.push_back({x, y, w, h});
  }
  buildBlock();
}
void City::buildBlock() {
  block.assign(bw * bh, 0);
  for (int cy = 0; cy < bh; cy++)
    for (int cx = 0; cx < bw; cx++) {
      float x = cx * cell + cell / 2, y = cy * cell + cell / 2;
      for (auto& h : houses) {
        if (x > h.x - 18 && x < h.x + h.w + 18 && y > h.y - 18 && y < h.y + h.h + 18) {
          block[cy * bw + cx] = 1; break;
        }
      }
    }
}
bool City::blockedAt(float x, float y) const {
  int cx = (int)(x / cell), cy = (int)(y / cell);
  if (cx < 0 || cy < 0 || cx >= bw || cy >= bh) return true;
  return block[cy * bw + cx];
}
static int nearFree(const City& c, int cx, int cy) {
  if (cx >= 0 && cy >= 0 && cx < c.bw && cy < c.bh && !c.block[cy * c.bw + cx]) return cy * c.bw + cx;
  for (int r = 1; r <= 3; r++)
    for (int y = cy - r; y <= cy + r; y++)
      for (int x = cx - r; x <= cx + r; x++) {
        if (x < 0 || y < 0 || x >= c.bw || y >= c.bh) continue;
        if (!c.block[y * c.bw + x]) return y * c.bw + x;
      }
  return -1;
}
int City::findPath(float x0, float y0, float x1, float y1, float ox[8], float oy[8]) const {
  int s = nearFree(*this, (int)(x0 / cell), (int)(y0 / cell));
  int g = nearFree(*this, (int)(x1 / cell), (int)(y1 / cell));
  if (s < 0 || g < 0 || s == g) return 0;
  int N = bw * bh;
  static float gs[9216];
  static int par[9216], open[9216];
  static char closed[9216], inOpen[9216];
  for (int i = 0; i < N; i++) { gs[i] = 1e18f; par[i] = -1; closed[i] = 0; inOpen[i] = 0; }
  int gx = g % bw, gy = g / bw;
  auto H = [&](int i) {
    int dx = abs(i % bw - gx), dy = abs(i / bw - gy);
    return (dx + dy) + 0.41f * (dx < dy ? dx : dy);
  };
  gs[s] = 0;
  int on = 0;
  open[on++] = s;
  inOpen[s] = 1;
  const int DX[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  const int DY[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  const float DC[8] = {1, 1, 1, 1, 1.414f, 1.414f, 1.414f, 1.414f};
  int found = -1, iter = 0;
  while (on > 0 && iter++ < 12000) {
    int bi = 0;
    float bf = gs[open[0]] + H(open[0]);
    for (int i = 1; i < on; i++) {
      float f = gs[open[i]] + H(open[i]);
      if (f < bf) { bf = f; bi = i; }
    }
    int cur = open[bi];
    open[bi] = open[--on];
    inOpen[cur] = 0;
    if (closed[cur]) continue;
    closed[cur] = 1;
    if (cur == g) { found = cur; break; }
    int cx = cur % bw, cy = cur / bw;
    for (int d = 0; d < 8; d++) {
      int nx = cx + DX[d], ny = cy + DY[d];
      if (nx < 0 || ny < 0 || nx >= bw || ny >= bh) continue;
      int ni = ny * bw + nx;
      if (block[ni] || closed[ni]) continue;
      if (d >= 4 && (block[cy * bw + nx] || block[ny * bw + cx])) continue;
      float cxw = nx * cell + cell / 2, cyw = ny * cell + cell / 2;
      float ng = gs[cur] + DC[d] * (onRoad(cxw, cyw) ? 0.6f : 1.0f);
      if (ng < gs[ni]) {
        gs[ni] = ng; par[ni] = cur;
        // sem isto o mesmo no entra N vezes e estoura open[2304]
        if (!inOpen[ni]) { inOpen[ni] = 1; open[on++] = ni; }
      }
    }
  }
  if (found < 0) {
    float rx, ry, gx2, gy2;
    roadPoint(x0, y0, rx, ry);
    roadPoint(x1, y1, gx2, gy2);
    float cand[4][2] = {{rx, ry}, {WORLD_W / 2, WORLD_H / 2}, {gx2, gy2}, {x1, y1}};
    int n = 0;
    for (int i = 0; i < 4 && n < 8; i++) {
      float dx = cand[i][0] - (n ? ox[n - 1] : x0), dy = cand[i][1] - (n ? oy[n - 1] : y0);
      if (dx * dx + dy * dy < 6400) continue;
      ox[n] = cand[i][0]; oy[n] = cand[i][1]; n++;
    }
    return n;
  }
  int path[9216], pn = 0;
  for (int i = found; i >= 0 && pn < 9216; i = par[i]) path[pn++] = i;
  // ordem: inicio -> fim
  float cxw[9216], cyw[9216];
  for (int i = 0; i < pn; i++) {
    int c = path[pn - 1 - i];
    cxw[i] = (c % bw) * cell + cell / 2;
    cyw[i] = (c / bw) * cell + cell / 2;
  }
  // string pulling: so emite waypoint quando a reta ate o proximo bate em parede
  auto clear = [&](int a, int b) {
    float dx = cxw[b] - cxw[a], dy = cyw[b] - cyw[a];
    int steps = (int)(sqrtf(dx * dx + dy * dy) / 18) + 1;
    for (int k = 1; k <= steps; k++) {
      float t = (float)k / steps;
      if (blockedAt(cxw[a] + dx * t, cyw[a] + dy * t)) return false;
    }
    return true;
  };
  int n = 0, anchor = 0;
  while (anchor < pn - 1 && n < 7) {
    int far = anchor + 1;
    for (int j = anchor + 2; j < pn; j++)
      if (clear(anchor, j)) far = j; else break;
    ox[n] = cxw[far]; oy[n] = cyw[far]; n++;
    anchor = far;
  }
  if (n == 0) {
    ox[0] = x1; oy[0] = y1;
    return 1;
  }
  // so encosta no alvo exato se a reta estiver limpa; senao mantem o centro da celula
  float lx = ox[n - 1], ly = oy[n - 1];
  float dx = x1 - lx, dy = y1 - ly;
  int steps = (int)(sqrtf(dx * dx + dy * dy) / 18) + 1;
  bool vis = true;
  for (int k = 1; k <= steps && vis; k++) {
    float t = (float)k / steps;
    if (blockedAt(lx + dx * t, ly + dy * t)) vis = false;
  }
  if (vis) { ox[n - 1] = x1; oy[n - 1] = y1; }
  return n;
}
void City::render(Renderer& r, const Camera& c) const {
  r.worldFill(0, 0, WORLD_W, WORLD_H, {34, 45, 34, 255}, c);
  r.worldFill(roadX0, 0, roadX1 - roadX0, WORLD_H, {55, 55, 60, 255}, c);
  r.worldFill(0, roadY0, WORLD_W, roadY1 - roadY0, {55, 55, 60, 255}, c);
  SDL_Color dash{220, 220, 120, 255};
  for (float y = 0; y < WORLD_H; y += 80)
    if (y < roadY0 || y > roadY1) r.worldFill((roadX0 + roadX1) / 2 - 3, y, 6, 40, dash, c);
  for (float x = 0; x < WORLD_W; x += 80)
    if (x < roadX0 || x > roadX1) r.worldFill(x, (roadY0 + roadY1) / 2 - 3, 40, 6, dash, c);
  for (auto& h : houses) {
    // variedade: 5 matrizes por indice, muda a planta sem mexer na hitbox
    int ax = (int)(h.x + h.y * 7) / 97 % 5;
    r.worldFill(h.x, h.y, h.w, h.h, {95, 95, 105, 255}, c);
    r.worldRect(h.x, h.y, h.w, h.h, {35, 35, 45, 255}, c);
    r.worldRect(h.x + 1, h.y + 1, h.w - 2, h.h - 2, {115, 115, 125, 255}, c);
    // cumeeira com tom alterado por variante
    SDL_Color roof{180, 90, 90, 255};
    if (ax == 1) roof = {80, 110, 150, 255};
    else if (ax == 2) roof = {150, 140, 90, 255};
    else if (ax == 3) roof = {90, 130, 90, 255};
    r.worldFill(h.x + 2, h.y + 2, h.w - 4, 3, roof, c);
    r.worldFill(h.x + 2, h.y + h.h - 5, h.w - 4, 3, roof, c);
    r.worldFill(h.x + 2, h.y + 2, 3, h.h - 4, roof, c);
    r.worldFill(h.x + h.w - 5, h.y + 2, 3, h.h - 4, roof, c);
    // interior
    r.worldFill(h.x + h.w * 0.2f, h.y + h.h * 0.2f, h.w * 0.6f, h.h * 0.6f, {70, 70, 82, 255}, c);
    // porta (lado sul, sempre)
    float doorW = fminf(12, h.w * 0.18f);
    r.worldFill(h.x + h.w / 2 - doorW / 2, h.y + h.h - 5, doorW, 5, {48, 40, 40, 255}, c);
    r.worldFill(h.x + h.w / 2 - doorW / 2 + 1, h.y + h.h - 5, doorW - 2, 5, {58, 48, 48, 255}, c);
    // janela (desoca no lado com folga)
    float win = fminf(8, h.w * 0.12f);
    float winX = (ax % 2) ? h.x + 6 : h.x + h.w - 6 - win;
    float winY = h.y + h.h * 0.3f;
    r.worldFill(winX, winY, win, win, {40, 50, 60, 255}, c);
    // variante H1: cumeeira cruzada
    if (ax == 4) {
      r.worldFill(h.x + 2, h.y + h.h / 2 - 1, h.w - 4, 3, roof, c);
      r.worldFill(h.x + h.w / 2 - 1, h.y + 2, 3, h.h - 4, roof, c);
    }
    // solo ao redor
    r.worldRect(h.x - 1, h.y - 1, h.w + 2, h.h + 2, {28, 30, 28, 120}, c);
  }
}
void City::renderLights(Renderer& r, const Camera& c) const {
  SDL_Color l{255, 220, 120, 255};
  SDL_Color w{255, 238, 170, 190};
  for (auto& h : houses) {
    // halo central fraco
    r.worldFill(h.x + h.w / 2 - 4, h.y + h.h / 2 - 4, 8, 8, l, c);
    // janela acesa, mesma posicao que o render de dia
    int ax = (int)(h.x + h.y * 7) / 97 % 5;
    float win = fminf(8, h.w * 0.12f);
    float winX = (ax % 2) ? h.x + 6 : h.x + h.w - 6 - win;
    float winY = h.y + h.h * 0.3f;
    r.worldFill(winX - 1, winY - 1, win + 2, win + 2, w, c);
  }
}
