#include "UI.h"
#include "Renderer.h"
#include "World.h"
#include "TimeSystem.h"
#include <SDL2/SDL.h>
#include <cctype>
static void glyph(char ch, uint8_t o[7]) {
  ch = toupper((unsigned char)ch);
  switch (ch) {
    case 'A': {uint8_t v[7]={0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'B': {uint8_t v[7]={0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'C': {uint8_t v[7]={0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'D': {uint8_t v[7]={0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'E': {uint8_t v[7]={0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'F': {uint8_t v[7]={0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'G': {uint8_t v[7]={0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'H': {uint8_t v[7]={0x11,0x11,0x11,0x1F,0x11,0x11,0x11}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'I': {uint8_t v[7]={0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'J': {uint8_t v[7]={0x07,0x02,0x02,0x02,0x02,0x12,0x0C}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'K': {uint8_t v[7]={0x11,0x12,0x14,0x18,0x14,0x12,0x11}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'L': {uint8_t v[7]={0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'M': {uint8_t v[7]={0x11,0x1B,0x15,0x15,0x11,0x11,0x11}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'N': {uint8_t v[7]={0x11,0x19,0x19,0x15,0x13,0x13,0x11}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'O': {uint8_t v[7]={0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'P': {uint8_t v[7]={0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'Q': {uint8_t v[7]={0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'R': {uint8_t v[7]={0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'S': {uint8_t v[7]={0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'T': {uint8_t v[7]={0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'U': {uint8_t v[7]={0x11,0x11,0x11,0x11,0x11,0x11,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'V': {uint8_t v[7]={0x11,0x11,0x11,0x11,0x11,0x0A,0x04}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'W': {uint8_t v[7]={0x11,0x11,0x11,0x15,0x15,0x1B,0x11}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'X': {uint8_t v[7]={0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'Y': {uint8_t v[7]={0x11,0x11,0x0A,0x04,0x04,0x04,0x04}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case 'Z': {uint8_t v[7]={0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '0': {uint8_t v[7]={0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '1': {uint8_t v[7]={0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '2': {uint8_t v[7]={0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '3': {uint8_t v[7]={0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '4': {uint8_t v[7]={0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '5': {uint8_t v[7]={0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '6': {uint8_t v[7]={0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '7': {uint8_t v[7]={0x1F,0x01,0x02,0x04,0x08,0x08,0x08}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '8': {uint8_t v[7]={0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '9': {uint8_t v[7]={0x0E,0x11,0x11,0x1F,0x01,0x02,0x0C}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case ':': {uint8_t v[7]={0,0x04,0,0,0,0x04,0}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '$': {uint8_t v[7]={0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '.': {uint8_t v[7]={0,0,0,0,0,0x0C,0x0C}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '-': {uint8_t v[7]={0,0,0,0x1F,0,0,0}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '+': {uint8_t v[7]={0,0x04,0x04,0x1F,0x04,0x04,0}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '/': {uint8_t v[7]={0x01,0x01,0x02,0x04,0x08,0x10,0x10}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    case '%': {uint8_t v[7]={0x19,0x1A,0x02,0x04,0x08,0x15,0x13}; for(int i=0;i<7;i++)o[i]=v[i]; break;}
    default: { for(int i=0;i<7;i++)o[i]=0; break; }
  }
}
void UI::text(Renderer& r, const std::string& s, float x, float y, int sc, SDL_Color col) {
  float cx = x;
  for (char ch : s) {
    if (ch == ' ') { cx += 6 * sc; continue; }
    uint8_t g[7]; glyph(ch, g);
    for (int row = 0; row < 7; row++)
      for (int c = 0; c < 5; c++)
        if (g[row] & (1 << (4 - c)))
          r.screenFill(cx + c * sc, y + row * sc, sc, sc, col);
    cx += 6 * sc;
  }
}
void UI::textWorld(Renderer& r, const std::string& s, float wx, float wy, int sc, SDL_Color col, const Camera& c) {
  float sx, sy;
  c.worldToScreen(wx, wy, sx, sy);
  float px = sx - s.size() * 3 * sc * c.zoom;
  float ps = sc * c.zoom;
  if (ps < 0.7f) ps = 0.7f;
  for (char ch : s) {
    if (ch == ' ') { px += 6 * ps; continue; }
    uint8_t g[7]; glyph(ch, g);
    for (int row = 0; row < 7; row++)
      for (int cc = 0; cc < 5; cc++)
        if (g[row] & (1 << (4 - cc)))
          r.screenFill(px + cc * ps, sy + row * ps, ps + 0.5f, ps + 0.5f, col);
    px += 6 * ps;
  }
}
void UI::draw(Renderer& r, World& w, TimeSystem& t, Camera& c) {
  r.screenFill(0, 0, c.vw, 56, {10, 10, 14, 210});
  for (size_t i = 0; i < w.cartels.size() && i < 4; i++) {
    auto& k = w.cartels[i];
    float x = 10 + i * 250;
    r.screenFill(x, 8, 240, 40, {22, 22, 30, 230});
    r.screenFill(x, 8, 6, 40, k.color);
    int ak = 0;
    for (auto& n : w.npcs) if (n.alive && n.team == k.id && n.wpn == 1) ak++;
    char b1[64], b2[96];
    snprintf(b1, sizeof b1, "%s $%d U%d", k.name.c_str(), (int)k.money, k.upg);
    snprintf(b2, sizeof b2, "TERR %d NPC %d/%d AK %d BLD %d/%d", w.terrCount(k.id), w.aliveCount(k.id), w.maxArmy(k.id), ak, w.armedCount(k.id), w.maxBlindado(k.id));
    text(r, b1, x + 12, 10, 2, {255, 255, 255, 255});
    text(r, b2, x + 12, 28, 1, {180, 180, 180, 255});
  }
  std::string ts = t.str();
  text(r, ts, c.vw - 170, 10, 2, {255, 240, 160, 255});
  int total = 0, cl = 0, ch = 0, cs = 0, cm = 0, grd = 0;
  for (auto& n : w.npcs) if (n.alive) {
    total++;
    if (n.cls == 0) cl++; else if (n.cls == 1) ch++; else if (n.cls == 2) cs++; else cm++;
    if (n.role == 0) grd++;
  }
  char b3[64], b4[96], b5[96];
  snprintf(b3, sizeof b3, "NPC %d", total);
  snprintf(b4, sizeof b4, "L %d H %d S %d M %d G %d", cl, ch, cs, cm, grd);
  int free = 0, drv = 0, hil = 0, mon = 0, trf = 0;
  for (auto& cc : w.cars) if (!cc.wreck) {
    if (cc.traffic) { trf++; continue; }
    if (cc.busy) drv++; else free++;
    if (cc.model == 1) hil++; else if (cc.model == 2) mon++;
  }
  snprintf(b5, sizeof b5, "CAR %d+%d H%d M%d T%d", drv, free, hil, mon, trf);
  text(r, b3, c.vw - 170, 22, 1, {160, 160, 160, 255});
  text(r, b4, c.vw - 170, 32, 1, {140, 200, 140, 255});
  text(r, b5, c.vw - 170, 42, 1, {120, 180, 220, 255});
  text(r, "WASD MOVE WHEEL ZOOM Q/E ZOOM SPACE VEL", 12, c.vh - 20, 1, {140, 140, 140, 255});
  for (auto& k : w.cartels) {
    char bb[64];
    snprintf(bb, sizeof bb, "%s $%d", k.name.c_str(), (int)k.money);
    textWorld(r, k.name, k.mx, k.my - 108, 2, {0, 0, 0, 255}, c);
    textWorld(r, k.name, k.mx, k.my - 110, 2, {255, 255, 255, 255}, c);
    textWorld(r, bb, k.mx, k.my + 92, 2, {0, 0, 0, 255}, c);
    textWorld(r, bb, k.mx, k.my + 90, 2, {255, 240, 160, 255}, c);
  }
}
