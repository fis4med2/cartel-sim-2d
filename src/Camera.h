#pragma once
struct Camera {
  float x, y, tx, ty, zoom, tzoom;
  int vw, vh;
  Camera(int w, int h);
  void update(float dt);
  void move(float dx, float dy);
  void addZoom(float d);
  void worldToScreen(float wx, float wy, float &sx, float &sy) const;
};
