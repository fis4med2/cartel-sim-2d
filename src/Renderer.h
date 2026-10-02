#pragma once
#include <SDL2/SDL.h>
#include "Camera.h"
struct Renderer {
  SDL_Renderer* r;
  Renderer(SDL_Renderer* rr) : r(rr) {}
  void clear(SDL_Color c);
  void present();
  void worldFill(float wx, float wy, float ww, float wh, SDL_Color c, const Camera& cam);
  void worldRect(float wx, float wy, float ww, float wh, SDL_Color c, const Camera& cam);
  void worldLine(float x0, float y0, float x1, float y1, SDL_Color c, const Camera& cam);
  void screenFill(float x, float y, float w, float h, SDL_Color c);
};
