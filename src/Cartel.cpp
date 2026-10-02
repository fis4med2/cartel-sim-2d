#include "Cartel.h"
Cartel::Cartel(int i, std::string n, SDL_Color c, float x, float y)
    : id(i), name(n), color(c), money(1000), mx(x), my(y) {}
