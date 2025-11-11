// main.cpp
// Complete example main program that initializes GLFW+GLAD, loads terrain, and
// runs an auto-follow camera which can be toggled to free camera with 'F'.
// Assumes the following helper classes exist in your project:
//  - Shader (methods: LoadFromFiles(pathVert, pathFrag), Use(), SetMat4, SetVec3, SetInt)
//  - Camera (members/methods shown in previous snippets: mode, UpdateAuto(t), GetViewMatrix(), GetProjection(aspect), ProcessMouseMovement, ProcessKeyboard, pos/front)
//  - Terrain (methods: Load(heightmapPath, texturePath, heightScale, size), Draw(), model)
// If your helper class method names differ, adapt calls accordingly.

#include <iostream>
#include <filesystem>
#include <string>
#include <chrono>

// GL / windowing
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// helpers (make sure these headers exist in your src/)
#include "Shader.h"
#include "Camera.h"
#include "Terrain.h"


// Window size
int WIN_W = 1280;
int WIN_H = 720;

// globals
GLFWwindow* gWindow = nullptr;
Camera gCamera;
bool firstMouse = true;
double lastX = 0.0, lastY = 0.0;
float lastFrameTime = 0.0f;
float globalTime = 0.0f;

// input state for free camera movement
bool keysDown[1024] = { false };

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    WIN_W = w; WIN_H = h;
    glViewport(0, 0, w, h);
}

std::string GetResourcePath(const std::string& relativePath) {
    // Start from where the exe is running
    std::filesystem::path exeDir = std::filesystem::current_path();

    // Go upward until we find a "resources" folder
    for (int i = 0; i < 5; ++i) { // go up to 5 levels up, just in case
        if (std::filesystem::exists(exeDir / "resources")) {
            return (exeDir / relativePath).string();
        }
        exeDir = exeDir.parent_path();
    }

    // Fallback — assume project root
    std::filesystem::path fallback = std::filesystem::current_path() / ".." / relativePath;
    return std::filesystem::absolute(fallback).string();
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (gCamera.mode != CamMode::FREE) {
        // keep firstMouse true so when switching to free cam we don't have big jump
        firstMouse = true;
        return;
    }

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float dx = float(xpos - lastX);
    float dy = float(lastY - ypos); // reversed: y ranges top->bottom
    lastX = xpos;
    lastY = ypos;

    gCamera.ProcessMouseMovement(dx, dy);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // toggle camera mode on 'F' press
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        gCamera.mode = (gCamera.mode == CamMode::AUTO) ? CamMode::FREE : CamMode::AUTO;
        // lock / unlock cursor depending on mode
        if (gCamera.mode == CamMode::FREE) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true;
        }
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keysDown[key] = true;
        if (action == GLFW_RELEASE) keysDown[key] = false;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void process_free_camera_input(float dt) {
    if (gCamera.mode != CamMode::FREE) return;
    if (keysDown[GLFW_KEY_W]) gCamera.ProcessKeyboard('W', dt);
    if (keysDown[GLFW_KEY_S]) gCamera.ProcessKeyboard('S', dt);
    if (keysDown[GLFW_KEY_A]) gCamera.ProcessKeyboard('A', dt);
    if (keysDown[GLFW_KEY_D]) gCamera.ProcessKeyboard('D', dt);
    // you can add Q/E for up/down if needed:
    if (keysDown[GLFW_KEY_Q]) gCamera.pos.y -= gCamera.speed * dt;
    if (keysDown[GLFW_KEY_E]) gCamera.pos.y += gCamera.speed * dt;
}

