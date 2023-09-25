#include <SDL2/SDL.h>
#include <iostream>
#include "Color.h"
#include "Framebuffer.h"
#include "ObjReader.h"
#include "Face.h"
#include "Uniforms.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <chrono>
#include <omp.h>
#include "Shaders.h"
#include "Fragment.h"

using namespace std::chrono;

// Global variables
Framebuffer framebuffer;
std::vector<glm::vec3> vertexArray;
std::vector<glm::vec3> resultVertexArray;
std::vector<Face> faceArray;
glm::mat4 rotMatrix = glm::rotate(glm::mat4(1), (0.05f), glm::vec3(0, 1, 0.2));
Color myColor(255, 255, 255);

// SDL window and renderer
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Function to initialize SDL
bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("cg-hw-modeling", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: Failed to create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

std::vector<std::vector<glm::vec3>> primitiveAssembly(
        const std::vector<glm::vec3>& transformedVertices
) {
    std::vector<std::vector<glm::vec3>> assembledPrimitives;
    std::vector<glm::vec3> currentPrimitive;

    for (const glm::vec3& vertex : transformedVertices) {
        currentPrimitive.push_back(vertex);

        if (currentPrimitive.size() == 3) {
            assembledPrimitives.push_back(currentPrimitive);
            currentPrimitive.clear();
        }
    }

    return assembledPrimitives;
}

// Function to render using vertex and uniform data
void render(const std::vector<glm::vec3>& vertices) {

    // 1. Vertex Shader
    std::vector<glm::vec3> transformedVertices;
    for (const glm::vec3& vertex : vertices) {
        glm::vec3 transformedVertex = vertexShader(vertex);
        transformedVertices.push_back(transformedVertex);
    }

    // 2. Primitive Assembly
    std::vector<std::vector<glm::vec3>> assembledPrimitives = primitiveAssembly(transformedVertices);

    // 3. Rasterization
    std::vector<Fragment> fragments;
    for (std::vector<glm::vec3> item : assembledPrimitives) {
        std::vector<Fragment> rasterizedTriangle = triangle(
                item[0],
                item[1],
                item[2]
        );
        fragments.insert(
                fragments.end(),
                rasterizedTriangle.begin(),
                rasterizedTriangle.end()
                );
    }
    // 4. Fragment Shader
    for(Fragment fragment: fragments){
        Color fragmentColor = fragmentShader(fragment);
        point(framebuffer, fragment.position.x, fragment.position.y, fragmentColor);
    }
}


int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    // Initialize uniforms for model, view, and projection matrices
    Uniforms uniforms;
    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 projection = glm::mat4(1);
    uniforms.model = model;
    uniforms.model = view;
    uniforms.model = projection;

    bool running = true;

    // Load OBJ file and set up vertex and face data
    loadOBJ("../Spaceship.obj", vertexArray, faceArray);
    resultVertexArray = setupVertexArray(vertexArray, faceArray);
    glm::vec4 tempVector;

    auto lastTime = high_resolution_clock::now();  // Record initial time

    Uint32 frameStart, frameTime;
    std::string title = "FPS: ";

    while (running) {
        frameStart = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Clear the framebuffer
        clear(framebuffer);

        // Apply rotation to vertices
        #pragma omp parallel for
        for (int i = 0; i < resultVertexArray.size(); i++) {
            glm::vec3 tempVector = rotMatrix * glm::vec4(resultVertexArray[i].x, resultVertexArray[i].y, resultVertexArray[i].z, 0);
            resultVertexArray[i] = glm::vec3(tempVector.x, tempVector.y, tempVector.z);
        }

        render(resultVertexArray);
        renderBuffer(renderer, framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);

        // Calculate frames per second and update window title
        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime > 0) {
            std::ostringstream titleStream;
            titleStream << "FPS: " << 1000.0 / frameTime;  // Milliseconds to seconds
            SDL_SetWindowTitle(window, titleStream.str().c_str());
        }
    }

    // Clean up and quit SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
