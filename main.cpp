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
#include <random>
#include "Shaders.h"
#include "Fragment.h"
#include "Camera.h"

using namespace std::chrono;

// Global variables
Framebuffer framebuffer;
Zbuffer zbuffer;
std::vector<Vertex> resultVertexArray;


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

int getRandomInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

std::vector<std::pair<int, int>> staticPixels;

// Function to add random static white pixels
void addRandomStaticWhitePixels() {
    staticPixels.clear();  // Clear the existing static pixels
    int numStaticPixels = 50;  // Adjust the number of static pixels as needed
    for (int i = 0; i < numStaticPixels; ++i) {
        int x = getRandomInt(0, SCREEN_WIDTH - 1);
        int y = getRandomInt(0, SCREEN_HEIGHT - 1);
        staticPixels.push_back(std::make_pair(x, y));
    }
}

// Function to copy static pixels to the framebuffer
void copyStaticPixelsToFramebuffer(Framebuffer& framebuffer) {
    for (const auto& pixel : staticPixels) {
        point(framebuffer, zbuffer, pixel.first, pixel.second, 0, Color(255, 255, 255));
    }
}

std::vector<std::vector<Vertex>> primitiveAssembly(const std::vector<Vertex>& transformedVertices) {
    std::vector<std::vector<Vertex>> assembledPrimitives;
    std::vector<Vertex> currentPrimitive;

    for (const Vertex vertex : transformedVertices) {
        currentPrimitive.push_back(vertex);

        if (currentPrimitive.size() == 3) {
            assembledPrimitives.push_back(currentPrimitive);
            currentPrimitive.clear();
        }
    }

    return assembledPrimitives;
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 60.0f;
    float aspectRatio = static_cast<float>(SCREEN_WIDTH / SCREEN_HEIGHT);
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(SCREEN_WIDTH / 2.0f, SCREEN_WIDTH / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

// Function to render using vertex and uniform data
void render(const std::vector<Vertex>& VBO, const Uniforms& uniforms) {
    // 1. Vertex Shader
    std::vector<Vertex> transformedVertices;
#pragma omp parallel for
    for (int i=0; i<VBO.size(); i+=1) {
        Vertex transformedVertex = vertexShader(VBO[i], uniforms);
        transformedVertices.push_back(transformedVertex);
    }

    // 2. Primitive Assembly
    std::vector<std::vector<Vertex>> assembledPrimitives = primitiveAssembly(transformedVertices);

    // 3. Rasterization
    std::vector<Fragment> fragments;
    for (std::vector<Vertex> item : assembledPrimitives) {
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
    for (Fragment& fragment : fragments) {
        Color fragmentColor = fragmentShaderEarth(fragment);
        point(framebuffer, zbuffer, fragment.position.x, fragment.position.y, fragment.z,fragmentColor);
    }
}

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    bool running = true;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> texCoords;
    std::vector<Face> faces;
    std::vector<glm::vec3> vertexBufferObject; // This will contain both vertices and normals

    // Load OBJ file and set up vertex and face data
    loadOBJ("../sphere.obj", vertices, normals, texCoords, faces );
    resultVertexArray = setupVertexArray(vertices, normals, faces);
    glm::vec4 tempVector;

    Uniforms uniforms;

    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 projection = glm::mat4(1);

    glm::vec3 translationVector(0.0f, -0.5f, 0.0f);
    float a = 45.0f;
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.2f); // Rotate around the Y-axis
    glm::vec3 scaleFactor(2.0f, -2.0f, 2.0f);

    // Camera
    Camera camera;
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), translationVector);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), scaleFactor);

    uniforms.viewport = createViewportMatrix();
    uniforms.projection = createProjectionMatrix();

    auto lastTime = high_resolution_clock::now();

    Uint32 frameStart, frameTime;
    std::string title = "FPS: ";

    addRandomStaticWhitePixels();

    while (running) {
        frameStart = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Clear the framebuffer
        clear(framebuffer, zbuffer);
        copyStaticPixelsToFramebuffer(framebuffer);

        a += 0.5;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(a), rotationAxis);


        uniforms.model = translation * rotation * scale;
        uniforms.view = glm::lookAt(
                camera.cameraPosition, // The position of the camera
                camera.targetPosition, // The point the camera is looking at
                camera.upVector        // The up vector defining the camera's orientation
        );

        render(resultVertexArray, uniforms);
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
