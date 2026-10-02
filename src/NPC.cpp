#include "NPC.h"
#include <cstdlib>
float clsMaxHp(int c) {
  return c == CLS_HEAVY ? 180 : c == CLS_SNIPER ? 70 : c == CLS_MEDIC ? 90 : 80;
}
float clsSpeed(int c, int combat) {
  if (c == CLS_LIGHT) return combat ? 170 : 110;
  if (c == CLS_HEAVY) return combat ? 100 : 70;
  if (c == CLS_SNIPER) return combat ? 110 : 85;
  return combat ? 140 : 100;
}
float clsSize(int c) {
  return c == CLS_HEAVY ? 14 : c == CLS_LIGHT ? 8 : 10;
}
void setRole(NPC& n) {
  int r = rand() % 100;
  if (n.cls == CLS_SNIPER || n.cls == CLS_MEDIC) n.role = r < 70 ? ROLE_GUARD : ROLE_RAIDER;
  else n.role = r < 45 ? ROLE_GUARD : ROLE_RAIDER;
}
