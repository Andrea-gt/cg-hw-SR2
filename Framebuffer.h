#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <array>
#include <glm/vec3.hpp>
#include <math.h>
#include <omp.h>
#include "Color.h" // Include your Color class header
#include "Fragment.h"
#include "Line.h"

constexpr size_t SCREEN_WIDTH = 800;
constexpr size_t SCREEN_HEIGHT = 600;

// Create a framebuffer alias using the Color struct and the defined screen dimensions
using Framebuffer = std::array<std::array<Color, SCREEN_WIDTH>, SCREEN_HEIGHT>;

using Zbuffer = std::array<std::array<float, SCREEN_WIDTH>, SCREEN_HEIGHT>;

glm::vec3 L = glm::vec3(50.0f, 0.0f, 50.0f);

std::pair<float, float> barycentricCoordinates(const glm::ivec2& P, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C) {
    glm::vec3 bary = glm::cross(
            glm::vec3(C.x - A.x, B.x - A.x, A.x - P.x),
            glm::vec3(C.y - A.y, B.y - A.y, A.y - P.y)
    );

    if (abs(bary.z) < 1) {
        return std::make_pair(-1, -1);
    }

    return std::make_pair(
            bary.y / bary.z,
            bary.x / bary.z
    );
}

void clear(Framebuffer& framebuffer, Zbuffer& zbuffer) {

#pragma omp parallel for
    for (size_t i = 0; i < SCREEN_HEIGHT; i++) {
        for (size_t j = 0; j < SCREEN_WIDTH; j++) {
            framebuffer[i][j] = Color(0,0,0);
            zbuffer[i][j] = 99999.0f;
        }
    }
}

void point(Framebuffer &framebuffer, Zbuffer &zbuffer,  int x, int y, double z, Color color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        if(zbuffer[y][x] > z ){
            framebuffer[y][x] = color;
            zbuffer[y][x] = z;
        }
    }
}

std::vector<Fragment> triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    std::vector<Fragment> fragments;
    glm::vec3 A = a.position;
    glm::vec3 B = b.position;
    glm::vec3 C = c.position;

    float minX = std::min(std::min(A.x, B.x), C.x);
    float minY = std::min(std::min(A.y, B.y), C.y);
    float maxX = std::max(std::max(A.x, B.x), C.x);
    float maxY = std::max(std::max(A.y, B.y), C.y);

    // Iterate over each point in the bounding box
#pragma omp parallel for
    for (int y = static_cast<int>(std::ceil(minY)); y <= static_cast<int>(std::floor(maxY)); ++y) {
        for (int x = static_cast<int>(std::ceil(minX)); x <= static_cast<int>(std::floor(maxX)); ++x) {
            if (x < 0 || y < 0 || y > SCREEN_HEIGHT || x > SCREEN_WIDTH)
                continue;

            glm::ivec2 P(x, y);
            auto barycentric = barycentricCoordinates(P, A, B, C);
            float w = 1 - barycentric.first - barycentric.second;
            float v = barycentric.first;
            float u = barycentric.second;
            float epsilon = 1e-10;

            if (w < epsilon || v < epsilon || u < epsilon)
                continue;

            double z = A.z * w + B.z * v + C.z * u;

            glm::vec3 normal = glm::normalize(
                    a.normal * w + b.normal * v + c.normal * u
            );

            glm::vec3 lightDirection = glm::normalize(L - glm::vec3(w,v,u));
            float intensity = glm::dot(normal, lightDirection);
            
            Color color = Color(255, 255, 255);

            fragments.push_back(
                    Fragment{
                            P,
                            color,
                            z,
                            intensity}
            );
        }
    }
    return fragments;
}

void renderBuffer(SDL_Renderer* renderer, const Framebuffer& framebuffer, int textureWidth, int textureHeight) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);

    void* texturePixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &texturePixels, &pitch);

    Uint32 format = SDL_PIXELFORMAT_ARGB8888;
    SDL_PixelFormat* mappingFormat = SDL_AllocFormat(format);

    Uint32* texturePixels32 = static_cast<Uint32*>(texturePixels);

#pragma omp parallel for
    for (int y = 0; y < textureHeight; y++) {
        for (int x = 0; x < textureWidth; x++) {
            int index = y * (pitch / sizeof(Uint32)) + x;
            const Color& color = framebuffer[y][x];
            texturePixels32[index] = SDL_MapRGBA(mappingFormat, color.r, color.g, color.b, color.a);
        }
    }

    SDL_UnlockTexture(texture);
    SDL_Rect textureRect = {0, 0, textureWidth, textureHeight};
    SDL_RenderCopy(renderer, texture, NULL, &textureRect);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);
}

#endif // FRAMEBUFFER_H