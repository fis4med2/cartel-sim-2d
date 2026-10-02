#pragma once
#include <SDL2/SDL.h>
struct Renderer;
struct Camera;
struct World;
struct TimeSystem;
struct UI;
struct Audio;
struct Game {
  SDL_Window* win = nullptr;
  SDL_Renderer* sdlr = nullptr;
  Renderer* ren = nullptr;
  Camera* cam = nullptr;
  World* world = nullptr;
  TimeSystem* time = nullptr;
  UI* ui = nullptr;
  Audio* au = nullptr;
  bool run = true;
  int W = 1280, H = 720;
  int speed = 1;
  bool init();
  void loop();
  void input(float dt);
  void update(float dt);
  void render();
  ~Game();
};
