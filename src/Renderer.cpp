#include "Renderer.h"
void Renderer::clear(SDL_Color c) {
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
  SDL_RenderClear(r);
}
void Renderer::present() { SDL_RenderPresent(r); }
void Renderer::worldFill(float wx, float wy, float ww, float wh, SDL_Color c, const Camera& cam) {
  float sx, sy;
  cam.worldToScreen(wx, wy, sx, sy);
  float sw = ww * cam.zoom, sh = wh * cam.zoom;
  if (sx + sw < 0 || sy + sh < 0 || sx > cam.vw || sy > cam.vh) return;
  SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
  SDL_Rect rc{(int)sx, (int)sy, (int)sw, (int)sh};
  SDL_RenderFillRect(r, &rc);
}
void Renderer::worldRect(float wx, float wy, float ww, float wh, SDL_Color c, const Camera& cam) {
  float sx, sy;
  cam.worldToScreen(wx, wy, sx, sy);
  float sw = ww * cam.zoom, sh = wh * cam.zoom;
  if (sx + sw < 0 || sy + sh < 0 || sx > cam.vw || sy > cam.vh) return;
  SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
  SDL_Rect rc{(int)sx, (int)sy, (int)sw, (int)sh};
  SDL_RenderDrawRect(r, &rc);
}
void Renderer::worldLine(float x0, float y0, float x1, float y1, SDL_Color c, const Camera& cam) {
  float a, b, d, e;
  cam.worldToScreen(x0, y0, a, b);
  cam.worldToScreen(x1, y1, d, e);
  SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
  SDL_RenderDrawLine(r, (int)a, (int)b, (int)d, (int)e);
}
void Renderer::screenFill(float x, float y, float w, float h, SDL_Color c) {
  SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
  SDL_Rect rc{(int)x, (int)y, (int)w, (int)h};
  SDL_RenderFillRect(r, &rc);
}
