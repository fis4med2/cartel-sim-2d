#include "Game.h"
#include "Renderer.h"
#include "Camera.h"
#include "World.h"
#include "TimeSystem.h"
#include "UI.h"
#include "Audio.h"
#include <cstdlib>
#include <cstdio>
#include <ctime>
bool Game::init() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return false;
  win = SDL_CreateWindow("CARTEL SIM 2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!win) return false;
  sdlr = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!sdlr) return false;
  SDL_SetRenderDrawBlendMode(sdlr, SDL_BLENDMODE_BLEND);
  srand((unsigned)::time(nullptr));
  cam = new Camera(W, H);
  ren = new Renderer(sdlr);
  world = new World();
  time = new TimeSystem();
  ui = new UI();
  au = new Audio();
  au->init();
  world->gen();
  return true;
}
void Game::input(float dt) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) run = false;
    else if (e.type == SDL_MOUSEWHEEL) cam->addZoom(e.wheel.y * 0.08f);
    else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) run = false;
    else if (e.type == SDL_KEYDOWN && !e.key.repeat && e.key.keysym.sym == SDLK_SPACE)
      speed = speed >= 4 ? 1 : speed * 2;
  }
  int mx, my;
  Uint32 mb = SDL_GetMouseState(&mx, &my);
  static int lx = -1, ly = -1;
  if (mb & SDL_BUTTON(SDL_BUTTON_LEFT)) {
    if (lx >= 0) { cam->move(-(mx - lx) / cam->zoom, -(my - ly) / cam->zoom); }
    lx = mx; ly = my;
  } else lx = ly = -1;
  const Uint8* k = SDL_GetKeyboardState(nullptr);
  float sp = 900 * dt / cam->zoom;
  if (sp > 1200 * dt) sp = 1200 * dt;
  if (k[SDL_SCANCODE_W] || k[SDL_SCANCODE_UP]) cam->move(0, -sp);
  if (k[SDL_SCANCODE_S] || k[SDL_SCANCODE_DOWN]) cam->move(0, sp);
  if (k[SDL_SCANCODE_A] || k[SDL_SCANCODE_LEFT]) cam->move(-sp, 0);
  if (k[SDL_SCANCODE_D] || k[SDL_SCANCODE_RIGHT]) cam->move(sp, 0);
  if (k[SDL_SCANCODE_Q]) cam->addZoom(-dt);
  if (k[SDL_SCANCODE_E]) cam->addZoom(dt);
  SDL_GetWindowSize(win, &W, &H);
  cam->vw = W; cam->vh = H;
}
void Game::update(float dt) {
  cam->update(dt);
  time->update(dt);
  world->update(dt, *time);
  int n = 0;
  for (auto& s : world->sounds) {
    if (n++ >= 6) break;
    au->play(s.wpn, s.x, s.y, cam->x, cam->y);
  }
  world->sounds.clear();
}
void Game::render() {
  ren->clear({20, 20, 26, 255});
  world->render(*ren, *cam);
  int a = time->overlay();
  if (a > 0) ren->screenFill(0, 0, W, H, SDL_Color{5, 5, 25, (Uint8)a});
  if (time->night()) world->renderLights(*ren, *cam);
  else if (a > 40) world->renderLights(*ren, *cam);
  ui->draw(*ren, *world, *time, *cam);
  char sb[8];
  snprintf(sb, sizeof sb, "%dX", speed);
  ui->text(*ren, sb, W - 60, H - 44, 2, speed > 1 ? SDL_Color{255, 220, 80, 255} : SDL_Color{140, 140, 140, 255});
  ren->present();
}

void Game::loop() {
  Uint32 last = SDL_GetTicks();
  while (run) {
    Uint32 now = SDL_GetTicks();
    float dt = (now - last) / 1000.f;
    last = now;
    if (dt > 0.05f) dt = 0.05f;
    input(dt);
    for (int i = 0; i < speed; i++) update(dt);
    render();
  }
}
Game::~Game() {
  delete ren; delete cam; delete world; delete time; delete ui;
  if (au) { au->close(); delete au; }
  if (sdlr) SDL_DestroyRenderer(sdlr);
  if (win) SDL_DestroyWindow(win);
  SDL_Quit();
}
