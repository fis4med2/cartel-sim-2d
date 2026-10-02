#pragma once
#include <SDL2/SDL.h>
#include <string>
struct Cartel {
  int id = 0;
  std::string name;
  SDL_Color color{255, 255, 255, 255};
  float money = 1000;
  float mx = 0, my = 0;
  int upg = 0;
  float baseT = 0;
  float backupT = 0;
  float prodT = 0;
  Cartel() {}
  Cartel(int i, std::string n, SDL_Color c, float x, float y);
};
