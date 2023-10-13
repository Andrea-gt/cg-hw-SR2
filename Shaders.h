#pragma once
#include <glm/vec3.hpp>
#include "Uniforms.h"
#include "Fragment.h"
#include "FastNoiseLite.h"

FastNoiseLite noiseGenerator;

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms) {
    // Apply transformations to the input vertex using the matrices from the uniforms
    glm::vec4 clipSpaceVertex = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(vertex.position, 1.0f);

    // Perspective divide
    glm::vec3 ndcVertex = glm::vec3(clipSpaceVertex) / clipSpaceVertex.w;

    // Normal transformation
    glm::vec3 transformedNormal = glm::mat3(uniforms.model) * vertex.normal;

    // Apply the viewport transform
    glm::vec4 screenVertex = uniforms.viewport * glm::vec4(ndcVertex, 1.0f);

    // Return the transformed vertex as a vec3
    return Vertex{
            glm::vec3(screenVertex),
            vertex.color,
            transformedNormal,
            vertex.position
    };
};

Color fragmentShader(Fragment& fragment) {
    fragment.color = fragment.color * fragment.intensity; // Red color with full opacity
    return fragment.color;
};

Color fragmentShaderSun(Fragment& fragment) {
    Color color;

    const glm::vec3 orangeColor = glm::vec3(0.96f, 0.39f, 0.08f);
    const glm::vec3 redColor = glm::vec3(0.6f, 0.003f, 0.0f);

    noiseGenerator.SetSeed(1337);
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    noiseGenerator.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes);
    noiseGenerator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_EuclideanSq);
    noiseGenerator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Div);
    noiseGenerator.SetCellularJitter(1);
    noiseGenerator.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);

    glm::vec3 uv = glm::vec3(fragment.originalPos.x, fragment.originalPos.y, fragment.originalPos.z);

    float xo = 1000.0f;
    float yo = 1000.0f;
    float zoom = 3000.0f;

    float noiseValue = noiseGenerator.GetNoise((uv.x + xo) * zoom, (uv.y + yo) * zoom, (uv.z + yo) * zoom);

    // Calculate a t value between 0 and 1 based on the noise value
    float t = (noiseValue + 1.0f) / 2.0f;

    // Interpolate between orangeColor and redColor
    glm::vec3 sunColor = glm::mix(orangeColor, redColor, t);

    fragment.intensity = glm::clamp(fragment.intensity, 1.0f, 1.0f);

    color = Color(sunColor.x * 255.0f, sunColor.y * 255.0f, sunColor.z * 255.0f);

    fragment.color = color * fragment.intensity;

    return fragment.color;
}


Color fragmentShaderEarth(Fragment& fragment) {

    const glm::vec3 orangeColor = glm::vec3(0.96f, 0.39f, 0.08f);
    const glm::vec3 redColor = glm::vec3(0.8f, 0.0f, 0.0f);

    noiseGenerator.SetSeed(1337);
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    noiseGenerator.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes);
    noiseGenerator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_EuclideanSq);
    noiseGenerator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Div);
    noiseGenerator.SetCellularJitter(1);
    noiseGenerator.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);

    glm::vec3 uv = glm::vec3(fragment.originalPos.x, fragment.originalPos.y, fragment.originalPos.z);

    float xo = 1000.0f;
    float yo = 1000.0f;
    float zoom = 2000.0f;

    float noiseValue = noiseGenerator.GetNoise((uv.x + xo) * zoom, (uv.y + yo) * zoom, (uv.z + yo) * zoom);

    // Calculate a t value between 0 and 1 based on the noise value
    float t = (noiseValue + 1.0f) / 2.0f;

    // Interpolate between orangeColor and redColor
    glm::vec3 sunColor = glm::mix(orangeColor, redColor, t);

    // Calculate the bloom intensity based on fragment intensity
    float bloomIntensity = fragment.intensity * -1.0; // Adjust the multiplier as needed

    // Amplify the sun's color based on bloom intensity
    sunColor *= bloomIntensity;

    // Grass
    const glm::vec3 mainGreen = glm::vec3(0.39f, 0.46f, 0.17f);
    const glm::vec3 middleGreen = glm::vec3(0.56f, 0.66f, 0.33f);

    noiseGenerator.SetSeed(1337);
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    noiseGenerator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);
    noiseGenerator.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
    noiseGenerator.SetCellularJitter(1);
    noiseGenerator.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);

    // Generate noise based on the adjusted position and zoom.
    float noiseValue2 = noiseGenerator.GetNoise((uv.x + xo) * zoom, (uv.y + yo) * zoom, (uv.z + yo) * zoom);

    glm::vec3 finalColor = glm::mix(mainGreen, middleGreen, noiseValue2);

    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    // Generate noise based on the adjusted position and zoom.
    float noiseValueC = noiseGenerator.GetNoise((uv.x + xo) * 300.0f, (uv.y + yo) * 300.0f, (uv.z + yo) * 300.0f);

    // Combine 'color' and 'finalColor' based on 'noiseValueC'
    glm::vec3 tmpColor = (noiseValueC < 0.2f) ? sunColor : finalColor;

    // Create a Color object from 'tmpColor'
    Color fragmentColor = Color(tmpColor.x * 255.0f, tmpColor.y * 255.0f, tmpColor.z * 255.0f);

    fragment.color = fragmentColor * fragment.intensity;
    return fragmentColor;
}

