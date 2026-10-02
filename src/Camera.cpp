#include "Camera.h"
#include "Config.h"
#include <algorithm>
Camera::Camera(int w, int h): x(WORLD_W / 2), y(WORLD_H / 2), tx(WORLD_W / 2), ty(WORLD_H / 2), zoom(0.32f), tzoom(0.32f), vw(w), vh(h) {}
void Camera::update(float dt) {
  float k = std::min(1.f, dt * 6.f);
  x += (tx - x) * k; y += (ty - y) * k; zoom += (tzoom - zoom) * k;
}
void Camera::move(float dx, float dy) {
  tx = std::clamp(tx + dx, 0.f, WORLD_W);
  ty = std::clamp(ty + dy, 0.f, WORLD_H);
}
void Camera::addZoom(float d) { tzoom = std::clamp(tzoom + d, 0.2f, 2.f); }
void Camera::worldToScreen(float wx, float wy, float &sx, float &sy) const {
  sx = (wx - x) * zoom + vw * 0.5f;
  sy = (wy - y) * zoom + vh * 0.5f;
}
