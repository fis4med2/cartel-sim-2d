#pragma once
#include <vector>
#include <utility>
struct Renderer;
struct Camera;
struct House { float x, y, w, h; };
struct City {
  std::vector<House> houses;
  float roadX0 = 2300, roadX1 = 2500, roadY0 = 2300, roadY1 = 2500;
  int bw = 96, bh = 96;
  float cell = 50;
  std::vector<char> block;
  void gen(const std::vector<std::pair<float, float>>& mans);
  bool inRoad(float x, float y, float w, float h) const;
  bool onRoad(float x, float y) const;
  void roadPoint(float x, float y, float& ox, float& oy) const;
  void buildBlock();
  bool blockedAt(float x, float y) const;
  int findPath(float x0, float y0, float x1, float y1, float ox[8], float oy[8]) const;
  void render(Renderer& r, const Camera& c) const;
  void renderLights(Renderer& r, const Camera& c) const;
};