Color fragmentShaderMoon(Fragment& fragment) {
    const int seed = 42; // You can change the seed for variation
    const float zoom = 1000.0f;
    const glm::vec3 lightGray = glm::vec3(0.7f, 0.7f, 0.7f);
    const glm::vec3 darkGray = glm::vec3(0.3f, 0.3f, 0.3f);

    noiseGenerator.SetSeed(seed);
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseGenerator.SetFrequency(0.01f); // Adjust the frequency for fine details
    noiseGenerator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    noiseGenerator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add);
    noiseGenerator.SetCellularJitter(2);

    glm::vec3 uv = glm::vec3(fragment.originalPos.x, fragment.originalPos.y, fragment.originalPos.z);

    // Adjust the noise parameters to control the appearance of the moon
    float noiseValue = noiseGenerator.GetNoise(uv.x * zoom, uv.y * zoom, uv.z * zoom);
    noiseValue = (noiseValue + 1.0f) * 0.5f; // Normalize to [0, 1]

    // Use noiseValue to determine the moon surface color
    glm::vec3 moonSurfaceColor = glm::mix(darkGray, lightGray, noiseValue);

    // Create a Color object from 'moonSurfaceColor'
    Color fragmentColor = Color(moonSurfaceColor.x * 255.0f, moonSurfaceColor.y * 255.0f, moonSurfaceColor.z * 255.0f);

    fragment.color = fragmentColor * fragment.intensity;

    return fragment.color;
}

Color fragmentShaderNeptune(Fragment& fragment) {
    const int seed = 1234;
    const float zoom = 1000.0f;
    const float stripeWidth = 0.5f; // Adjust the width of the stripes
    const glm::vec3 stripeColor1 = glm::vec3(0.2f, 0.4f, 0.6f);
    const glm::vec3 stripeColor2 = glm::vec3(0.4f, 0.2f, 0.6f);

    noiseGenerator.SetSeed(seed);
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noiseGenerator.SetFrequency(0.02f);
    noiseGenerator.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    noiseGenerator.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add);
    noiseGenerator.SetCellularJitter(2);

    glm::vec3 uv = glm::vec3(fragment.originalPos.x, fragment.originalPos.y, fragment.originalPos.z);

    // Use FastNoiseLite to generate the stripe pattern
    float noiseValue = noiseGenerator.GetNoise(uv.x * zoom, uv.y * zoom, uv.z * zoom);

    // Use a sine function to create a smooth stripe pattern
    float stripePattern = 0.5f * (glm::sin(uv.x * stripeWidth) + glm::sin(uv.y * stripeWidth) + noiseValue);

    // Interpolate between stripe colors
    glm::vec3 stripeColor = glm::mix(stripeColor1, stripeColor2, stripePattern);

    // Create a Color object from 'stripeColor'
    Color fragmentColor = Color(stripeColor.x * 255.0f, stripeColor.y * 255.0f, stripeColor.z * 255.0f);

    fragment.color = fragmentColor * fragment.intensity;

    return fragment.color;
}

Color fragmentShaderSpace(Fragment& fragment) {
    const int starDensity = 100; // Adjust star density as needed

    // Initialize a random number generator
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> randomDist(1, starDensity);

    // Check if this fragment should be a star (white)
    if (randomDist(gen) == 1) {
        fragment.color = Color(255, 255, 255); // White for stars
    } else {
        fragment.color = Color(0, 0, 10); // Adjust the color as needed
    }

    return fragment.color;
}










