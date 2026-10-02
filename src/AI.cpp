#include "AI.h"
#include "World.h"
#include "Weapon.h"
#include "Config.h"
#include <cmath>
#include <cstdlib>
static float dist(float ax, float ay, float bx, float by) {
  float dx = ax - bx, dy = ay - by;
  return sqrtf(dx * dx + dy * dy);
}
static bool segEdge(float x0, float y0, float x1, float y1,
                    float x2, float y2, float x3, float y3) {
  float d = (x1 - x0) * (y3 - y2) - (y1 - y0) * (x3 - x2);
  if (fabsf(d) < 1e-6f) return false;
  float t = ((x2 - x0) * (y3 - y2) - (y2 - y0) * (x3 - x2)) / d;
  float u = ((x2 - x0) * (y1 - y0) - (y2 - y0) * (x1 - x0)) / d;
  return t >= 0 && t <= 1 && u >= 0 && u <= 1;
}
static bool losClear(float x0, float y0, float x1, float y1, World& w) {
  for (auto& h : w.city.houses) {
    if (x1 > h.x && x1 < h.x + h.w && y1 > h.y && y1 < h.y + h.h) return false;
    if (segEdge(x0, y0, x1, y1, h.x, h.y, h.x + h.w, h.y)) return false;
    if (segEdge(x0, y0, x1, y1, h.x, h.y + h.h, h.x + h.w, h.y + h.h)) return false;
    if (segEdge(x0, y0, x1, y1, h.x, h.y, h.x, h.y + h.h)) return false;
    if (segEdge(x0, y0, x1, y1, h.x + h.w, h.y, h.x + h.w, h.y + h.h)) return false;
  }
  return true;
}
static float sightMul(float h) { return (h >= 18 || h < 5) ? 1.25f : h >= 12 ? 1.1f : 0.9f; }
static float dmgMul(float h) { return (h >= 18 || h < 5) ? 1.2f : h >= 12 ? 1.05f : 0.9f; }
static float chatP(float h) { return (h >= 18 || h < 5) ? 0.05f : h >= 12 ? 0.4f : 0.8f; }
static void invasionTarget(World& w, int team, float fx, float fy, float& ox, float& oy) {
  for (auto& o : w.npcs) {
    if (!o.alive || o.team != team) continue;
    if (o.state != NST_CAPTURE) continue;
    if (dist(fx, fy, o.wx, o.wy) < 1000 && (rand() % 100) < 60) {
      ox = o.wx + (rand() % 80 - 40); oy = o.wy + (rand() % 80 - 40);
      ox = fmaxf(0, fminf(WORLD_W, ox)); oy = fmaxf(0, fminf(WORLD_H, oy));
      return;
    }
  }
  int G = w.terr.gw * w.terr.gh;
  static int foeN[128], mateN[128];
  for (int i = 0; i < G; i++) foeN[i] = mateN[i] = 0;
  for (auto& o : w.npcs) {
    int c = w.terr.cellAt(o.x, o.y);
    if (c < 0) continue;
    if (o.team == team) mateN[c]++;
    else foeN[c]++;
  }
  int bi = -1; float bs = 1e18f;
  for (int i = 0; i < G; i++) {
    if (w.terr.owner[i] == team) continue;
    float cx = (i % w.terr.gw) * w.terr.cw + w.terr.cw / 2;
    float cy = (i / w.terr.gw) * w.terr.ch + w.terr.ch / 2;
    if (w.city.inRoad(cx - 40, cy - 40, 80, 80)) continue;
    float score = foeN[i] * 300.0f - mateN[i] * 120.0f + dist(fx, fy, cx, cy) / 4.0f - w.terr.prog[i] * 50.0f + (rand() % 120);
    if (score < bs) { bs = score; bi = i; }
  }
  if (bi < 0) { ox = fx + (rand() % 600 - 300); oy = fy + (rand() % 600 - 300); }
  else {
    ox = (bi % w.terr.gw) * w.terr.cw + w.terr.cw / 2 + (rand() % 120 - 60);
    oy = (bi / w.terr.gw) * w.terr.ch + w.terr.ch / 2 + (rand() % 120 - 60);
  }
  ox = fmaxf(0, fminf(WORLD_W, ox)); oy = fmaxf(0, fminf(WORLD_H, oy));
}
static void setPath(NPC& n, World& w) {
  n.qn = w.city.findPath(n.x, n.y, n.wx, n.wy, n.qx, n.qy);
  n.qi = 0;
}
static bool pathStep(NPC& n, World& w, float reach, float& tx, float& ty) {
  if (n.qn == 0) {
    if (n.prt > 0) { tx = n.wx; ty = n.wy; return false; }
    n.prt = 1.0f;
    setPath(n, w);
    if (n.qn == 0) { tx = n.wx; ty = n.wy; return false; }
  }
  tx = n.qx[n.qi]; ty = n.qy[n.qi];
  if (dist(n.x, n.y, tx, ty) < reach) {
    n.qi++;
    if (n.qi >= n.qn) { n.qn = 0; tx = n.wx; ty = n.wy; return false; }
    tx = n.qx[n.qi]; ty = n.qy[n.qi];
  }
  return true;
}
static float roomSide(World& w, float x, float y, float rx, float ry, float look) {
  for (float d = 12; d <= look; d += 12)
    if (w.city.blockedAt(x + rx * d, y + ry * d)) return d;
  return look;
}
static void probe(World& w, int& side, float x, float y, float& dx, float& dy, float look) {
  float l = sqrtf(dx * dx + dy * dy);
  if (l < 1) return;
  float nx = dx / l, ny = dy / l;
  // amostra dois pontos: parede rente nao escapa so pelo teste distante
  bool ahead = w.city.blockedAt(x + nx * look, y + ny * look) ||
    w.city.blockedAt(x + nx * look * 0.55f, y + ny * look * 0.55f);
  if (!ahead) return;
  float rx = -ny, ry = nx;
  float ld = roomSide(w, x, y, rx, ry, look);
  float rd = roomSide(w, x, y, -rx, -ry, look);
  // histerese: so troca de lado quando o outro tem folga clara, senao oscila
  float s = side > 0 ? 1.f : -1.f;
  if (ld > rd * 1.3f) s = 1;
  else if (rd > ld * 1.3f) s = -1;
  side = s > 0 ? 1 : -1;
  dx = nx * 0.25f + rx * s * 0.95f;
  dy = ny * 0.25f + ry * s * 0.95f;
}
static int carCrew(World& w, int ci, NPC* self = nullptr) {
  int n = 0;
  for (auto& o : w.npcs) {
    if (!o.alive || &o == self) continue;
    if (o.carId == ci) n++;
  }
  return n;
}
static bool carHasDriver(World& w, int ci, NPC* self = nullptr) {
  for (auto& o : w.npcs) {
    if (!o.alive || &o == self) continue;
    if (o.carId == ci && o.state == NST_DRIVE) return true;
  }
  return false;
}
static float foeNear(World& w, float x, float y, int team, float range) {
  float b = range;
  for (auto& o : w.npcs) {
    if (!o.alive || o.team == team) continue;
    float dx = o.x - x, dy = o.y - y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d < b) b = d;
  }
  return b;
}
static bool fire(NPC& n, NPC& f, World& w) {
  if (n.cool > 0 || n.burstT > 0) return true;
  if (n.carId >= 0 && n.state == NST_DRIVE) return true;
  float d = dist(n.x, n.y, f.x, f.y);
  if (d > wpnRange(n.wpn)) return true;
  if (!losClear(n.x, n.y, f.x, f.y, w)) { n.cool = 0.15f; return false; }
  n.cool = wpnCool(n.wpn);
  float base = atan2f(f.y - n.y, f.x - n.x);
  n.ang = base;
  float mv = sqrtf(n.vx * n.vx + n.vy * n.vy);
  float sp = wpnSpread(n.wpn) + (mv > 120 ? 0.05f : mv > 40 ? 0.02f : 0.0f) + d / wpnRange(n.wpn) * 0.02f;
  float a = base + ((rand() % 1000) / 1000.0f - 0.5f) * 2 * sp;
  float dmg = (wpnDmg(n.wpn) + rand() % 8) * (1 + 0.2f * w.cartels[n.team].upg) * dmgMul(w.hour);
  if (n.wpn != W_SNIPER && d > wpnRange(n.wpn) * 0.7f) dmg *= 0.6f;
  float bs = wpnSpeed(n.wpn);
  float gl = n.wpn == W_SNIPER ? 16 : n.wpn == W_AK ? 10 : 7;
  // sem clamp, o births nasce alem do alvo quando ele esta em cima de nos
  float off = fminf(clsSize(n.cls) / 2 + gl, d * 0.9f);
  float mx = n.x + cosf(base) * off;
  float my = n.y + sinf(base) * off;
  w.bullets.push_back({mx, my, cosf(a) * bs, sinf(a) * bs, wpnRange(n.wpn) / bs + 0.1f, dmg, n.team, n.wpn});
  w.sounds.push_back({n.wpn, n.x, n.y});
  n.muz = n.wpn == W_SNIPER ? 0.12f : 0.06f;
  if (n.wpn == W_AK) {
    n.burst++;
    if (n.burst >= 3) { n.burst = 0; n.burstT = 0.8f; }
  }
  if (n.wpn == W_SNIPER) n.ret = 1.2f;
  for (auto& aa : w.npcs) {
    if (!aa.alive || aa.team != n.team || &aa == &n) continue;
    if (dist(aa.x, aa.y, n.x, n.y) < 600) aa.alert = 2.5f;
  }
  return true;
}
void aiUpdate(NPC& n, World& w, float dt) {
  if (!n.alive) return;
  n.cool -= dt; n.ret -= dt; n.burstT -= dt; n.alert -= dt; n.hit -= dt; n.call -= dt; n.muz -= dt; n.chat -= dt; n.prt -= dt; n.fleeLock -= dt; n.et -= dt;
  if (n.carId >= 0 && n.carId < (int)w.cars.size()) {
    Car& c = w.cars[n.carId];
    if (c.wreck) {
      int crew = carCrew(w, n.carId, &n);
      if (crew == 0) { c.busy = false; }
      n.carId = -1; n.state = NST_PATROL; n.x += 24; n.vx = 120;
      return;
    }
    if (n.state != NST_DRIVE) {
      if (!carHasDriver(w, n.carId, &n) && !c.wreck) {
        n.state = NST_DRIVE; c.busy = true; c.team = n.team; c.t = 0; n.qn = 0; n.prt = 0;
        return;
      }
      n.x = c.x; n.y = c.y; n.vx = c.vx; n.vy = c.vy;
      float seeP = wpnRange(n.wpn) * sightMul(w.hour);
      NPC* foeP = nullptr; float bdP = seeP;
      for (auto& o : w.npcs) {
        if (!o.alive || o.team == n.team) continue;
        float dP = dist(n.x, n.y, o.x, o.y);
        if (dP < bdP && losClear(c.x, c.y, o.x, o.y, w)) { bdP = dP; foeP = &o; }
      }
      if (foeP) { n.ang = atan2f(foeP->y - n.y, foeP->x - n.x); fire(n, *foeP, w); }
      else n.ang = c.ang;
      return;
    } else {
      n.x = c.x; n.y = c.y; n.vx = c.vx; n.vy = c.vy; n.ang = c.ang;
      float tx = n.wx, ty = n.wy;
      float d = dist(c.x, c.y, tx, ty);
      int cell = w.terr.cellAt(c.x, c.y);
      bool enemy = cell >= 0 && w.terr.owner[cell] != n.team && w.terr.owner[cell] != -1;
      float dH = dist(c.x, c.y, w.cartels[n.team].mx, w.cartels[n.team].my);
      if (dH < 220 && c.hp < c.maxHp) c.hp = fminf(c.maxHp, c.hp + 25 * dt);
      NPC* tgt = nullptr; float bt = 380;
      if (c.armed) {
        c.gun -= dt;
        for (auto& o : w.npcs) {
          if (!o.alive || o.team == n.team || o.carId >= 0) continue;
          float od = dist(c.x, c.y, o.x, o.y);
          if (od < bt) { bt = od; tgt = &o; }
        }
        if (tgt && c.gun <= 0) {
          if (!losClear(c.x, c.y, tgt->x, tgt->y, w)) { c.gun = 0.2f; }
          else {
          c.gun = 0.5f;
          c.tur = atan2f(tgt->y - c.y, tgt->x - c.x);
          float ga = c.tur + ((rand() % 1000) / 1000.0f - 0.5f) * 0.1f;
          w.bullets.push_back({c.x + cosf(c.tur) * 24, c.y + sinf(c.tur) * 24,
            cosf(ga) * 1200, sinf(ga) * 1200, 0.4f, 18 * (1 + 0.2f * w.cartels[n.team].upg) * dmgMul(w.hour), n.team, W_AK});
          w.sounds.push_back({3, c.x, c.y});
          n.muz = 0.08f;
          }
        }
        if (c.hp < c.maxHp * 0.35f) { tx = w.cartels[n.team].mx; ty = w.cartels[n.team].my; d = dist(c.x, c.y, tx, ty); }
        else if (tgt && bt < 180) {
          float a = atan2f(c.y - tgt->y, c.x - tgt->x);
          tx = c.x + cosf(a) * 220; ty = c.y + sinf(a) * 220; d = 220;
        } else if (tgt && bt < 300) {
          float a = atan2f(c.y - tgt->y, c.x - tgt->x) + 1.57f * (n.wallSide > 0 ? 1 : -1);
          tx = c.x + cosf(a) * 180; ty = c.y + sinf(a) * 180; d = 180;
        } else if (tgt && bt < 380) { tx = tgt->x; ty = tgt->y; d = dist(c.x, c.y, tx, ty); }
      }
      float fn = foeNear(w, c.x, c.y, n.team, 120);
      bool wantExit = (d < 90 || c.hp < 40 || (enemy && !c.armed));
      if (wantExit && fn < 120 && c.hp > 40) {
        wantExit = false;
        tx = w.cartels[n.team].mx; ty = w.cartels[n.team].my; d = dist(c.x, c.y, tx, ty);
      }
      if (wantExit) {
        int crew = carCrew(w, n.carId, &n);
        if (crew == 0) { c.busy = false; if (!c.armed) c.team = -1; }
        c.vx = c.vy = 0;
        n.carId = -1; n.state = NST_PATROL; n.ret = 0; n.cx = 0; n.cy = 0; n.qn = 0; n.prt = 0;
        float ox[4] = {24, -24, 0, 0}, oy[4] = {0, 0, 24, -24};
        n.x = c.x + 24; n.y = c.y;
        for (int i = 0; i < 4; i++) {
          float ex = c.x + ox[i], ey = c.y + oy[i];
          if (!w.city.blockedAt(ex, ey)) { n.x = ex; n.y = ey; break; }
        }
        n.vx = n.vy = 0;
        return;
      }
      float wx = tx, wy = ty;
      float moved = dist(n.cx, n.cy, tx, ty);
      if ((n.qn == 0 && n.prt <= 0) || moved > 200) {
        n.prt = 0.4f; n.qn = w.city.findPath(c.x, c.y, tx, ty, n.qx, n.qy); n.qi = 0; n.cx = tx; n.cy = ty;
      }
      if (n.qi < n.qn) {
        wx = n.qx[n.qi]; wy = n.qy[n.qi];
        if (dist(c.x, c.y, wx, wy) < 90) { n.qi++; if (n.qi >= n.qn) { n.qn = 0; wx = tx; wy = ty; } }
      }
      float dx = wx - c.x, dy = wy - c.y;
      probe(w, n.wallSide, c.x, c.y, dx, dy, 110);
      {
        float l = sqrtf(dx * dx + dy * dy);
        if (l > 1) {
          float nx = dx / l, ny = dy / l;
          float px = -ny, py = nx;
          bool left = w.city.blockedAt(c.x + nx * 70 + px * 18, c.y + ny * 70 + py * 18);
          bool right = w.city.blockedAt(c.x + nx * 70 - px * 18, c.y + ny * 70 - py * 18);
          if (left && !right) { dx += -px * 60; dy += -py * 60; }
          else if (right && !left) { dx += px * 60; dy += py * 60; }
        }
      }
      float dl = sqrtf(dx * dx + dy * dy);
      float base = w.city.onRoad(c.x, c.y) ? 340 : 220;
      float spd = c.model == CAR_HILUX ? base + 50 : c.model == CAR_MONSTRO ? (w.city.onRoad(c.x, c.y) ? 300 : 245) : base;
      for (auto& o : w.cars) {
        if (&o == &c || o.wreck || dl < 1) continue;
        if (!o.traffic && !o.armed) continue;
        float ox = o.x - c.x, oy = o.y - c.y;
        float ah = (dx * ox + dy * oy) / dl;
        float d2 = ox * ox + oy * oy;
        if (ah > 0 && ah < 110 && d2 < 14400) { spd = fminf(spd, 70); break; }
      }
      float k = fminf(1, dt * 3);
      float dvx = dl > 1 ? dx / dl * spd : 0, dvy = dl > 1 ? dy / dl * spd : 0;
      c.vx += (dvx - c.vx) * k; c.vy += (dvy - c.vy) * k;
      for (auto& h : w.city.houses) {
        float r = 18;
        if (c.x > h.x - r && c.x < h.x + h.w + r && c.y > h.y - r && c.y < h.y + h.h + r) {
          float l = c.x - (h.x - r), rr = (h.x + h.w + r) - c.x;
          float u = c.y - (h.y - r), dd = (h.y + h.h + r) - c.y;
          float m = fminf(fminf(l, rr), fminf(u, dd));
          if (m == l) { c.x = h.x - r; c.vx = fminf(0, c.vx); }
          else if (m == rr) { c.x = h.x + h.w + r; c.vx = fmaxf(0, c.vx); }
          else if (m == u) { c.y = h.y - r; c.vy = fminf(0, c.vy); }
          else { c.y = h.y + h.h + r; c.vy = fmaxf(0, c.vy); }
        }
      }
      float csp = sqrtf(c.vx * c.vx + c.vy * c.vy);
      if (csp < 40 && dl > 150) c.stuck += dt;
      else c.stuck = fmaxf(0, c.stuck - dt * 2);
      if (c.stuck > 0.6f) {
        c.stuck = 0;
        c.vx = -c.vx * 0.8f + (rand() % 120 - 60);
        c.vy = -c.vy * 0.8f + (rand() % 120 - 60);
        n.qn = 0; n.prt = 0;
        c.t += 1;
        if (c.t >= 5) {
          int crew = carCrew(w, n.carId, &n);
          if (crew == 0) { c.busy = false; if (!c.armed) c.team = -1; }
          c.t = 0; c.vx = c.vy = 0;
          n.carId = -1; n.state = NST_PATROL; n.ret = 0; n.cx = 0; n.cy = 0;
          n.x = c.x + 24; n.y = c.y; n.vx = n.vy = 0;
          return;
        }
      }
      n.x = c.x; n.y = c.y; n.vx = c.vx; n.vy = c.vy; n.ang = c.ang;
      if (!c.armed) c.tur = c.ang;
      return;
    }
  }
  float maxHp = clsMaxHp(n.cls);
  float see = wpnRange(n.wpn) * sightMul(w.hour);
  NPC* foe = nullptr;
  float bs = -1e9;
  float bd = see;
  {
    NPC* cand[6]; float cd[6]; float cs[6]; int cn = 0;
    for (auto& o : w.npcs) {
      if (!o.alive || o.team == n.team) continue;
      float d = dist(n.x, n.y, o.x, o.y);
      bool flash = o.muz > 0 && (w.hour >= 18 || w.hour < 5) && d < 700;
      if (d > see && !flash) continue;
      float threat = -d + (clsMaxHp(o.cls) - o.hp) * 1.5f;
      if (o.cls == CLS_MEDIC) threat += 60;
      if (o.cls == CLS_SNIPER) threat += 40;
      if (o.hp < clsMaxHp(o.cls) * 0.4f) threat += 50;
      int sup = 0;
      for (auto& a : w.npcs) {
        if (!a.alive || a.team != n.team || &a == &n) continue;
        if (dist(a.x, a.y, o.x, o.y) < 350) { sup++; if (sup >= 3) break; }
      }
      threat += sup * 40;
      if (flash) threat += 200;
      int pos = cn < 6 ? cn++ : -1;
      if (cn >= 6) {
        int wi = 0;
        for (int i = 1; i < 6; i++) if (cs[i] < cs[wi]) wi = i;
        if (threat > cs[wi]) pos = wi; else pos = -1;
      }
      if (pos >= 0) { cand[pos] = &o; cd[pos] = d; cs[pos] = threat; }
      if (cn > 6) cn = 6;
    }
    for (int i = 0; i < cn; i++) {
      if (!cand[i]) continue;
      bool flash = cand[i]->muz > 0 && (w.hour >= 18 || w.hour < 5);
      bool vis = flash || losClear(n.x, n.y, cand[i]->x, cand[i]->y, w);
      if (!vis) continue;
      float t = cs[i] + 300;
      if (t > bs) { bs = t; foe = cand[i]; bd = cd[i]; }
    }
    if (foe) { n.ex = foe->x; n.ey = foe->y; n.et = 3; }
  }
  bool hasMem = !foe && n.et > 0;
  if (!foe && n.alert > 0) {
    for (auto& o : w.npcs) {
      if (!o.alive || o.team != n.team) continue;
      if (o.state == NST_ATTACK || o.state == NST_CHASE) {
        for (auto& e : w.npcs) {
          if (!e.alive || e.team == n.team) continue;
          if (dist(o.x, o.y, e.x, e.y) < see && dist(n.x, n.y, e.x, e.y) < 700) {
            foe = &e; bd = dist(n.x, n.y, e.x, e.y); break;
          }
        }
      }
      if (foe) break;
    }
    if (foe) { n.ex = foe->x; n.ey = foe->y; n.et = 3; hasMem = false; }
  }
  NPC* wounded = nullptr;
  if (n.cls == CLS_MEDIC) {
    float bw = 500;
    for (auto& o : w.npcs) {
      if (!o.alive || o.team != n.team || &o == &n) continue;
      if (o.hp < clsMaxHp(o.cls) * 0.6f) {
        float d = dist(n.x, n.y, o.x, o.y);
        if (d < bw) { bw = d; wounded = &o; }
      }
    }
    if (!wounded) {
      for (auto& o : w.npcs) {
        if (!o.alive || o.team != n.team || &o == &n) continue;
        float d = dist(n.x, n.y, o.x, o.y);
        if (d < 200) { wounded = &o; break; }
      }
    }
  }
  Cartel& home = w.cartels[n.team];
  float dHome = dist(n.x, n.y, home.mx, home.my);
  if (n.call <= 0 && foe) {
    int ally = 0, enemy = 0;
    for (auto& o : w.npcs) {
      if (!o.alive) continue;
      float d = dist(n.x, n.y, o.x, o.y);
      if (d > 450) continue;
      if (o.team == n.team) ally++;
      else enemy++;
    }
    if (n.hp < maxHp * 0.45f || enemy > ally) {
      n.call = 8;
      home.backupT = 6;
      for (auto& a : w.npcs) {
        if (!a.alive || a.team != n.team) continue;
        if (dist(a.x, a.y, n.x, n.y) < 900) a.alert = 4;
      }
    }
  }
  float myFrac = n.hp / maxHp;
  bool losing = !foe || (foe->hp / clsMaxHp(foe->cls) >= myFrac + 0.05f);
  // superioridade numerica e o sinal mais forte em combate; sem isso o
  // medic segura 40% de vida com 4 contra 1 ate morrer
  int nAlly = 0, nFoe = 0;
  for (auto& o : w.npcs) {
    if (!o.alive) continue;
    float d = dist(n.x, n.y, o.x, o.y);
    if (d > 450) continue;
    if (o.team == n.team) nAlly++; else nFoe++;
  }
  bool outnumbered = nFoe > nAlly * 2 && nFoe >= 4;
  if ((myFrac < 0.25f || (outnumbered && myFrac < 0.55f && n.fleeLock <= 0)) && losing && n.state != NST_DRIVE) {
    n.fleeLock = 12;
    n.state = NST_FLEE;
  }
  else if (n.state == NST_SEEK_CAR && foe && bd < 150) n.state = NST_CHASE;
  else if (n.state == NST_SEEK_CAR) {
    if (foe && bd < 150) n.state = NST_CHASE;
  }
  else if (n.cls == CLS_MEDIC && wounded && (!foe || bd > 140)) n.state = NST_HEAL;
  else if (n.cls == CLS_MEDIC && foe && bd < 140) n.state = (n.hp < maxHp * 0.7f ? NST_FLEE : NST_COVER);
  else if (foe && bd < (n.wpn == W_SNIPER ? 460 : n.wpn == W_AK ? 300 : 230) && n.cls != CLS_SNIPER) n.state = NST_ATTACK;
  else if (foe && n.cls == CLS_SNIPER && bd < 300) n.state = NST_CHASE;
  else if (foe && n.cls == CLS_SNIPER && bd < 550 && n.hp < maxHp * 0.7f) n.state = NST_COVER;
  else if (foe && n.hp < maxHp * 0.6f && bd < see * 0.8f && n.cls != CLS_HEAVY) n.state = NST_COVER;
  else if (foe) n.state = NST_CHASE;
  else if (hasMem && (n.state == NST_PATROL || n.state == NST_CAPTURE || n.state == NST_CHASE || n.state == NST_ATTACK)) n.state = NST_CHASE;
  else if (n.state != NST_PATROL && n.state != NST_CAPTURE && n.state != NST_CHAT) n.state = NST_PATROL;

  float sp = clsSpeed(n.cls, 0), tx = n.wx, ty = n.wy;
  float strafe = 0;
  if (n.state == NST_SEEK_CAR) {
    if (foe && bd < 150) { n.state = NST_CHASE; }
    else {
    int bi = -1; float bsScore = 1e9; float bb = 1e9;
    for (int i = 0; i < (int)w.cars.size(); i++) {
      Car& c = w.cars[i];
      if (c.wreck || c.traffic) continue;
      if (c.team >= 0 && c.team != n.team) continue;
      float d = dist(n.x, n.y, c.x, c.y);
      if (d > 800) continue;
      if (c.busy && !(c.armed && carCrew(w, i) <= 1)) continue;
      bool contested = false;
      for (auto& o : w.npcs) {
        if (&o == &n || !o.alive || o.team != n.team) continue;
        if (o.state == NST_SEEK_CAR && dist(o.x, o.y, c.x, c.y) < 200 && dist(o.x, o.y, n.x, n.y) > 1) { contested = true; break; }
      }
      if (contested) continue;
      float score = d;
      if (c.armed && c.team == n.team) score -= 400;
      if (c.model == CAR_HILUX) score -= 50;
      score -= (c.hp / c.maxHp) * 100;
      if (score < bsScore) { bsScore = score; bi = i; bb = d; }
    }
    if (bi < 0) n.state = NST_PATROL;
    else {
      Car& c = w.cars[bi];
      tx = c.x; ty = c.y; sp = clsSpeed(n.cls, 1);
      if (bb < 24) {
        if (c.wreck) { n.state = NST_PATROL; n.ret = 0.5f; }
        else if (foeNear(w, c.x, c.y, n.team, 120) < 120 || !losClear(n.x, n.y, c.x, c.y, w)) {
          if (foe) n.state = NST_CHASE;
          else { n.state = NST_PATROL; n.ret = 0.5f; }
        } else if (!c.busy && carCrew(w, bi) == 0) {
          c.busy = true; c.team = n.team; c.t = 0;
          n.carId = bi; n.state = NST_DRIVE;
          n.x = c.x; n.y = c.y; n.cx = 0; n.cy = 0; n.qn = 0; n.prt = 0;
          c.tur = c.ang;
          return;
        } else if (c.armed && carCrew(w, bi) == 0) {
          c.busy = true; c.team = n.team; c.t = 0;
          n.carId = bi; n.state = NST_DRIVE;
          n.x = c.x; n.y = c.y; n.cx = 0; n.cy = 0; n.qn = 0; n.prt = 0;
          return;
        } else if (c.armed && carCrew(w, bi) == 1 && !carHasDriver(w, bi, &n)) {
          c.busy = true; c.team = n.team; c.t = 0;
          n.carId = bi; n.state = NST_DRIVE;
          n.x = c.x; n.y = c.y; n.cx = 0; n.cy = 0; n.qn = 0; n.prt = 0;
          return;
        } else if (c.armed && carCrew(w, bi) == 1) {
          n.carId = bi; n.state = NST_ATTACK;
          n.x = c.x; n.y = c.y;
          return;
        } else { n.state = NST_PATROL; n.ret = 0.5f; }
      }
    }
    }
  } else if (n.state == NST_PATROL || n.state == NST_CAPTURE) {
    if (n.state == NST_PATROL && n.ret <= 0) {
      int c = w.terr.cellAt(n.x, n.y);
      bool frontier = c >= 0 && w.terr.owner[c] != n.team;
      if (n.role == ROLE_RAIDER && !frontier && n.cls != CLS_SNIPER && rand() % 100 < 70) {
        invasionTarget(w, n.team, n.x, n.y, tx, ty);
        n.state = NST_CAPTURE;
      } else if (n.role == ROLE_GUARD && frontier && !foe) {
        float btx = tx, bty = ty; bool found = false;
        for (int k = 0; k < 12; k++) {
          float ax = n.x + (rand() % 1000 - 500), ay = n.y + (rand() % 1000 - 500);
          int cc = w.terr.cellAt(ax, ay);
          if (cc < 0 || w.terr.owner[cc] != n.team) continue;
          int cx = cc % w.terr.gw, cy = cc / w.terr.gw;
          bool adj = false;
          const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
          for (int d = 0; d < 4; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (nx < 0 || ny < 0 || nx >= w.terr.gw || ny >= w.terr.gh) continue;
            int ni = ny * w.terr.gw + nx;
            if (w.terr.owner[ni] != n.team && w.terr.owner[ni] != -1) { adj = true; break; }
          }
          if (adj) { btx = ax; bty = ay; found = true; break; }
          if (!found) { btx = ax; bty = ay; }
        }
        tx = btx; ty = bty;
      }
      if (n.state == NST_PATROL && n.chat <= 0 && !foe) {
        int cc0 = w.terr.cellAt(n.x, n.y);
        if (cc0 >= 0 && w.terr.owner[cc0] == n.team && (rand() % 100) < chatP(w.hour) * 50) {
          for (auto& o : w.npcs) {
            if (&o == &n || !o.alive || o.team != n.team || o.carId >= 0) continue;
            float dd = dist(n.x, n.y, o.x, o.y);
            if (dd < 150 && dd > 20) { n.state = NST_CHAT; n.ret = 3 + rand() % 3; n.chat = 12 + rand() % 10; break; }
          }
        }
      }
      if (n.state == NST_PATROL) {
        if (n.cls == CLS_SNIPER) { tx = home.mx + (rand() % 800 - 400); ty = home.my + (rand() % 800 - 400); }
        else if (n.cls == CLS_MEDIC) {
          NPC* friend1 = nullptr;
          float bf = 600;
          for (auto& o : w.npcs) {
            if (!o.alive || o.team != n.team || &o == &n || o.cls == CLS_MEDIC) continue;
            float d = dist(n.x, n.y, o.x, o.y);
            if (d < bf) { bf = d; friend1 = &o; }
          }
          if (friend1) { tx = friend1->x + (rand() % 160 - 80); ty = friend1->y + (rand() % 160 - 80); }
          else { tx = n.x + (rand() % 500 - 250); ty = n.y + (rand() % 500 - 250); }
        } else { tx = n.x + (rand() % 600 - 300); ty = n.y + (rand() % 600 - 300); }
      }
      n.wx = fmaxf(0, fminf(WORLD_W, tx)); n.wy = fmaxf(0, fminf(WORLD_H, ty));
      n.ret = 2 + rand() % 3;
      n.qn = 0; n.prt = 0;
      tx = n.wx; ty = n.wy;
      float dtg = dist(n.x, n.y, tx, ty);
      if (dtg > 350 && n.carId < 0 && (n.state == NST_CAPTURE || n.state == NST_PATROL)) {
        if (foe && bd < 150) {}
        else {
        int chance = n.cls == CLS_SNIPER ? 10 : n.cls == CLS_MEDIC ? 20 : 55;
        if (rand() % 100 < chance) {
          for (auto& c : w.cars) {
            if (c.busy || c.wreck || c.traffic) continue;
            if (c.team >= 0 && c.team != n.team) continue;
            if (dist(n.x, n.y, c.x, c.y) < 800) { n.state = NST_SEEK_CAR; break; }
          }
        }
        }
      }
    } else { pathStep(n, w, 55, tx, ty); }
    if (dist(n.x, n.y, tx, ty) < 14) n.ret = 0;
  } else if (n.state == NST_SUPPORT) {
    NPC* mate = nullptr; float mb = 1e9f;
    for (auto& o : w.npcs) {
      if (!o.alive || o.team != n.team || &o == &n || o.carId >= 0) continue;
      if (o.state != NST_ATTACK && o.state != NST_CHASE && o.state != NST_COVER) continue;
      float d = dist(n.x, n.y, o.x, o.y);
      if (d < mb) { mb = d; mate = &o; }
    }
    if (!mate) { n.state = NST_PATROL; tx = n.wx; ty = n.wy; }
    else if (foe) { n.state = NST_CHASE; tx = foe->x; ty = foe->y; sp = clsSpeed(n.cls, 1); }
    else {
      float a = atan2f(mate->y - n.y, mate->x - n.x) + 1.57f * n.strafeDir;
      tx = mate->x + cosf(a) * 140; ty = mate->y + sinf(a) * 140;
      sp = clsSpeed(n.cls, 1);
      if (mb < 140) { tx = n.x; ty = n.y; sp = 0; }
    }
  } else if (n.state == NST_CHAT) {
    NPC* pal = nullptr; float bp = 200;
    for (auto& o : w.npcs) {
      if (&o == &n || !o.alive || o.team != n.team || o.carId >= 0) continue;
      float dd = dist(n.x, n.y, o.x, o.y);
      if (dd < bp) { bp = dd; pal = &o; }
    }
    tx = n.x; ty = n.y; sp = 0;
    if (pal) n.ang = atan2f(pal->y - n.y, pal->x - n.x);
    if (!pal || bp > 220 || n.ret <= 0 || foe) { n.state = NST_PATROL; n.ret = 0; tx = n.wx; ty = n.wy; sp = clsSpeed(n.cls, 0); }
  } else if (n.state == NST_HEAL && wounded) {
    tx = wounded->x; ty = wounded->y; sp = clsSpeed(n.cls, 1);
    if (dist(n.x, n.y, tx, ty) < 70) {
      wounded->hp = fminf(clsMaxHp(wounded->cls), wounded->hp + 16 * dt);
      sp = 0;
    }
  } else if ((n.state == NST_CHASE || n.state == NST_SUPPORT) && foe) {
    sp = clsSpeed(n.cls, 1);
    if (foe->hp < clsMaxHp(foe->cls) * 0.35f) sp += 30;
    if (n.cls == CLS_SNIPER) {
      if (n.ret > 0) { tx = n.x + (rand() % 400 - 200); ty = n.y + (rand() % 400 - 200); }
      else if (bd < 300) { tx = n.x + (n.x - foe->x); ty = n.y + (n.y - foe->y); }
      else if (bd > 550) { tx = foe->x; ty = foe->y; }
      else { tx = n.x; ty = n.y; sp = 0; if (!fire(n, *foe, w)) { tx = foe->x; ty = foe->y; sp = 90; } }
    } else {
      // para na distancia de combate em vez de colar no alvo
      float minD = n.wpn == W_SNIPER ? 420 : n.wpn == W_AK ? 260 : 200;
      if (bd < minD) {
        float ang = atan2f(n.y - foe->y, n.x - foe->x);
        tx = n.x + cosf(ang) * 150; ty = n.y + sinf(ang) * 150;
      } else {
        tx = foe->x; ty = foe->y;
        if (n.cls == CLS_LIGHT && bd < minD + 120) {
          float ang = atan2f(n.y - foe->y, n.x - foe->x) + 1.57f;
          tx += cosf(ang) * 120; ty += sinf(ang) * 120;
        }
      }
      if (bd < see * 0.6f) fire(n, *foe, w);
    }
  } else if (n.state == NST_CHASE && hasMem) {
    tx = n.ex; ty = n.ey; sp = clsSpeed(n.cls, 1);
    if (dist(n.x, n.y, tx, ty) < 50) { n.et = -1; n.state = NST_PATROL; n.ret = 0; }
  } else if (n.state == NST_ATTACK && foe) {
    // tiro a distancia: mantem folga minima por arma, sem entrar no corpo a corpo
    float minD = n.wpn == W_SNIPER ? 420 : n.wpn == W_AK ? 260 : 200;
    if (bd > see) n.state = NST_CHASE;
    else if (bd < minD) {
      float ang = atan2f(n.y - foe->y, n.x - foe->x);
      tx = n.x + cosf(ang) * 150; ty = n.y + sinf(ang) * 150;
      sp = clsSpeed(n.cls, 1);
    } else {
      // heavy segura posicao e metralha; light orbita. variar o lado
      // evita o bando inteiro girando na mesma direcao
      if (rand() % 100 < 4) n.strafeDir = -n.strafeDir;
      float ang = atan2f(n.y - foe->y, n.x - foe->x) + 1.57f * n.strafeDir;
      if (n.cls == CLS_HEAVY) {
        tx = n.x + cosf(ang) * 40; ty = n.y + sinf(ang) * 40;
        sp = 50;
      } else {
        tx = n.x + cosf(ang) * 90; ty = n.y + sinf(ang) * 90;
        sp = 90;
      }
      if (!fire(n, *foe, w)) { tx = n.x; ty = n.y; sp = 0; }
    }
  } else if (n.state == NST_COVER && foe) {
    if (n.cx == 0 || n.ret <= 0) {
      float best = 1e9;
      float bx = n.x, by = n.y;
      for (auto& h : w.city.houses) {
        float hx = h.x + h.w / 2, hy = h.y + h.h / 2;
        float dh = dist(n.x, n.y, hx, hy);
        if (dh > 350) continue;
        bool blocks = !losClear(foe->x, foe->y, hx, hy, w);
        bool reach = losClear(n.x, n.y, hx, hy, w);
        float score = dh + (blocks ? -200 : 150) + (reach ? 0 : 200);
        if (dist(hx, hy, foe->x, foe->y) > bd + 100) score += 100;
        if (score < best) { best = score; bx = hx; by = hy; }
      }
      n.cx = bx; n.cy = by;
      n.ret = 3;
    }
    tx = n.cx; ty = n.cy; sp = 170;
    if (dist(n.x, n.y, tx, ty) < 26) {
      sp = 0;
      if (!fire(n, *foe, w)) { tx = foe->x; ty = foe->y; sp = 90; n.cx = 0; }
      else strafe = 40;
    }
    if (n.hp > maxHp * 0.85f || !foe) { n.state = NST_PATROL; n.cx = 0; }
  } else if (n.state == NST_FLEE) {
    NPC* med = nullptr; float md = 500;
    if (n.cls != CLS_MEDIC && n.hp > maxHp * 0.15f) {
      for (auto& o : w.npcs) {
        if (!o.alive || o.team != n.team || o.cls != CLS_MEDIC || &o == &n) continue;
        float d = dist(n.x, n.y, o.x, o.y);
        if (d < md) { md = d; med = &o; }
      }
    }
    if (med && md < 500) {
      n.wx = med->x; n.wy = med->y;
      tx = med->x; ty = med->y; sp = clsSpeed(n.cls, 1);
      if (md < 80) { n.hp += 10 * dt; if (n.hp > maxHp * 0.6f) n.state = NST_PATROL; }
    } else {
      if (n.wx != home.mx || n.wy != home.my) { n.wx = home.mx; n.wy = home.my; n.qn = 0; n.prt = 0; }
      pathStep(n, w, 120, tx, ty);
      sp = clsSpeed(n.cls, 1);
    }
    for (auto& a : w.npcs) {
      if (!a.alive || a.team != n.team || &a == &n) continue;
      if (dist(a.x, a.y, n.x, n.y) < 500) a.alert = 2.5f;
    }
    if (foe && bd < 200) fire(n, *foe, w);
    if (dHome < 220) {
      n.hp += 25 * dt;
      // teto real: com upgrade o hp max passa de maxHp e o limiar antigo
      // nunca era alcancado, travando o npc na base para sempre
      float top = clsMaxHp(n.cls) * (1 + 0.2f * w.cartels[n.team].upg);
      if (n.hp > fmaxf(maxHp * 0.6f, top * 0.75f)) n.state = NST_PATROL;
    }
  }
  if (!foe && !wounded && n.state == NST_PATROL) {
    for (auto& a : w.npcs) {
      if (!a.alive || a.team != n.team || &a == &n) continue;
      if ((a.state == NST_ATTACK || a.state == NST_CHASE) && dist(n.x, n.y, a.x, a.y) < 450) {
        n.state = NST_SUPPORT; break;
      }
    }
  }
    // ferido: manca e pinga sangue enquanto anda ate curar na base
  float topHp = clsMaxHp(n.cls) * (1 + 0.2f * w.cartels[n.team].upg);
  if (n.hp < topHp * 0.3f && sp > 0) {
    sp *= 0.55f;
    if (rand() % 100 < 8)
      w.blood.push_back({n.x, n.y, 1.5f + (rand() % 20) / 10.f, 30.f + (rand() % 30)});
  }
  float dx = tx - n.x, dy = ty - n.y;
  float dl = sqrtf(dx * dx + dy * dy);
  float dvx = 0, dvy = 0;
  if (dl > 1 && sp > 0) {
    if (!(n.state == NST_ATTACK && dl < 30 && n.wpn == W_PISTOL)) {
      dvx = dx / dl * sp; dvy = dy / dl * sp;
    }
  }
  if (strafe != 0 && foe) {
    float a = atan2f(n.y - foe->y, n.x - foe->x) + 1.57f;
    dvx += cosf(a) * strafe; dvy += sinf(a) * strafe;
  }
  for (auto& o : w.npcs) {
    if (&o == &n || !o.alive || o.carId >= 0) continue;
    float rr = clsSize(n.cls) + clsSize(o.cls);
    float sx = n.x - o.x, sy = n.y - o.y;
    float sd = sqrtf(sx * sx + sy * sy);
    if (sd < rr && sd > 0.1f) { dvx += sx / sd * 90; dvy += sy / sd * 90; }
  }
  if (n.carId < 0 && (n.state == NST_PATROL || n.state == NST_CAPTURE || n.state == NST_SEEK_CAR || n.state == NST_FLEE))
    probe(w, n.wallSide, n.x, n.y, dvx, dvy, 55);
  float k = fminf(1, dt * 6);
  n.vx += (dvx - n.vx) * k;
  n.vy += (dvy - n.vy) * k;
  for (auto& h : w.city.houses) {
    float r = clsSize(n.cls) / 2 + 2;
    float hx = fmaxf(h.x, fminf(n.x, h.x + h.w));
    float hy = fmaxf(h.y, fminf(n.y, h.y + h.h));
    float sx = n.x - hx, sy = n.y - hy;
    float sd = sqrtf(sx * sx + sy * sy);
    if (sd < r + 14 && sd > 0.1f) {
      dvx += sx / sd * 120; dvy += sy / sd * 120;
      n.vx += sx / sd * 200 * dt; n.vy += sy / sd * 200 * dt;
    }
  }
  n.x += n.vx * dt;
  n.y += n.vy * dt;
  bool hitWall = false;
  for (auto& h : w.city.houses) {
    float r = clsSize(n.cls) / 2;
    if (n.x > h.x - r && n.x < h.x + h.w + r && n.y > h.y - r && n.y < h.y + h.h + r) {
      float l = n.x - (h.x - r), rr = (h.x + h.w + r) - n.x;
      float u = n.y - (h.y - r), dd = (h.y + h.h + r) - n.y;
      float m = fminf(fminf(l, rr), fminf(u, dd));
      if (m == l) { n.x = h.x - r; n.vx = fminf(0, n.vx); }
      else if (m == rr) { n.x = h.x + h.w + r; n.vx = fmaxf(0, n.vx); }
      else if (m == u) { n.y = h.y - r; n.vy = fminf(0, n.vy); }
      else { n.y = h.y + h.h + r; n.vy = fmaxf(0, n.vy); }
      hitWall = true;
    }
  }
  // separacao posicional: empurra os dois, senao ficam um dentro do outro
  for (auto& o : w.npcs) {
    if (&o == &n || !o.alive || o.carId >= 0) continue;
    float rr = (clsSize(n.cls) + clsSize(o.cls)) / 2 + 2;
    float sx = n.x - o.x, sy = n.y - o.y;
    float sd = sqrtf(sx * sx + sy * sy);
    if (sd >= rr) continue;
    if (sd < 0.01f) { sx = 1; sy = 0; sd = 1; }
    float push = (rr - sd) * 0.5f;
    n.x += sx / sd * push; n.y += sy / sd * push;
    o.x -= sx / sd * push; o.y -= sy / sd * push;
  }
  float want = (dl > 1 && sp > 0) ? sp : 0;
  float got = sqrtf(n.vx * n.vx + n.vy * n.vy);
  if ((hitWall && want > 50) || (want > 60 && got < 18 && dl > 40)) n.stuck += dt;
  else n.stuck = fmaxf(0, n.stuck - dt * 2);
  if (n.stuck > 0.6f) {
    n.stuck = 0; n.cx = 0; n.cy = 0; n.qn = 0; n.prt = 0;
    float side = (rand() % 2 ? 1 : -1);
    float sa = n.ang + side * 1.57f;
    n.vx = cosf(sa) * 120; n.vy = sinf(sa) * 120;
    n.ret = fminf(n.ret, 1.2f);
    if (n.state == NST_SEEK_CAR) { n.state = NST_PATROL; n.ret = 0.5f; }
    if (n.state == NST_COVER) { n.cx = 0; n.state = NST_CHASE; }
  }
  if (foe) n.ang = atan2f(foe->y - n.y, foe->x - n.x);
  else if (fabsf(n.vx) + fabsf(n.vy) > 10) n.ang = atan2f(n.vy, n.vx);
  n.anim += sqrtf(n.vx * n.vx + n.vy * n.vy) * dt * 0.09f;
  n.x = fmaxf(0, fminf(WORLD_W, n.x));
  n.y = fmaxf(0, fminf(WORLD_H, n.y));
}
