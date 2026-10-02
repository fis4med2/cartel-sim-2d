#include "Audio.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <sys/stat.h>
static const char* MP3[3] = {"pistol-sound-effect_zejYI9w.mp3", "tir-dak47.mp3", "machine gun.mp3"};
static void cb(void* u, Uint8* s, int len) {
  Audio* a = (Audio*)u;
  Sint16* o = (Sint16*)s;
  int n = len / 4;
  for (int i = 0; i < n; i++) { o[i * 2] = 0; o[i * 2 + 1] = 0; }
  for (int k = 0; k < 8; k++) {
    Audio::Voice& v = a->v[k];
    if (!v.on) continue;
    for (int i = 0; i < n && v.pos < v.n; i++, v.pos++) {
      int sm = v.d[v.pos];
      o[i * 2] += (Sint16)(sm * v.vl / 128);
      o[i * 2 + 1] += (Sint16)(sm * v.vr / 128);
    }
    if (v.pos >= v.n) v.on = false;
  }
}
static bool older(const std::string& a, const std::string& b) {
  struct stat sa, sb;
  if (stat(a.c_str(), &sa)) return true;
  if (stat(b.c_str(), &sb)) return false;
  return sb.st_mtime > sa.st_mtime;
}
static std::string findMp3(const char* name) {
  const char* pre[3] = {"", "../", "assets/"};
  char b[512];
  for (int i = 0; i < 3; i++) {
    snprintf(b, sizeof b, "%s%s", pre[i], name);
    struct stat s;
    if (!stat(b, &s)) return b;
  }
  return "";
}
bool Audio::init() {
  SDL_AudioSpec want, got;
  SDL_zero(want);
  want.freq = 22050; want.format = AUDIO_S16SYS; want.channels = 2;
  want.samples = 1024; want.callback = cb; want.userdata = this;
  dev = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
  if (!dev) return false;
  for (int i = 0; i < 3; i++) {
    std::string mp3 = findMp3(MP3[i]);
    if (mp3.empty()) continue;
    std::string wav = mp3.substr(0, mp3.size() - 4) + ".wav";
    if (older(wav, mp3)) {
      char cmd[1024];
      snprintf(cmd, sizeof cmd, "ffmpeg -y -v error -i '%s' -ac 1 -ar 22050 -sample_fmt s16 '%s'",
        mp3.c_str(), wav.c_str());
      if (system(cmd)) continue;
    }
    SDL_AudioSpec sp;
    Uint8* raw = nullptr; Uint32 rl = 0;
    if (!SDL_LoadWAV(wav.c_str(), &sp, &raw, &rl)) continue;
    SDL_AudioCVT cv;
    if (SDL_BuildAudioCVT(&cv, sp.format, sp.channels, sp.freq, AUDIO_S16SYS, 1, 22050) < 0) {
      SDL_FreeWAV(raw); continue;
    }
    cv.len = rl; cv.buf = (Uint8*)malloc(rl * cv.len_mult);
    memcpy(cv.buf, raw, rl);
    SDL_FreeWAV(raw);
    if (SDL_ConvertAudio(&cv)) { free(cv.buf); continue; }
    b[i].d = (Sint16*)cv.buf;
    b[i].n = cv.len_cvt / 2;
  }
  if (!b[0].d && !b[1].d && !b[2].d) { SDL_CloseAudioDevice(dev); dev = 0; return false; }
  ok = true;
  SDL_PauseAudioDevice(dev, 0);
  return true;
}
void Audio::play(int wpn, float x, float y, float cx, float cy) {
  if (!ok) return;
  int bi = wpn == 1 ? 1 : wpn == 3 ? 2 : wpn == 2 ? 2 : 0;
  if (bi < 0 || bi > 2 || !b[bi].d) return;
  float dx = x - cx, dy = y - cy;
  float d = sqrtf(dx * dx + dy * dy);
  if (d > 1500) return;
  float vv = 1 - d / 1500;
  int vol = (int)(vv * vv * 128);
  if (vol < 6) return;
  float p = fmaxf(-1, fminf(1, dx / 700));
  int vl = p < 0 ? vol : (int)(vol * (1 - p));
  int vr = p > 0 ? vol : (int)(vol * (1 + p));
  SDL_LockAudioDevice(dev);
  int slot = -1;
  for (int i = 0; i < 8; i++) if (!v[i].on) { slot = i; break; }
  if (slot < 0) {
    slot = 0;
    for (int i = 1; i < 8; i++) if (v[i].pos > v[slot].pos) slot = i;
  }
  v[slot].d = b[bi].d; v[slot].n = b[bi].n; v[slot].pos = 0;
  v[slot].vl = vl; v[slot].vr = vr; v[slot].on = true;
  SDL_UnlockAudioDevice(dev);
}
void Audio::close() {
  if (dev) SDL_CloseAudioDevice(dev);
  dev = 0;
  for (int i = 0; i < 3; i++) { free(b[i].d); b[i].d = nullptr; }
  ok = false;
}
