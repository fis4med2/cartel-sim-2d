#include "World.h"
#include "AI.h"
#include "Renderer.h"
#include "Weapon.h"
#include "Config.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>
World::World() : terr(10, 10, WORLD_W, WORLD_H) {}
int World::aliveCount(int team) const {
  int n = 0;
  for (auto& o : npcs) if (o.alive && o.team == team) n++;
  return n;
}
int World::terrCount(int team) const {
  int n = 0;
  for (auto o : terr.owner) if (o == team) n++;
  return n;
}
void World::gen() {
  cartels.clear(); npcs.clear(); corpses.clear(); drops.clear(); tracers.clear(); cars.clear(); bullets.clear(); sparks.clear(); sounds.clear();
  cartels.emplace_back(0, "SINALOA", SDL_Color{220, 50, 50, 255}, 700, 700);
  cartels.emplace_back(1, "CALI", SDL_Color{60, 120, 255, 255}, 4100, 700);
  cartels.emplace_back(2, "JUAREZ", SDL_Color{60, 200, 80, 255}, 700, 4100);
  cartels.emplace_back(3, "MEDALLO", SDL_Color{240, 200, 40, 255}, 4100, 4100);
  std::vector<std::pair<float, float>> mans;
  for (auto& k : cartels) mans.push_back({k.mx, k.my});
  city.gen(mans);
  for (int cy = 0; cy < terr.gh; cy++)
    for (int cx = 0; cx < terr.gw; cx++) {
      float x = cx * terr.cw, y = cy * terr.ch;
      int i = cy * terr.gw + cx;
      if (city.inRoad(x, y, terr.cw, terr.ch)) { terr.owner[i] = -1; continue; }
      int o = (cx < 5 ? 0 : 1) + (cy < 5 ? 0 : 2);
      terr.owner[i] = o;
    }
  for (auto& k : cartels) {
    int setup[16] = {0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,0};
    for (int i = 0; i < 16; i++) {
      NPC n;
      n.team = k.id;
      n.cls = setup[i];
      n.wpn = n.cls == CLS_HEAVY ? W_AK : n.cls == CLS_SNIPER ? W_SNIPER : W_PISTOL;
      n.hp = clsMaxHp(n.cls);
      setRole(n);
      n.x = k.mx + rand() % 400 - 200;
      n.y = k.my + rand() % 400 - 200;
      n.wx = n.x; n.wy = n.y;
      npcs.push_back(n);
    }
  }
  {
    SDL_Color past[5] = {{200, 90, 90, 255}, {90, 140, 200, 255}, {200, 200, 210, 255}, {90, 180, 140, 255}, {200, 170, 90, 255}};
    float ccx2 = (city.roadX0 + city.roadX1) / 2, ccy2 = (city.roadY0 + city.roadY1) / 2;
    for (int i = 0; i < 10; i++) {
      Car c;
      c.traffic = true; c.model = CAR_CIV; c.maxHp = 120; c.hp = 120;
      c.paint = past[rand() % 5];
      c.axis = i < 5 ? 0 : 1;
      c.dir = (i % 2) ? 1 : -1;
      c.cruise = 220 + rand() % 70;
      float lane = c.dir > 0 ? 55 : -55;
      for (int tries = 0; tries < 8; tries++) {
        if (c.axis == 0) { c.x = ccx2 + lane; c.y = 100 + rand() % (int)(WORLD_H - 200); }
        else { c.y = ccy2 + lane; c.x = 100 + rand() % (int)(WORLD_W - 200); }
        bool ok = true;
        for (auto& o : cars) {
          if (!o.traffic) continue;
          float dx = o.x - c.x, dy = o.y - c.y;
          if (dx * dx + dy * dy < 22500) { ok = false; break; }
        }
        if (ok) break;
      }
      c.ang = c.axis == 0 ? (c.dir > 0 ? 1.57f : -1.57f) : (c.dir > 0 ? 0 : 3.14f);
      cars.push_back(c);
    }
  }
  loadBackup();
}
void World::update(float dt, TimeSystem& ts) {
  hour = ts.hour;
  tick++;
  for (auto& n : npcs) aiUpdate(n, *this, dt);
  for (auto& n : npcs) {
    if (!n.alive) {
      corpses.push_back({n.x, n.y, n.team, n.cls, 8.f});
      // poça: nasce perto do cadáver, variando pressa/tamanho
      for (int i = 0; i < 4 + (rand() % 4); i++)
        blood.push_back({n.x + (rand() % 18 - 9), n.y + (rand() % 18 - 9),
          4 + (rand() % 100) / 12.f, 90 + (rand() % 100) / 4.f});
      if (n.carId >= 0 && n.carId < (int)cars.size()) {
        bool other = false;
        for (auto& o : npcs) {
          if (&o == &n || !o.alive) continue;
          if (o.carId == n.carId) { other = true; break; }
        }
        if (!other) {
          cars[n.carId].busy = false;
          if (!cars[n.carId].armed) cars[n.carId].team = -1;
        }
      }
    }
  }
  npcs.erase(std::remove_if(npcs.begin(), npcs.end(),
    [](const NPC& n) { return !n.alive; }), npcs.end());
  for (auto& cp : corpses) cp.t -= dt;
  corpses.erase(std::remove_if(corpses.begin(), corpses.end(),
    [](const Corpse& c) { return c.t <= 0; }), corpses.end());
  for (auto& bl : blood) bl.t -= dt;
  blood.erase(std::remove_if(blood.begin(), blood.end(),
    [](const Blood& b) { return b.t <= 0; }), blood.end());
  for (auto& c : cars) {
    c.ram -= dt; c.gun -= dt;
    if (c.traffic && !c.wreck) {
      float sp = c.cruise;
      float ccx3 = (city.roadX0 + city.roadX1) / 2, ccy3 = (city.roadY0 + city.roadY1) / 2;
      float lane = c.dir > 0 ? 55 : -55;
      float kk = fminf(1, dt * 2);
      if (c.axis == 0) { c.x += (ccx3 + lane - c.x) * kk; c.vx = 0; c.vy = c.dir * sp; c.y += c.vy * dt; }
      else { c.y += (ccy3 + lane - c.y) * kk; c.vy = 0; c.vx = c.dir * sp; c.x += c.vx * dt; }
      if (fabsf(c.vx) + fabsf(c.vy) > 20) c.ang = atan2f(c.vy, c.vx);
      float ccx = (city.roadX0 + city.roadX1) / 2, ccy = (city.roadY0 + city.roadY1) / 2;
      bool out = c.axis == 0 ? (c.y > WORLD_H - 50 || c.y < 50) : (c.x > WORLD_W - 50 || c.x < 50);
      if (out) {
        float lane = c.dir > 0 ? 55 : -55;
        if (c.axis == 0) { c.x = ccx + lane; c.y = c.dir > 0 ? 60 : WORLD_H - 60; }
        else { c.y = ccy + lane; c.x = c.dir > 0 ? 60 : WORLD_W - 60; }
        c.vx = c.vy = 0;
      }
      continue;
    }
    if (c.wreck) {
      c.t -= dt;
      if (c.t <= 0) {
        c.vx = c.vy = 0; c.t = 0;
        int bkreal = rand() % (int)cartels.size();
        c.x = fmaxf(60, fminf(WORLD_W - 60, cartels[bkreal].mx + 150 + rand() % 120));
        c.y = fmaxf(60, fminf(WORLD_H - 60, cartels[bkreal].my + 150 + rand() % 120));
        c.armed = false; c.model = CAR_NORMAL; c.maxHp = 200; c.hp = 200; c.team = -1;
        c.wreck = false; c.busy = false;
      }
      continue;
    }
    c.x += c.vx * dt; c.y += c.vy * dt;
    c.vx *= (1 - fminf(1, dt * 1.5f)); c.vy *= (1 - fminf(1, dt * 1.5f));
    float csp0 = sqrtf(c.vx * c.vx + c.vy * c.vy);
    if (csp0 > 30) {
      float want = atan2f(c.vy, c.vx);
      float d0 = want - c.ang;
      while (d0 > 3.14159f) d0 -= 6.28318f;
      while (d0 < -3.14159f) d0 += 6.28318f;
      c.ang += d0 * fminf(1, dt * 8);
      if (!c.armed) c.tur = c.ang;
    }
    c.x = fmaxf(0, fminf(WORLD_W, c.x));
    c.y = fmaxf(0, fminf(WORLD_H, c.y));
    float spd = sqrtf(c.vx * c.vx + c.vy * c.vy);
    if (!c.traffic && spd > 150 && c.ram <= 0) {
      for (auto& n : npcs) {
        if (!n.alive || n.carId >= 0) continue;
        bool enemy = (c.team < 0) ? true : (n.team != c.team);
        if (!enemy) continue;
        float dx = n.x - c.x, dy = n.y - c.y;
        if (dx * dx + dy * dy < 625) {
          n.hp -= 60; n.hit = 0.2f;
          n.vx += dx * 4; n.vy += dy * 4;
          c.ram = 1.f;
          if (n.hp <= 0 && n.alive) {
            n.alive = false;
            if (c.team >= 0) cartels[c.team].money += 60;
          }
          break;
        }
      }
    }
  }
  for (auto& tr : tracers) tr.t -= dt;
  tracers.erase(std::remove_if(tracers.begin(), tracers.end(),
    [](const Tracer& t) { return t.t <= 0; }), tracers.end());
  for (auto& sp : sparks) { sp.t -= dt; sp.x += sp.vx * dt; sp.y += sp.vy * dt; sp.vx *= 0.92f; sp.vy *= 0.92f; }
  sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
    [](const Spark& s) { return s.t <= 0; }), sparks.end());
  auto burst = [&](float x, float y, int n, float spd) {
    for (int i = 0; i < n && (int)sparks.size() < 500; i++) {
      float a = (rand() % 628) / 100.0f;
      float v = spd * (0.4f + (rand() % 100) / 100.0f);
      sparks.push_back({x, y, cosf(a) * v, sinf(a) * v, 0.25f + (rand() % 100) / 500.0f});
    }
  };
  for (auto& b : bullets) {
    b.life -= dt;
    float step = sqrtf(b.vx * b.vx + b.vy * b.vy) * dt;
    int sub = (int)(step / 8) + 1;
    float sdx = b.vx * dt / sub, sdy = b.vy * dt / sub;
    for (int s = 0; s < sub && b.life > 0; s++) {
      b.x += sdx; b.y += sdy;
      bool stop = false;
      for (auto& h : city.houses) {
        if (b.x > h.x && b.x < h.x + h.w && b.y > h.y && b.y < h.y + h.h) {
          tracers.push_back({b.x - b.vx * 0.012f, b.y - b.vy * 0.012f, b.x, b.y, 0.12f});
          burst(b.x, b.y, 3, 160);
          b.life = 0; stop = true; break;
        }
      }
      if (stop) break;
      for (int ci = 0; ci < (int)cars.size() && b.life > 0; ci++) {
        Car& cc = cars[ci];
        if (cc.wreck || cc.team < 0 || cc.team == b.team) continue;
        float cr = cc.model == CAR_MONSTRO ? 21 : cc.model == CAR_HILUX ? 18 : 15;
        float dx = cc.x - b.x, dy = cc.y - b.y;
        if (dx * dx + dy * dy < cr * cr) {
          cc.hp -= b.dmg;
          burst(b.x, b.y, 6, 260);
          tracers.push_back({b.x - b.vx * 0.01f, b.y - b.vy * 0.01f, b.x, b.y, 0.1f});
          if (cc.hp <= 0) {
            cc.wreck = true; cc.t = 12.f; cc.busy = false; cc.vx = cc.vy = 0;
            for (auto& dd : npcs) {
              if (dd.alive && dd.carId == ci) {
                dd.carId = -1; dd.hp -= 30; dd.hit = 0.25f; dd.x += 24; dd.vx = 120;
                if (dd.hp <= 0) dd.alive = false;
              }
            }
          }
          b.life = 0; stop = true; break;
        }
      }
      if (stop) break;
      for (auto& nn : npcs) {
        if (!nn.alive || nn.team == b.team || nn.carId >= 0) continue;
        float rr = clsSize(nn.cls) / 2 + 2;
        float dx = nn.x - b.x, dy = nn.y - b.y;
        if (dx * dx + dy * dy < rr * rr) {
          float mv = sqrtf(nn.vx * nn.vx + nn.vy * nn.vy);
          float dd = sqrtf(b.vx * b.vx + b.vy * b.vy);
          nn.hp -= b.dmg * (b.wpn == W_SNIPER ? 1.0f : 0.85f + 0.15f * (1 - fminf(1, mv / 200)));
          nn.hit = 0.18f;
          if (dd > 1) { nn.vx += b.vx / dd * 30; nn.vy += b.vy / dd * 30; }
          // sangue jogado na direcao do tiro + pingue no ponto
          float bnx = dd > 1 ? b.vx / dd : 0, bny = dd > 1 ? b.vy / dd : 0;
          blood.push_back({nn.x + bnx * (8 + rand() % 14), nn.y + bny * (8 + rand() % 14),
            2 + (rand() % 100) / 28.f, 55 + (rand() % 100) / 3.f});
          blood.push_back({nn.x - bnx * (4 + rand() % 8), nn.y - bny * (4 + rand() % 8),
            1 + (rand() % 50) / 25.f, 40 + (rand() % 100) / 5.f});
          if (blood.size() > 600) blood.erase(blood.begin(), blood.begin() + 100);
          burst(b.x, b.y, 2, 120);
          tracers.push_back({b.x - b.vx * 0.008f, b.y - b.vy * 0.008f, b.x, b.y, 0.08f});
          if (nn.hp <= 0 && nn.alive) {
            nn.alive = false;
            cartels[b.team].money += b.wpn == W_AK ? 80 : b.wpn == W_SNIPER ? 120 : 50;
            if (nn.wpn != W_PISTOL) drops.push_back({nn.x, nn.y, nn.wpn, 20.f});
          }
          b.life = 0; break;
        }
      }
    }
  }
  bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
    [](const Bullet& b) { return b.life <= 0; }), bullets.end());
  for (auto& d : drops) d.t -= dt;
  drops.erase(std::remove_if(drops.begin(), drops.end(),
    [](const Drop& d) { return d.t <= 0; }), drops.end());
  for (auto& n : npcs) {
    if (!n.alive || n.cls == CLS_HEAVY || n.cls == CLS_SNIPER) continue;
    if (n.wpn != W_PISTOL) continue;
    for (auto it = drops.begin(); it != drops.end(); ++it) {
      float dx = n.x - it->x, dy = n.y - it->y;
      if (dx * dx + dy * dy < 400) { n.wpn = it->wpn; drops.erase(it); break; }
    }
  }
  int G = terr.gw * terr.gh, T = (int)cartels.size();
  static int cnt[128 * 4];
  for (int i = 0; i < G; i++) cnt[i * 4] = cnt[i * 4 + 1] = cnt[i * 4 + 2] = cnt[i * 4 + 3] = 0;
  for (auto& n : npcs) {
    if (!n.alive) continue;
    int c = terr.cellAt(n.x, n.y);
    if (c >= 0) cnt[c * 4 + n.team]++;
  }
  for (int c = 0; c < G; c++) {
    int best = -1, bv = 0, second = 0;
    for (int t = 0; t < T; t++) {
      int v = cnt[c * 4 + t];
      if (v > bv) { second = bv; bv = v; best = t; }
      else if (v > second) second = v;
    }
    if (best >= 0 && bv > 0 && second == 0 && terr.owner[c] != best) {
      if (terr.who[c] != best) { terr.who[c] = best; terr.prog[c] = 0; }
      terr.prog[c] += dt;
      if (terr.prog[c] > 2.5f) {
        terr.owner[c] = best; terr.prog[c] = 0; terr.who[c] = -1;
        cartels[best].money += 150;
      }
    } else terr.prog[c] = fmaxf(0, terr.prog[c] - dt * 2);
  }
  moneyT += dt;
  if (moneyT > 1) {
    moneyT = 0;
    for (auto& k : cartels) {
      int alive = aliveCount(k.id);
      float upkeep = alive * 1.5f + armedCount(k.id) * 3.0f;
      k.money += 5 + terrCount(k.id) * 4 - upkeep;
      if (k.money < 0) k.money = 0;
      if (k.money > 6000) k.money = 6000;
    }
  }
  eventT += dt;
  if (eventT > 10) {
    eventT = 0;
    int r = rand() % 5, k = rand() % T;
    if (r == 0) cartels[k].money += 250;
    else if (r == 1) cartels[k].money = fmaxf(0, cartels[k].money - 150);
    else if (r == 2 && !npcs.empty()) npcs[rand() % npcs.size()].hp -= 40;
    else if (r == 3 && aliveCount(k) < maxArmy(k) - 2) {
      NPC n; n.team = k;
      int rc = rand() % 100;
      n.cls = rc < 35 ? CLS_LIGHT : rc < 60 ? CLS_HEAVY : rc < 75 ? CLS_SNIPER : CLS_MEDIC;
      n.wpn = n.cls == CLS_HEAVY ? W_AK : n.cls == CLS_SNIPER ? W_SNIPER : W_PISTOL;
      n.hp = clsMaxHp(n.cls) * (1 + 0.2f * cartels[k].upg);
      setRole(n);
      n.x = cartels[k].mx + rand() % 200 - 100;
      n.y = cartels[k].my + rand() % 200 - 100;
      n.wx = n.x; n.wy = n.y;
      npcs.push_back(n);
    } else if (r == 4) {
      float ax = rand() % (int)WORLD_W, ay = rand() % (int)WORLD_H;
      drops.push_back({ax, ay, W_AK, 30.f});
    }
  }
  respT += dt;
  if (respT > 3) {
    respT = 0;
    for (auto& k : cartels) {
      if (aliveCount(k.id) >= maxArmy(k.id) - 6 || terrCount(k.id) == 0) continue;
      if (k.money < 120) continue;
      k.money -= 120;
      std::vector<int> cells;
      for (int i = 0; i < G; i++) if (terr.owner[i] == k.id) cells.push_back(i);
      float sx = k.mx, sy = k.my;
      if (!cells.empty()) {
        int c = cells[rand() % cells.size()];
        sx = (c % terr.gw) * terr.cw + terr.cw / 2 + rand() % 160 - 80;
        sy = (c / terr.gw) * terr.ch + terr.ch / 2 + rand() % 160 - 80;
      }
      NPC n; n.team = k.id; n.x = sx; n.y = sy; n.wx = sx; n.wy = sy;
      int rc = rand() % 100;
      n.cls = rc < 35 ? CLS_LIGHT : rc < 60 ? CLS_HEAVY : rc < 75 ? CLS_SNIPER : CLS_MEDIC;
      n.wpn = n.cls == CLS_HEAVY ? W_AK : n.cls == CLS_SNIPER ? W_SNIPER : W_PISTOL;
      n.hp = clsMaxHp(n.cls) * (1 + 0.2f * k.upg);
      setRole(n);
      npcs.push_back(n);
    }
  }
  baseAI(dt);
  saveT += dt;
  if (saveT > 30) { saveT = 0; saveBackup(); }
}
int World::armedCount(int team) const {
  int n = 0;
  for (auto& c : cars) if (!c.wreck && c.armed && c.team == team) n++;
  return n;
}
int World::hiluxCount() const {
  int n = 0;
  for (auto& c : cars) if (!c.wreck && c.model == CAR_HILUX) n++;
  return n;
}
int World::maxArmy(int team) const {
  const Cartel& k = cartels[team];
  int v = 12 + (int)(k.money / 300) + terrCount(team) / 4 + k.upg * 2;
  if (v < 12) v = 12;
  if (v > 30) v = 30;
  return v;
}
int World::maxBlindado(int team) const {
  const Cartel& k = cartels[team];
  int v = 1 + (int)(k.money / 1500) + k.upg / 2;
  if (v < 1) v = 1;
  if (v > 4) v = 4;
  return v;
}
int World::maxHilux(int team) const {
  const Cartel& k = cartels[team];
  int v = 2 + (int)(k.money / 1200);
  if (v < 2) v = 2;
  if (v > 5) v = 5;
  return v;
}
int World::squadSize(int team) const {
  const Cartel& k = cartels[team];
  if (k.money > 3500) return 4;
  if (k.money > 1800) return 3;
  return 2;
}
float World::prodInterval(int team) const {
  const Cartel& k = cartels[team];
  if (k.money > 3000) return 2.5f;
  if (k.money > 1500) return 3.5f;
  return 5.0f;
}
void World::buySquad(int team) {
  Cartel& k = cartels[team];
  int sz = squadSize(team);
  int cost = 200 * sz;
  if (k.money < cost || aliveCount(team) + sz > maxArmy(team)) return;
  int med = 0, snp = 0;
  for (auto& o : npcs) if (o.alive && o.team == team) {
    if (o.cls == CLS_MEDIC) med++;
    if (o.cls == CLS_SNIPER) snp++;
  }
  float sx = k.mx, sy = k.my;
  if (k.backupT > 0) {
    for (auto& o : npcs) {
      if (!o.alive || o.team != team) continue;
      if (o.state == NST_ATTACK || o.state == NST_CHASE || o.state == NST_FLEE) {
        int c = terr.cellAt(o.x, o.y);
        if (c >= 0 && terr.owner[c] == team) { sx = o.x; sy = o.y; break; }
        float bx = o.x, by = o.y;
        float best = 1e9;
        for (int i = 0; i < (int)terr.owner.size(); i++) {
          if (terr.owner[i] != team) continue;
          float cx = (i % terr.gw) * terr.cw + terr.cw / 2;
          float cy = (i / terr.gw) * terr.ch + terr.ch / 2;
          float dx = cx - bx, dy = cy - by;
          float d = dx * dx + dy * dy;
          if (d < best) { best = d; sx = cx; sy = cy; }
        }
        break;
      }
    }
  }
  k.money -= cost;
  for (int i = 0; i < sz; i++) {
    NPC n; n.team = team;
    if (med == 0 && i == 0) n.cls = CLS_MEDIC;
    else if (snp == 0 && i == 1 && (rand() % 100) < 40) n.cls = CLS_SNIPER;
    else {
      int rc = rand() % 100;
      n.cls = rc < 40 ? CLS_LIGHT : rc < 70 ? CLS_HEAVY : (rc < 85 ? CLS_MEDIC : CLS_SNIPER);
    }
    if (n.cls == CLS_MEDIC) med++;
    if (n.cls == CLS_SNIPER) snp++;
    n.wpn = n.cls == CLS_HEAVY ? W_AK : n.cls == CLS_SNIPER ? W_SNIPER : W_PISTOL;
    n.hp = clsMaxHp(n.cls) * (1 + 0.2f * k.upg);
      setRole(n);
    n.x = sx + rand() % 200 - 100; n.y = sy + rand() % 200 - 100;
    n.wx = n.x; n.wy = n.y; n.alert = 3;
    npcs.push_back(n);
  }
}
bool World::buyBlindado(int team) {
  Cartel& k = cartels[team];
  if (k.money < 800 || armedCount(team) >= maxBlindado(team)) return false;
  int slot = -1;
  for (int i = 0; i < (int)cars.size(); i++)
    if (!cars[i].busy && !cars[i].wreck && !cars[i].armed && cars[i].model == CAR_NORMAL) { slot = i; break; }
  if (slot < 0 && (int)cars.size() < 30) {
    Car c; c.x = k.mx + 120; c.y = k.my;
    cars.push_back(c); slot = (int)cars.size() - 1;
  }
  if (slot < 0) return false;
  k.money -= 800;
  Car& c = cars[slot];
  c.armed = true; c.team = team; c.model = CAR_MONSTRO;
  c.maxHp = 700; c.hp = 700;
  c.x = k.mx + 120; c.y = k.my; c.vx = c.vy = 0; c.busy = false; c.tur = c.ang;
  return true;
}
bool World::buyHilux(int team) {
  Cartel& k = cartels[team];
  if (k.money < 350) return false;
  if (hiluxCount() >= maxHilux(team) * 2 || (int)cars.size() >= 30) return false;
  int own = 0;
  for (auto& c : cars) if (!c.wreck && c.model == CAR_HILUX) own++;
  if (own >= 6) return false;
  k.money -= 350;
  Car c; c.model = CAR_HILUX; c.maxHp = 300; c.hp = 300;
  c.x = k.mx + 90; c.y = k.my + 90; c.team = -1;
  cars.push_back(c);
  return true;
}
void World::buyUpg(int team) {
  Cartel& k = cartels[team];
  if (k.money < 600 || k.upg >= 3) return;
  k.money -= 600; k.upg++;
  for (auto& n : npcs)
    if (n.alive && n.team == team) n.hp = fminf(clsMaxHp(n.cls) * (1 + 0.2f * k.upg), n.hp + 25);
}
void World::baseAI(float dt) {
  for (auto& k : cartels) {
    k.baseT += dt; k.backupT -= dt;
    if (k.baseT < prodInterval(k.id)) continue;
    k.baseT = 0;
    int alive = aliveCount(k.id), terr = terrCount(k.id);
    int cap = maxArmy(k.id);
    int bc = maxBlindado(k.id);
    bool losing = alive < 9 || terr < 15 || k.backupT > 0;
    int sz = squadSize(k.id);
    int cost = 200 * sz;
    if (losing && k.money >= cost && alive < cap) { buySquad(k.id); continue; }
    if (k.money >= 900 && k.upg < 3 && alive >= 10) { buyUpg(k.id); continue; }
    if (k.money >= 800 && armedCount(k.id) < bc && alive >= 8 && buyBlindado(k.id)) continue;
    if (k.money >= 600 && k.upg < 3) { buyUpg(k.id); continue; }
    if (k.money >= 350 && alive < cap - 4 && alive >= 8 && buyHilux(k.id)) continue;
    if (k.money >= cost && alive < cap) buySquad(k.id);
  }
}
void World::saveBackup() const {
  FILE* f = fopen("backup.dat", "w");
  if (!f) return;
  for (auto& k : cartels) fprintf(f, "K %d %.0f %d\n", k.id, k.money, k.upg);
  fprintf(f, "T");
  for (int o : terr.owner) fprintf(f, " %d", o);
  fprintf(f, "\nN %zu\n", npcs.size());
  fclose(f);
}
void World::loadBackup() {
  FILE* f = fopen("backup.dat", "r");
  if (!f) return;
  int id, upg; float money;
  for (size_t i = 0; i < cartels.size(); i++) {
    if (fscanf(f, "K %d %f %d\n", &id, &money, &upg) != 3) break;
    if (id >= 0 && id < (int)cartels.size()) {
      cartels[id].money = money; cartels[id].upg = upg > 3 ? 3 : upg;
    }
  }
  char tag = 0;
  if (fscanf(f, "%c", &tag) == 1 && tag == 'T') {
    for (size_t i = 0; i < terr.owner.size(); i++) {
      int o = -1;
      if (fscanf(f, " %d", &o) != 1) break;
      terr.owner[i] = o;
    }
  }
  fclose(f);
}
void World::render(Renderer& r, Camera& c) {
  city.render(r, c);
  for (int cy = 0; cy < terr.gh; cy++)
    for (int cx = 0; cx < terr.gw; cx++) {
      int i = cy * terr.gw + cx;
      float x = cx * terr.cw, y = cy * terr.ch;
      SDL_Color col{80, 80, 80, 40};
      if (terr.owner[i] >= 0) {
        auto k = cartels[terr.owner[i]].color;
        col = {k.r, k.g, k.b, 70};
      }
      r.worldFill(x, y, terr.cw, terr.ch, col, c);
      r.worldRect(x, y, terr.cw, terr.ch, {0, 0, 0, 60}, c);
      if (terr.owner[i] >= 0 && ((cx + cy) & 1) == 0) {
        auto kc = cartels[terr.owner[i]].color;
        float gx = x + terr.cw * 0.7f, gy = y + terr.ch * 0.32f;
        r.worldFill(gx - 5, gy - 4, 11, 10, {kc.r, kc.g, kc.b, 45}, c);
        r.worldLine(gx - 6, gy + 4, gx - 1, gy - 5, {240, 240, 240, 200}, c);
        r.worldLine(gx - 1, gy - 5, gx + 3, gy + 1, {240, 240, 240, 200}, c);
        r.worldLine(gx + 3, gy + 1, gx + 7, gy - 4, {kc.r, kc.g, kc.b, 220}, c);
      }
    }
  for (auto& k : cartels) {
    // mansao de luxo: composta, teto decorado, jardim formal, piscina com borda, heliponto
    float wx0 = k.mx - 130, wy0 = k.my - 100, ww = 260, wh = 200;
    // calcada do terreno
    r.worldFill(wx0, wy0, ww, wh, {36, 38, 36, 255}, c);
    // jardim formal nos cantos
    for (int i = 0; i < 4; i++) {
      float jx = wx0 + 8 + (i % 2) * (ww - 58), jy = wy0 + 8 + (i / 2) * (wh - 58);
      r.worldFill(jx, jy, 50, 50, {45, 110, 55, 255}, c);
      r.worldFill(jx + 2, jy + 2, 46, 46, {60, 140, 70, 255}, c);
      r.worldFill(jx + 10, jy + 10, 30, 30, k.color, c);
      r.worldFill(jx + 22, jy + 22, 6, 6, {255, 220, 130, 255}, c);
    }
    // piscina com borda clara, quintal a direita do caminho do portao
    float px = wx0 + ww / 2 + 40, py = wy0 + wh - 50;
    r.worldFill(px - 3, py - 3, 58, 34, {230, 230, 235, 255}, c);
    r.worldFill(px - 2, py - 2, 56, 32, {120, 200, 240, 255}, c);
    r.worldFill(px + 8, py + 4, 40, 10, {90, 180, 230, 255}, c);
    r.worldFill(px - 2, py + 10, 56, 6, {140, 210, 245, 255}, c);
    // heliponto, canto esquerdo, entre os jardins
    float hx = wx0 + 36, hy = wy0 + wh / 2;
    r.worldFill(hx - 16, hy - 16, 32, 32, {50, 50, 56, 255}, c);
    r.worldRect(hx - 16, hy - 16, 32, 32, {220, 220, 220, 255}, c);
    r.worldFill(hx - 10, hy - 10, 20, 20, {60, 60, 66, 255}, c);
    r.worldFill(hx - 6, hy - 6, 12, 12, {230, 230, 235, 255}, c);
    r.worldFill(hx - 2, hy - 2, 4, 4, {25, 25, 30, 255}, c);
    r.worldFill(hx - 12, hy - 1, 9, 2, {235, 235, 235, 255}, c);
    r.worldFill(hx + 4, hy - 1, 9, 2, {235, 235, 235, 255}, c);
    // muro com pinacle no canto e na frente
    r.worldFill(wx0, wy0, ww, 8, k.color, c);
    r.worldFill(wx0, wy0 + wh - 8, ww, 8, k.color, c);
    r.worldFill(wx0, wy0, 8, wh, k.color, c);
    r.worldFill(wx0 + ww - 8, wy0, 8, wh, k.color, c);
    // portao central, mais largo
    r.worldFill(k.mx - 22, wy0 + wh - 8, 44, 8, {25, 25, 30, 255}, c);
    r.worldFill(k.mx - 22, wy0 + wh - 6, 44, 4, k.color, c);
    // pinacles nos cantos
    float pinele[4][2] = {{wx0 - 2, wy0 - 2}, {wx0 + ww - 6, wy0 - 2}, {wx0 - 2, wy0 + wh - 14}, {wx0 + ww - 6, wy0 + wh - 14}};
    for (int i = 0; i < 4; i++) {
      r.worldFill(pinele[i][0], pinele[i][1], 8, 8, {28, 30, 28, 255}, c);
      r.worldFill(pinele[i][0] + 2, pinele[i][1] + 2, 4, 4, k.color, c);
    }
    // casa: fundacao + parede com sombreamento
    float hW = k.mx - 55, hH = k.my - 50, hWW = 110, hWH = 70;
    r.worldFill(hW, hH, hWW, hWH, {48, 42, 54, 255}, c);
    r.worldFill(hW + 2, hH + 2, hWW - 4, hWH - 4, {62, 55, 68, 255}, c);
    // cumeeira com recorte central
    r.worldFill(hW + 4, hH + 3, hWW - 8, 4, k.color, c);
    r.worldFill(hW + 4, hH + hWH - 7, hWW - 8, 4, k.color, c);
    r.worldFill(hW + hWW / 2 - 1.5f, hH + 4, 3, hWH - 8, k.color, c);
    // duas janelas laterais com pincelada noturna
    float jW = 14, jH = 10;
    r.worldFill(hW + 10, hH + hWH / 2 - jH / 2, jW, jH, {180, 220, 255, 255}, c);
    r.worldFill(hW + hWW - 10 - jW, hH + hWH / 2 - jH / 2, jW, jH, {180, 220, 255, 255}, c);
    r.worldFill(hW + 12, hH + hWH / 2 - jH / 2 + 2, jW - 4, jH - 4, {90, 140, 190, 255}, c);
    r.worldFill(hW + hWW - 12 - jW, hH + hWH / 2 - jH / 2 + 2, jW - 4, jH - 4, {90, 140, 190, 255}, c);
    // porta principal elevada
    r.worldFill(k.mx - 11, hH + hWH - 6, 22, 6, {30, 25, 32, 255}, c);
    r.worldFill(k.mx - 9, hH + hWH - 5, 18, 4, k.color, c);
    // torres de vigilancia nas quinas internas
    float tx[4] = {wx0 + 18, wx0 + ww - 30, wx0 + 18, wx0 + ww - 30};
    float ty[4] = {wy0 + 18, wy0 + 18, wy0 + wh - 30, wy0 + wh - 30};
    for (int i = 0; i < 4; i++) {
      r.worldFill(tx[i], ty[i], 12, 12, {40, 40, 46, 255}, c);
      r.worldFill(tx[i] + 2, ty[i] + 2, 8, 8, {55, 55, 62, 255}, c);
      r.worldFill(tx[i] + 4, ty[i] + 4, 4, 4, k.color, c);
    }
    // caminho do portao ate a casa
    r.worldFill(k.mx - 8, hH + hWH, 16, wy0 + wh - (hH + hWH), {90, 88, 96, 255}, c);
    r.worldFill(k.mx - 7, hH + hWH, 14, wy0 + wh - (hH + hWH), {110, 108, 116, 255}, c);
    // faixa separando quintal
    r.worldLine(wx0 + 10, k.my + 50, wx0 + ww - 10, k.my + 50, {70, 70, 78, 255}, c);
    for (int i = 0; i < k.upg; i++)
      r.worldFill(k.mx - 60 + i * 16, wy0 - 2, 12, 8, {255, 215, 0, 255}, c);
  }
  for (auto& d : drops) {
    r.worldFill(d.x - 6, d.y - 3, 12, 6, {20, 20, 20, 255}, c);
    SDL_Color dc = d.wpn == W_SNIPER ? SDL_Color{80, 200, 120, 255} : SDL_Color{150, 100, 40, 255};
    r.worldFill(d.x - 6, d.y - 3, 12, 3, dc, c);
  }
  // sangue no chao: cresce nos primeiros segundos, escurece, some
  for (auto& bl : blood) {
    float age = (90 - bl.t) / 90.f;
    if (age > 1) age = 1;
    float rr = bl.r * (1 + age * 0.6f) * 0.55f;   // engrossa devagar, nunca sai
    float a = bl.t > 20 ? 140 : bl.t / 20 * 140;   // desbota nos ultimos 20s
    Uint8 u = (Uint8)a;
    SDL_Color lb{140, 20, 28, u};
    // forma irregular: circulo central + pontos no raio
    r.worldFill(bl.x - rr, bl.y - rr, rr * 2, rr * 2, lb, c);
    int h = (int)(bl.x * 31 + bl.y * 17) % 4;
    for (int i = 0; i < 4; i++) {
      if (i == h) continue;
      float a2 = i * 1.5707f + (h * 0.3f);
      float px = bl.x + cosf(a2) * rr, py = bl.y + sinf(a2) * rr;
      r.worldFill(px - 1, py - 1, 2, 2, lb, c);
    }
  }
  for (auto& cp : corpses) {
    Uint8 a = (Uint8)(90 * (cp.t / 8.f) + 30);
    auto kc = cartels[cp.team].color;
    SDL_Color dc{kc.r, kc.g, kc.b, a};
    float s = clsSize(cp.cls);
    r.worldFill(cp.x - s / 2, cp.y - s / 2, s, s, dc, c);
    r.worldFill(cp.x - 6, cp.y - 6, 12, 12, SDL_Color{140, 20, 20, (Uint8)(a / 2)}, c);
  }
  for (auto& n : npcs) {
    if (!n.alive || n.carId >= 0) continue;
    auto col = cartels[n.team].color;
    float s = clsSize(n.cls);
    float step = sinf(n.anim * 6.28f);
    float bob = step * 1.6f;
    float vlen = sqrtf(n.vx * n.vx + n.vy * n.vy);
    float px = 0, py = 0, fx = 0, fy = 1;
    float ca = cosf(n.ang), sa = sinf(n.ang);
    fx = ca; fy = sa;
    px = -sa; py = ca;
    float ox = 0, oy = 0;
    if (vlen > 1) { ox = px * bob; oy = py * bob; }
    float bx = n.x + ox, by = n.y + oy;
    r.worldFill(bx - s / 2 - 1, by - s / 2 - 1, s + 2, s + 2, {0, 0, 0, 110}, c);
    float fo = step * s * 0.28f;
    r.worldFill(bx + px * s * 0.26f + fx * fo - 1.5f, by + py * s * 0.26f + fy * fo - 1.5f, 3, 3, {25, 25, 30, 255}, c);
    r.worldFill(bx - px * s * 0.26f - fx * fo - 1.5f, by - py * s * 0.26f - fy * fo - 1.5f, 3, 3, {25, 25, 30, 255}, c);
    SDL_Color body = col;
    if (n.hit > 0) body = {255, 255, 255, 255};
    else if (n.state == NST_FLEE) { body.r = body.r / 2; body.g = body.g / 2; body.b = body.b / 2; }
    r.worldFill(bx - s / 2, by - s / 2, s, s, body, c);
    r.worldRect(bx - s / 2, by - s / 2, s, s, {0, 0, 0, 255}, c);
    if (n.cls == CLS_HEAVY) {
      r.worldFill(bx + px * (s / 2 - 1) - 2, by + py * (s / 2 - 1) - 2, 4, 4, {30, 30, 35, 255}, c);
      r.worldFill(bx - px * (s / 2 - 1) - 2, by - py * (s / 2 - 1) - 2, 4, 4, {30, 30, 35, 255}, c);
    }
    r.worldFill(bx - 2.5f, by - 2.5f, 5, 5, {220, 180, 150, 255}, c);
    if (n.cls == CLS_SNIPER)
      r.worldRect(bx - 3.5f, by - 3.5f, 7, 7, {40, 70, 45, 255}, c);
    else if (n.cls == CLS_LIGHT)
      r.worldLine(bx - px * 2.5f - fx * 1.5f, by - py * 2.5f - fy * 1.5f,
                  bx + px * 2.5f - fx * 1.5f, by + py * 2.5f - fy * 1.5f, {240, 240, 240, 255}, c);
    else
      r.worldLine(bx - px * 2.5f - fx * 2.5f, by - py * 2.5f - fy * 2.5f,
                  bx + px * 2.5f - fx * 2.5f, by + py * 2.5f - fy * 2.5f, {60, 45, 35, 255}, c);
    if (cartels[n.team].upg > 0)
      r.worldFill(bx - s / 2, by + s / 2 - 2, s, 2, {255, 215, 0, 255}, c);
    if (n.role == ROLE_GUARD)
      r.worldLine(bx - 6, by + s / 2 + 3, bx + 6, by + s / 2 + 3, {235, 235, 235, 220}, c);
    float gx = bx + ca * (s / 2), gy = by + sa * (s / 2);
    float gl = n.wpn == W_SNIPER ? 16 : n.wpn == W_AK ? 10 : 7;
    r.worldLine(gx, gy, gx + ca * gl, gy + sa * gl, {20, 20, 20, 255}, c);
    if (n.muz > 0) {
      float fs = wpnFlash(n.wpn);
      float fx = gx + ca * gl, fy = gy + sa * gl;
      r.worldFill(fx - fs / 2, fy - fs / 2, fs, fs, {255, 220, 100, 255}, c);
      r.worldFill(fx - fs / 4, fy - fs / 4, fs / 2, fs / 2, {255, 255, 230, 255}, c);
      r.worldLine(fx - ca * 4 - sa * fs / 2, fy - sa * 4 + ca * fs / 2,
                  fx - ca * 4 + sa * fs / 2, fy - sa * 4 - ca * fs / 2, {255, 180, 60, 255}, c);
    }
    if (n.cls == CLS_MEDIC) {
      r.worldFill(bx - 1, by - 4, 2, 8, {255, 255, 255, 255}, c);
      r.worldFill(bx - 4, by - 1, 8, 2, {255, 255, 255, 255}, c);
    }
    if (n.cls == CLS_HEAVY)
      r.worldRect(bx - s / 2 + 2, by - s / 2 + 2, s - 4, s - 4, {0, 0, 0, 120}, c);
    float mh = clsMaxHp(n.cls);
    float w = 14 * (n.hp / mh);
    if (w < 0) w = 0;
    r.worldFill(n.x - 7, n.y - s / 2 - 7, 14, 3, {0, 0, 0, 200}, c);
    SDL_Color hc = n.hp > mh * 0.6f ? SDL_Color{60, 220, 60, 255} : SDL_Color{220, 60, 60, 255};
    r.worldFill(n.x - 7, n.y - s / 2 - 7, w, 3, hc, c);
    if (n.state == NST_CHAT) {
      float bob2 = sinf(n.anim * 12.0f) * 1.5f;
      r.worldFill(n.x - 7, n.y - s / 2 - 17 + bob2, 14, 9, {240, 240, 240, 230}, c);
      r.worldRect(n.x - 7, n.y - s / 2 - 17 + bob2, 14, 9, {0, 0, 0, 255}, c);
      r.worldFill(n.x - 4, n.y - s / 2 - 14 + bob2, 2, 2, {20, 20, 20, 255}, c);
      r.worldFill(n.x - 1, n.y - s / 2 - 14 + bob2, 2, 2, {20, 20, 20, 255}, c);
      r.worldFill(n.x + 2, n.y - s / 2 - 14 + bob2, 2, 2, {20, 20, 20, 255}, c);
    }
  }
  for (auto& tr : tracers)
    r.worldLine(tr.x0, tr.y0, tr.x1, tr.y1, {255, 200, 120, 220}, c);
  for (auto& b : bullets) {
    float bl = sqrtf(b.vx * b.vx + b.vy * b.vy);
    float nx = bl > 1 ? b.vx / bl : 1, ny = bl > 1 ? b.vy / bl : 0;
    float tl = b.wpn == W_SNIPER ? 22 : b.wpn == W_AK ? 14 : 10;
    SDL_Color bc = b.wpn == W_SNIPER ? SDL_Color{180, 255, 200, 255} : SDL_Color{255, 240, 170, 255};
    r.worldLine(b.x - nx * tl, b.y - ny * tl, b.x, b.y, bc, c);
    r.worldFill(b.x - 1.5f, b.y - 1.5f, 3, 3, {255, 255, 255, 255}, c);
  }
  for (auto& sp2 : sparks)
    r.worldFill(sp2.x - 1.5f, sp2.y - 1.5f, 3, 3, {255, 220, 130, 255}, c);
  for (auto& car : cars) {
    SDL_Color bc = car.wreck ? SDL_Color{30, 30, 30, 220}
      : car.traffic ? car.paint
      : car.team >= 0 ? cartels[car.team].color : SDL_Color{150, 150, 155, 255};
    float cw = car.model == CAR_MONSTRO ? 44 : car.model == CAR_HILUX ? 36 : car.model == CAR_CIV ? 34 : car.armed ? 36 : 30;
    float ch = car.model == CAR_MONSTRO ? 26 : car.model == CAR_HILUX ? 18 : car.model == CAR_CIV ? 17 : car.armed ? 20 : 16;
    float ca = cosf(car.ang), sa = sinf(car.ang);
    float px = -sa, py = ca;
    for (float i = -ch / 2; i <= ch / 2; i += 2) {
      float mx = car.x + px * i, my = car.y + py * i;
      r.worldLine(mx - ca * cw / 2, my - sa * cw / 2, mx + ca * cw / 2, my + sa * cw / 2,
        car.wreck ? SDL_Color{30, 30, 30, 220} : SDL_Color{(Uint8)(bc.r / 3), (Uint8)(bc.g / 3), (Uint8)(bc.b / 3), 255}, c);
    }
    for (float i = -ch / 2 + 1; i <= ch / 2 - 1; i += 1) {
      float mx = car.x + px * i, my = car.y + py * i;
      r.worldLine(mx - ca * (cw / 2 - 3), my - sa * (cw / 2 - 3), mx + ca * (cw / 2 - 3), my + sa * (cw / 2 - 3), bc, c);
    }
    float fx = car.x + ca * cw / 2, fy = car.y + sa * cw / 2;
    float bx = car.x - ca * cw / 2, by = car.y - sa * cw / 2;
    r.worldLine(fx + px * ch / 2, fy + py * ch / 2, fx - px * ch / 2, fy - py * ch / 2, {255, 240, 180, 255}, c);
    r.worldLine(bx + px * ch / 2, by + py * ch / 2, bx - px * ch / 2, by - py * ch / 2, {180, 40, 40, 255}, c);
    r.worldFill(car.x - 5, car.y - 5, 10, 10, {120, 180, 220, 220}, c);
    float ta = car.armed ? car.tur : car.ang;
    float tc = cosf(ta), ts = sinf(ta);
    r.worldLine(car.x, car.y, car.x + tc * (car.armed ? 26 : 18), car.y + ts * (car.armed ? 26 : 18), {20, 20, 20, 255}, c);
    if (car.armed) {
      r.worldLine(car.x, car.y, car.x + tc * 26, car.y + ts * 26, {255, 215, 0, 255}, c);
      r.worldRect(car.x - 3, car.y - 3, 6, 6, {0, 0, 0, 255}, c);
    }
    if (car.model == CAR_HILUX && !car.wreck) {
      float bedx = car.x - ca * cw / 4, bedy = car.y - sa * cw / 4;
      r.worldLine(bedx + px * (ch / 2 - 3), bedy + py * (ch / 2 - 3),
                  bedx - px * (ch / 2 - 3), bedy - py * (ch / 2 - 3), {0, 0, 0, 160}, c);
      float cabx = car.x + ca * cw * 0.12f, baby = car.y + sa * cw * 0.12f;
      r.worldLine(cabx + px * (ch / 2 - 2), baby + py * (ch / 2 - 2),
                  cabx - px * (ch / 2 - 2), baby - py * (ch / 2 - 2), {235, 235, 235, 255}, c);
      float slx = car.x - ca * cw * 0.32f, sly = car.y - sa * cw * 0.32f;
      r.worldLine(slx + px * (ch / 2 - 2), sly + py * (ch / 2 - 2),
                  slx - px * (ch / 2 - 2), sly - py * (ch / 2 - 2), {0, 0, 0, 160}, c);
      r.worldLine(car.x - ca * cw / 2 + px * (ch / 2 - 1), car.y - sa * cw / 2 + py * (ch / 2 - 1),
                  car.x + ca * cw / 2 + px * (ch / 2 - 1), car.y + sa * cw / 2 + py * (ch / 2 - 1), {0, 0, 0, 120}, c);
      r.worldLine(car.x - ca * cw / 2 - px * (ch / 2 - 1), car.y - sa * cw / 2 - py * (ch / 2 - 1),
                  car.x + ca * cw / 2 - px * (ch / 2 - 1), car.y + sa * cw / 2 - py * (ch / 2 - 1), {0, 0, 0, 120}, c);
    }
    if (car.model == CAR_CIV && !car.wreck) {
      float roofx = car.x - ca * 2, roofy = car.y - sa * 2;
      r.worldLine(roofx + px * (ch / 2 - 3), roofy + py * (ch / 2 - 3),
                  roofx - px * (ch / 2 - 3), roofy - py * (ch / 2 - 3), {30, 30, 35, 255}, c);
      float wlx = car.x + ca * cw * 0.22f, wly = car.y + sa * cw * 0.22f;
      r.worldLine(wlx + px * (ch / 2 - 2), wly + py * (ch / 2 - 2),
                  wlx - px * (ch / 2 - 2), wly - py * (ch / 2 - 2), {150, 200, 230, 255}, c);
      float rwx = car.x - ca * cw * 0.28f, rwy = car.y - sa * cw * 0.28f;
      r.worldLine(rwx + px * (ch / 2 - 2), rwy + py * (ch / 2 - 2),
                  rwx - px * (ch / 2 - 2), rwy - py * (ch / 2 - 2), {150, 200, 230, 255}, c);
    }
    if (car.model == CAR_MONSTRO && !car.wreck) {
      for (int sgi = -1; sgi <= 1; sgi += 2)
        for (int sgj = -1; sgj <= 1; sgj += 2) {
          float txp = car.x + ca * sgi * (cw / 2 - 1) + px * sgj * (ch / 2 + 2);
          float typ = car.y + sa * sgi * (cw / 2 - 1) + py * sgj * (ch / 2 + 2);
          r.worldFill(txp - 5, typ - 4, 10, 8, {12, 12, 12, 255}, c);
          r.worldFill(txp - 2, typ - 2, 4, 4, {90, 90, 95, 255}, c);
        }
      for (int sgi = -1; sgi <= 1; sgi += 2) {
        float ox = px * sgi * (ch / 2 - 3), oy = py * sgi * (ch / 2 - 3);
        r.worldLine(car.x + ox - ca * (cw / 2 - 5), car.y + oy - sa * (cw / 2 - 5),
                    car.x + ox + ca * (cw / 2 - 5), car.y + oy + sa * (cw / 2 - 5), {0, 0, 0, 200}, c);
      }
      float tipx = fx + ca * 10, tipy = fy + sa * 10;
      r.worldLine(fx + px * (ch / 2 - 1), fy + py * (ch / 2 - 1), tipx, tipy, {185, 185, 190, 255}, c);
      r.worldLine(fx - px * (ch / 2 - 1), fy - py * (ch / 2 - 1), tipx, tipy, {185, 185, 190, 255}, c);
      r.worldLine(fx + px * (ch / 2 - 1), fy + py * (ch / 2 - 1), fx - px * (ch / 2 - 1), fy - py * (ch / 2 - 1), {70, 70, 75, 255}, c);
      for (int gi = 0; gi < 3; gi++) {
        float goff = (gi - 1) * (ch / 5);
        r.worldLine(fx - ca * 2 + px * goff, fy - sa * 2 + py * goff,
                    fx + ca * 3 + px * goff, fy + sa * 3 + py * goff, {20, 20, 20, 255}, c);
      }
      float turx = car.x - ca * cw * 0.08f, tury = car.y - sa * cw * 0.08f;
      r.worldRect(turx - 7, tury - 7, 14, 14, {8, 8, 8, 255}, c);
      r.worldLine(turx - 7, tury - 7, turx + 7, tury - 7, {8, 8, 8, 255}, c);
      r.worldLine(turx - 7, tury + 7, turx + 7, tury + 7, {8, 8, 8, 255}, c);
      float hx = turx + ca * 2, hy = tury + sa * 2;
      r.worldFill(hx - 2, hy - 2, 4, 4, {150, 200, 230, 255}, c);
      float gx = turx - tc * 6 + ts * 5, gy = tury - ts * 6 - tc * 5;
      r.worldLine(gx, gy, gx + tc * 3 - ts * 6, gy + ts * 3 + tc * 6, {230, 230, 230, 255}, c);
      float gx2 = turx - tc * 10 - ts * 5, gy2 = tury - ts * 10 + tc * 5;
      r.worldLine(gx2, gy2, gx2 + tc * 3 + ts * 6, gy2 + ts * 3 - tc * 6, {230, 230, 230, 255}, c);
      float spx = bx - ca * 2, spy = by - sa * 2;
      r.worldRect(spx - 5, spy - 5, 10, 10, {10, 10, 10, 255}, c);
      r.worldFill(spx - 2, spy - 2, 4, 4, {100, 100, 105, 255}, c);
    }
    if (!car.wreck) {
      float w = cw * (car.hp / car.maxHp);
      if (w < 0) w = 0;
      r.worldFill(car.x - cw / 2, car.y - ch / 2 - 6, cw, 3, {0, 0, 0, 200}, c);
      r.worldFill(car.x - cw / 2, car.y - ch / 2 - 6, w, 3, car.armed ? SDL_Color{255, 200, 40, 255} : SDL_Color{60, 200, 255, 255}, c);
    }
  }
}
void World::renderLights(Renderer& r, Camera& c) {
  city.renderLights(r, c);
  for (auto& k : cartels) {
    r.worldFill(k.mx - 8, k.my - 8, 16, 16, {255, 240, 160, 255}, c);
    // torres acesas a noite
    float txl[4] = {k.mx - 112, k.mx + 98, k.mx - 112, k.mx + 98};
    float tyl[4] = {k.my - 82, k.my - 82, k.my + 68, k.my + 68};
    for (int i = 0; i < 4; i++)
      r.worldFill(txl[i], tyl[i], 6, 6, {255, 230, 150, 255}, c);
    // piscina de noite, borda clara
    r.worldFill(k.mx - 22, k.my + 6, 44, 22, {100, 170, 220, 150}, c);
  }
  for (auto& n : npcs) {
    if (!n.alive || n.muz <= 0) continue;
    float s = wpnFlash(n.wpn) * 2.2f;
    r.worldFill(n.x - s / 2, n.y - s / 2, s, s, {255, 200, 90, 90}, c);
  }
  for (auto& b : bullets)
    r.worldFill(b.x - 5, b.y - 5, 10, 10, {255, 220, 130, 70}, c);
  for (auto& sp3 : sparks)
    r.worldFill(sp3.x - 3, sp3.y - 3, 6, 6, {255, 200, 110, 80}, c);
}