int main() {
    // ---------- Initialize GLFW ----------
    if (!glfwInit()) {
        std::cerr << "ERROR: GLFW init failed\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    gWindow = glfwCreateWindow(WIN_W, WIN_H, "TimeJump - Terrain Demo", nullptr, nullptr);
    if (!gWindow) {
        std::cerr << "ERROR: Window creation failed\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(gWindow);

    // Load GL functions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "ERROR: Failed to initialize GLAD\n";
        return -1;
    }

    // callbacks
    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetCursorPosCallback(gWindow, mouse_callback);
    glfwSetKeyCallback(gWindow, key_callback);

    // set OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Print renderer info
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n";

    // ---------- Create / load shaders ----------
    Shader terrainShader;
    bool ok = terrainShader.LoadFromFiles(
        GetResourcePath("resources/shaders/terrain.vert"),
        GetResourcePath("resources/shaders/terrain.frag")
    );

    if (!ok) {
        std::cerr << "ERROR: Failed to load terrain shader files. Check paths.\n";
        // continue so user can fix
    }

    // ---------- Load terrain ----------
    Terrain terrain;
    // heightmap and albedo textures: adjust paths to where you placed them in the repo
    const std::string heightmapPath = "resources/textures/heightmap.png";
    const std::string terrainTexPath = "resources/textures/grass.jpg";

    if (!terrain.Load(
        GetResourcePath("resources/textures/heightmap.png"),
        GetResourcePath("resources/textures/grass.jpg"),
        25.0f, 200.0f
    )) {
        std::cerr << "ERROR: Terrain failed to load. Make sure files exist at:\n"
            << "  " << heightmapPath << "\n"
            << "  " << terrainTexPath << "\n";
        // don't exit — give debugging info
    }

    // Place terrain a bit lower if needed
    terrain.model = glm::translate(terrain.model, glm::vec3(0.0f, -1.0f, 0.0f));

    // ---------- Camera initial settings ----------
    gCamera.mode = CamMode::AUTO;
    // initial cam pos is set inside camera default; you can override:
    gCamera.pos = glm::vec3(0.0f, 30.0f, 80.0f);

    // timing
    auto startTime = std::chrono::high_resolution_clock::now();
    lastFrameTime = 0.0f;

    // Main loop
    while (!glfwWindowShouldClose(gWindow)) {
        // timing
        auto now = std::chrono::high_resolution_clock::now();
        float t = std::chrono::duration<float>(now - startTime).count();
        float delta = t - lastFrameTime;
        lastFrameTime = t;
        globalTime = t;

        // handle input
        process_free_camera_input(delta);

        // auto camera updates (if auto)
        if (gCamera.mode == CamMode::AUTO) {
            gCamera.UpdateAuto(globalTime);
        }

        // clear
        glViewport(0, 0, WIN_W, WIN_H);
        glClearColor(0.53f, 0.80f, 0.92f, 1.0f); // sky-like background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // prepare matrices
        float aspect = (float)WIN_W / (float)WIN_H;
        glm::mat4 view = gCamera.GetViewMatrix();
        glm::mat4 proj = gCamera.GetProjection(aspect);

        // ---------- Render terrain ----------
        terrainShader.Use();
        terrainShader.SetMat4("uView", view);
        terrainShader.SetMat4("uProj", proj);
        terrainShader.SetMat4("uModel", terrain.model);

        // simple moving sun position to show dynamic lighting (optional)
        glm::vec3 sunPos = glm::vec3(100.0f * cos(globalTime * 0.1f), 80.0f + 50.0f * sin(globalTime * 0.1f), 60.0f * sin(globalTime * 0.1f));
        terrainShader.SetVec3("lightPos", sunPos);
        terrainShader.SetVec3("viewPos", gCamera.pos);
        terrainShader.SetVec3("lightColor", glm::vec3(1.0f, 0.98f, 0.9f));
        terrainShader.SetVec3("ambientColor", glm::vec3(0.25f, 0.25f, 0.25f));
        terrainShader.SetInt("uTex", 0);

        terrain.Draw();

        // optional: draw debug info (sun marker) - not implemented here

        // swap buffers / poll
        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }

    // cleanup and exit
    glfwTerminate();
    return 0;
}

