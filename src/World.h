#pragma once
#include "City.h"
#include "Territory.h"
#include "Cartel.h"
#include "NPC.h"
#include "Car.h"
#include "TimeSystem.h"
#include <vector>
struct Renderer;
struct Camera;
struct World {
  City city;
  Territory terr;
  std::vector<Cartel> cartels;
  std::vector<NPC> npcs;
  struct Drop { float x, y; int wpn; float t; };
  struct Tracer { float x0, y0, x1, y1, t; };
  struct Corpse { float x, y; int team, cls; float t; };
  struct Blood { float x, y, r, t; };
  struct Bullet { float x, y, vx, vy, life, dmg; int team, wpn; };
  struct Spark { float x, y, vx, vy, t; };
  struct Snd { int wpn; float x, y; };
  std::vector<Drop> drops;
  std::vector<Tracer> tracers;
  std::vector<Corpse> corpses;
  std::vector<Blood> blood;
  std::vector<Car> cars;
  std::vector<Bullet> bullets;
  std::vector<Spark> sparks;
  std::vector<Snd> sounds;
  float moneyT = 0, eventT = 0, respT = 0, saveT = 0;
  float hour = 8;
  int tick = 0;
  World();
  void gen();
  void update(float dt, TimeSystem& t);
  void render(Renderer& r, Camera& c);
  void renderLights(Renderer& r, Camera& c);
  void baseAI(float dt);
  void buySquad(int team);
  bool buyBlindado(int team);
  bool buyHilux(int team);
  void buyUpg(int team);
  int armedCount(int team) const;
  int hiluxCount() const;
  int maxArmy(int team) const;
  int maxBlindado(int team) const;
  int maxHilux(int team) const;
  int squadSize(int team) const;
  float prodInterval(int team) const;
  void saveBackup() const;
  void loadBackup();
  int aliveCount(int team) const;
  int terrCount(int team) const;
};
