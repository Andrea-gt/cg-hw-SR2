#include <vector>
#include <array>
#pragma once

struct Face
{
    std::array<int, 3> vertexIndices;
    std::array<int, 3> normalIndices;
    std::array<int, 3> texIndices;
};
