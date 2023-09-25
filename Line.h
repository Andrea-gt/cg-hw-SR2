#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Fragment.h"

std::vector<Fragment> line(const Vertex& v1, const Vertex& v2) {
    glm::ivec2 p1(static_cast<int>(v1.position.x), static_cast<int>(v1.position.y));
    glm::ivec2 p2(static_cast<int>(v2.position.x), static_cast<int>(v2.position.y));

    std::vector<Fragment> fragments;

    int dx = std::abs(p2.x - p1.x);
    int dy = std::abs(p2.y - p1.y);
    int sx = (p1.x < p2.x) ? 1 : -1;
    int sy = (p1.y < p2.y) ? 1 : -1;

    int err = dx - dy;

    glm::ivec2 current = p1;

    while (true) {
        Fragment fragment;
        fragment.position = current;
        fragment.color = v1.color;

        fragments.push_back(fragment);

        if (current == p2) {
            break;
        }

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            current.x += sx;
        }
        if (e2 < dx) {
            err += dx;
            current.y += sy;
        }
    }

    return fragments;
}