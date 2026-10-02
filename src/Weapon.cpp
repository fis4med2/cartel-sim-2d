#include "Weapon.h"
int wpnDmg(int t) { return t == W_AK ? 24 : t == W_SNIPER ? 60 : 10; }
float wpnRange(int t) { return t == W_AK ? 420 : t == W_SNIPER ? 650 : 280; }
float wpnCool(int t) { return t == W_AK ? 0.28f : t == W_SNIPER ? 1.5f : 0.6f; }
float wpnSpeed(int t) { return t == W_AK ? 1400 : t == W_SNIPER ? 2200 : 900; }
float wpnSpread(int t) { return t == W_AK ? 0.09f : t == W_SNIPER ? 0.012f : 0.06f; }
float wpnFlash(int t) { return t == W_AK ? 9 : t == W_SNIPER ? 14 : 6; }
