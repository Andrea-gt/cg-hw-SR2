#pragma once
#include <glm/vec3.hpp>
#include "Uniforms.h"

glm::vec3 vertexShader(const glm::vec3& vertex) {
    glm::vec3 transformedVertex = glm::vec3(
            vertex.x * 200.0f, // Multiply, not assign
            vertex.y * -200.0f, // Multiply, not assign
            vertex.z * 200.0f // Multiply, not assign
    );
    return transformedVertex;
}


Color fragmentShader(Fragment) {
    return  Color{255,0,0};
}

