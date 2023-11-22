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
std::vector<Vertex> resultVertexArray, resultVertexArrayShip;


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
    int numStaticPixels = 250;  // Adjust the number of static pixels as needed
    for (int i = 0; i < numStaticPixels; ++i) {
        int x = getRandomInt(0, SCREEN_WIDTH - 1);
        int y = getRandomInt(0, SCREEN_HEIGHT - 1);
        staticPixels.push_back(std::make_pair(x, y));
    }
}

// Function to copy static pixels to the framebuffer
void copyStaticPixelsToFramebuffer(Framebuffer& framebuffer) {
    for (const auto& pixel : staticPixels) {
        point(framebuffer, zbuffer, pixel.first, pixel.second, 0, Color(100, 100, 100), true);
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
void render(const std::vector<Vertex> &VBO, const Uniforms &uniforms, Color (*shader)(Fragment&), glm::vec3 translationVector = L) {
    // 1. Vertex Shader
    std::vector<Vertex> transformedVertices(VBO.size());

    //#pragma omp parallel for
    for (int i=0; i<VBO.size(); i+=1) {
        Vertex transformedVertex = vertexShader(VBO[i], uniforms);
        transformedVertices[i] = transformedVertex;
    }

    // 2. Primitive Assembly
    std::vector<std::vector<Vertex>> assembledPrimitives = primitiveAssembly(transformedVertices);

    // 3. Rasterization
    std::vector<Fragment> fragments;
    for (std::vector<Vertex> item : assembledPrimitives) {
        std::vector<Fragment> rasterizedTriangle = triangle(
                item[0],
                item[1],
                item[2],
                translationVector
        );
        fragments.insert(
                fragments.end(),
                rasterizedTriangle.begin(),
                rasterizedTriangle.end()
        );
    }

    // 4. Fragment Shader
    for (Fragment& fragment : fragments) {
        Color fragmentColor = shader(fragment);
        point(framebuffer, zbuffer, fragment.position.x, fragment.position.y, fragment.z,fragmentColor);
    }
}

glm::mat4 createModeltMatrix(glm::vec3 scaleFactor, glm::vec3 translationVector, float a) {
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(a), rotationAxis);

    glm::mat4 model = glm::mat4(1);
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), translationVector);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), scaleFactor);

    return translation * rotation * scale;
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

    vertices = {};
    normals = {};
    texCoords = {};
    faces = {};

    loadOBJ("../Spaceship4.obj", vertices, normals, texCoords, faces );
    //loadOBJ("../nave2.obj", vertices, normals, texCoords, faces );
    resultVertexArrayShip = setupVertexArray(vertices, normals, faces);

    glm::vec4 tempVector;

    Uniforms uniformSun, uniformEarth, uniformNeptune, uniformMoon, uniformShip;

    float a = 45.0f;
    float b = 45.0f;

    glm::vec3 scaleFactor(2.0f, -2.0f, 2.0f);
    glm::vec3 translationVector(0.0f, -0.5f, 0.0f);

    // Camera
    Camera camera;
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    //Sun
    uniformSun.viewport = createViewportMatrix();
    uniformSun.projection = createProjectionMatrix();

    //Earth
    uniformEarth.viewport = createViewportMatrix();
    uniformEarth.projection = createProjectionMatrix();

    //Neptune
    uniformNeptune.viewport = createViewportMatrix();
    uniformNeptune.projection = createProjectionMatrix();

    //Moon
    uniformMoon.viewport = createViewportMatrix();
    uniformMoon.projection = createProjectionMatrix();

    //Ship
    uniformShip.viewport = createViewportMatrix();
    uniformShip.projection = createProjectionMatrix();


    auto lastTime = high_resolution_clock::now();

    Uint32 frameStart, frameTime;
    std::string title = "FPS: ";
    float rotSpeed = M_PI;
    camera.targetPosition = glm::vec3(camera.cameraPosition.x + 2.0f * sin(rotSpeed), 0.0f, camera.cameraPosition.z + 2.0f * cos(rotSpeed));
    addRandomStaticWhitePixels();

    while (running) {
        frameStart = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        camera.cameraPosition.x += 0.8f * sin(rotSpeed);
                        camera.cameraPosition.z += 0.8f * cos(rotSpeed);
                        camera.targetPosition.x += 0.8f * sin(rotSpeed);
                        camera.targetPosition.z += 0.8f * cos(rotSpeed);
                        break;
                    case SDLK_a:
                        rotSpeed += 0.05;
                        camera.targetPosition = glm::vec3(camera.cameraPosition.x + 2.0f * sin(rotSpeed), 0.0f, camera.cameraPosition.z + 2.0f * cos(rotSpeed));
                        break;
                    case SDLK_s:
                        camera.cameraPosition.x -= 0.8f * sin(rotSpeed);
                        camera.cameraPosition.z -= 0.8f * cos(rotSpeed);
                        camera.targetPosition.x -= 0.8f * sin(rotSpeed);
                        camera.targetPosition.z -= 0.8f * cos(rotSpeed);
                        break;
                    case SDLK_d:
                        rotSpeed -= 0.05;
                        camera.targetPosition = glm::vec3(camera.cameraPosition.x + 2.0f * sin(rotSpeed), 0.0f, camera.cameraPosition.z + 2.0f * cos(rotSpeed));
                        break;
                }
            }

        }

        glm::mat4 view = glm::lookAt(
                camera.cameraPosition, // The position of the camera
                camera.targetPosition, // The point the camera is looking at
                camera.upVector
        );


        // Calculate the difference vector
        glm::vec3 spaceShipPos = camera.targetPosition;
        spaceShipPos.y += 0.1f;

        // Clear the framebuffer
        clear(framebuffer, zbuffer);
        copyStaticPixelsToFramebuffer(framebuffer);

        a += 0.5;
        b += 0.8;

        //Sun
        uniformSun.model = createModeltMatrix(scaleFactor, translationVector, a);
        uniformSun.view = view;

        glm::mat4 rotation = glm::rotate(glm::mat4(1),glm::radians(20.0f),glm::vec3(0,0,1));
        uniformShip.model = createModeltMatrix(scaleFactor*0.07f, spaceShipPos, glm::degrees(rotSpeed-(M_PI/2))) * rotation;
        uniformShip.view = view;

        #pragma omp parallel for
        for(int i = 0; i<4; i++) {
            if(i==0){
                render(resultVertexArray, uniformSun, fragmentShaderSun, translationVector);
            }
            
            else if(i==1) {

                glm::vec3 earthTranslationVector(2.0f * std::cos(glm::radians(a)), 0, 2.0f * std::sin(glm::radians(a)));
                uniformEarth.model = createModeltMatrix(scaleFactor*0.2f, translationVector+earthTranslationVector, a);
                uniformEarth.view = view;

                //Moon
                glm::vec3 moonTranslationVector(0.5f * std::cos(glm::radians(b)), 0, 0.5f * std::sin(glm::radians(b)));
                uniformMoon.model = createModeltMatrix(scaleFactor*0.1f, translationVector+moonTranslationVector+earthTranslationVector, a);
                uniformMoon.view = view;
                render(resultVertexArray, uniformEarth, fragmentShaderEarth, translationVector+earthTranslationVector);
                render(resultVertexArray, uniformMoon, fragmentShaderMoon, translationVector+moonTranslationVector+earthTranslationVector);

            } else if (i==2) {
                //Neptune
                glm::vec3 neptuneTranslationVector(3.0f * std::cos(glm::radians(a)), 0, 3.0f * std::sin(glm::radians(a)));
                uniformNeptune.model = createModeltMatrix(scaleFactor*0.3f, translationVector+neptuneTranslationVector, a);
                uniformNeptune.view = view;
                render(resultVertexArray, uniformNeptune, fragmentShaderNeptune, translationVector+neptuneTranslationVector);

            } else if (i==3) {
                render(resultVertexArrayShip, uniformShip, fragmentShader, -((translationVector + spaceShipPos) + camera.cameraPosition));
            }
        }

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
