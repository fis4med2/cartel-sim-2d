#pragma once
#include <SDL2/SDL.h>
// Som por nome mp3 exato. Converte p/ wav via ffmpeg se ausente.
// play com volume por distancia + pan. Falha muda -> jogo segue.
struct Audio {
  SDL_AudioDeviceID dev = 0;
  struct Buf { Sint16* d = nullptr; Uint32 n = 0; };
  Buf b[3];
  struct Voice { Sint16* d = nullptr; Uint32 n = 0, pos = 0; int vl = 0, vr = 0; bool on = false; };
  Voice v[8];
  bool ok = false;
  bool init();
  void play(int wpn, float x, float y, float cx, float cy);
  void close();
};
