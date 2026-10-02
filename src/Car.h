#pragma once
#include <SDL2/SDL.h>
enum { CAR_NORMAL, CAR_HILUX, CAR_MONSTRO, CAR_CIV };
struct Car {
  float x = 0, y = 0, vx = 0, vy = 0, ang = 0, tur = 0;
  float hp = 200, maxHp = 200;
  int team = -1;
  int model = CAR_NORMAL;
  bool busy = false;
  bool wreck = false;
  bool armed = false;
  bool traffic = false;
  int axis = 0;
  float dir = 1;
  float cruise = 250;
  SDL_Color paint{180, 180, 190, 255};
  float t = 0;
  float ram = 0;
  float stuck = 0;
  float gun = 0;
  float idle = 0;
};
