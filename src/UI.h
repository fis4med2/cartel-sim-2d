#pragma once
#include <string>
struct Renderer;
struct World;
struct TimeSystem;
struct Camera;
struct UI {
  void draw(Renderer& r, World& w, TimeSystem& t, Camera& c);
  void text(Renderer& r, const std::string& s, float x, float y, int sc, struct SDL_Color col);
  void textWorld(Renderer& r, const std::string& s, float wx, float wy, int sc, struct SDL_Color col, const Camera& c);
};
