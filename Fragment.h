#pragma once
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    Color color;
    glm::vec3 normal;
};


struct Fragment {
    glm::ivec2 position; // X and Y coordinates of the pixel (in screen space)
    Color color; // r, g, b values for color
    double z;
    float intensity;
};


