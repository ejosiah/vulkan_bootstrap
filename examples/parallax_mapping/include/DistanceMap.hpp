#pragma once

#include <glm/glm.hpp>
#include <vector>
std::vector<float> init_distance_map(const unsigned char* heightMap, int width, int height, int depth, bool invert = true);