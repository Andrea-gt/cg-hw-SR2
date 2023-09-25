#pragma once
#include <glm/vec3.hpp>
#include "Uniforms.h"
#include "Fragment.h"


Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms) {
    // Apply transformations to the input vertex using the matrices from the uniforms
    glm::vec4 clipSpaceVertex = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(vertex.position, 1.0f);

    // Perspective divide
    glm::vec3 ndcVertex = glm::vec3(clipSpaceVertex) / clipSpaceVertex.w;


    // Apply the viewport transform
    glm::vec4 screenVertex = uniforms.viewport * glm::vec4(ndcVertex, 1.0f);

    // Return the transformed vertex as a vec3
    return Vertex{
            glm::vec3(screenVertex),
            vertex.color
    };
}

Color fragmentShader(Fragment& fragment) {
    return fragment.color;
};

