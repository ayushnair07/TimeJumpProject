// main.cpp


#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "Terrain.h"
#include "Skybox.h"
#include "Camera.h"
#include "TextRenderer.h"
#include "EnvSphere.h"

#include "ModelLoader.h"
#include "TreeInstancer.h"

// ---------- Globals ----------
int WIN_W = 1280;
int WIN_H = 720;
GLFWwindow* gWindow = nullptr;

Camera gCamera;
float lastFrameTime = 0.0f;
float globalTime = 0.0f;

// Post-process & time-jump globals
GLuint sceneFBO = 0;
GLuint sceneColorTex = 0;
GLuint sceneDepthRBO = 0;
GLuint quadVAO = 0, quadVBO = 0;
Shader postShader;
float jumpAnim = 0.0f;   
float jumpTarget = 0.0f; 
bool timeJumpOn = false; 
// alt sky & textures

Skybox skyDead;  // new alternative sky
GLuint sandTexture = 0;
GLuint terrainDefaultTex = 0; // store original terrain texture id

Shader terrainShader, skyShader;
Terrain terrain;
Skybox sky;
bool okTerrain = false, okTerrainShader = false, okSkyShader = false;
unsigned int blackTex = 0;

MeshGL sunSphere;
Shader sunShader;

MeshGL reflectiveSphere;
Shader envShader;

MeshGL_Model treeMesh;
GLuint treeDiffuse = 0;
TreeInstancer treeInstancer;
GLsizei treeInstanceCount = 0;
Shader treeShader;

// HUD
TextRenderer ui;

// forward declarations of callbacks (defined below)
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


struct SpotState {
    glm::vec3 position = glm::vec3(0.0f, 40.0f, 0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    float innerAngleDeg = 12.0f;
    float outerAngleDeg = 18.0f;
    glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.8f);
    float intensity = 6.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    bool enabled = true;
};

// single global spotlight you can toggle / move
static SpotState gSpot;

static void UploadSpotToShader(Shader& shader, const std::string& name, const SpotState& st) {
    // assume shader.Use() already called
    shader.SetVec3(name + ".position", st.position);
    shader.SetVec3(name + ".direction", glm::normalize(st.direction));
    shader.SetFloat(name + ".cutOff", cos(glm::radians(st.innerAngleDeg)));
    shader.SetFloat(name + ".outerCutOff", cos(glm::radians(st.outerAngleDeg)));
    shader.SetVec3(name + ".color", st.color);
    shader.SetFloat(name + ".intensity", st.intensity);
    shader.SetFloat(name + ".constant", st.constant);
    shader.SetFloat(name + ".linear", st.linear);
    shader.SetFloat(name + ".quadratic", st.quadratic);
    shader.SetInt(name + ".enabled", st.enabled ? 1 : 0);
}

// ----------------- Helper: find resources folder -----------------
static std::string GetResourcePath(const std::string& relativePath) {
    namespace fs = std::filesystem;
    fs::path dir = fs::current_path();
    for (int i = 0; i < 6; ++i) {
        fs::path candidate = dir / "resources";
        if (fs::exists(candidate) && fs::is_directory(candidate)) {
            return (dir / relativePath).lexically_normal().string();
        }
        if (dir.has_parent_path()) dir = dir.parent_path();
        else break;
    }
    fs::path fallback = fs::current_path() / relativePath;
    return fs::absolute(fallback).lexically_normal().string();
}
static void CreatePostResources(int width, int height) {
    // delete old if present
    if (sceneColorTex) { glDeleteTextures(1, &sceneColorTex); sceneColorTex = 0; }
    if (sceneDepthRBO) { glDeleteRenderbuffers(1, &sceneDepthRBO); sceneDepthRBO = 0; }
    if (sceneFBO) { glDeleteFramebuffers(1, &sceneFBO); sceneFBO = 0; }

    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneColorTex);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex, 0);

    glGenRenderbuffers(1, &sceneDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: scene FBO not complete\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Fullscreen quad (create once)
    if (quadVAO == 0) {
        float quadVerts[] = {
            // pos.xy   // uv.xy
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }
}
// ----------------- Input helpers (reuse your own if present) -----------------
static bool keysDown[1024] = { false };
void process_free_camera_input(float dt) {
    if (keysDown[GLFW_KEY_T]) {
                   // toggle jump pulse
            timeJumpOn = !timeJumpOn;
        jumpTarget = timeJumpOn ? 1.0f : 0.0f;
                  // (we will swap textures mid-animation in the main loop)
            
    }

    if (gCamera.mode != CamMode::FREE) return;
    if (keysDown[GLFW_KEY_W]) gCamera.ProcessKeyboard('W', dt);
    if (keysDown[GLFW_KEY_S]) gCamera.ProcessKeyboard('S', dt);
    if (keysDown[GLFW_KEY_A]) gCamera.ProcessKeyboard('A', dt);
    if (keysDown[GLFW_KEY_D]) gCamera.ProcessKeyboard('D', dt);
    if (keysDown[GLFW_KEY_Q]) gCamera.pos.y -= gCamera.speed * dt;
    if (keysDown[GLFW_KEY_E]) gCamera.pos.y += gCamera.speed * dt;
}

// ----------------- Main -----------------
int main() {
    // ---------- Init GLFW + GLAD ----------
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "ERROR: Failed to initialize GLAD\n";
        return -1;
    }

    // callbacks
    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetCursorPosCallback(gWindow, mouse_callback);
    glfwSetKeyCallback(gWindow, key_callback);

    // GL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // reduce cubemap seams

    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    // ---------- Load / compile shaders ----------
    okTerrainShader = terrainShader.LoadFromFiles(
        GetResourcePath("resources/shaders/terrain.vert"),
        GetResourcePath("resources/shaders/terrain.frag")
    );
    if (!okTerrainShader) std::cerr << "ERROR: terrain shader failed\n";

    okSkyShader = skyShader.LoadFromFiles(
        GetResourcePath("resources/shaders/skybox.vert"),
        GetResourcePath("resources/shaders/skybox.frag")
    );
    if (!okSkyShader) std::cerr << "ERROR: sky shader failed\n";

    bool okEnvShader = envShader.LoadFromFiles(
        GetResourcePath("resources/shaders/env_vert.glsl"),
        GetResourcePath("resources/shaders/env_frag.glsl")
    );
    if (!okEnvShader) std::cerr << "ERROR: env shader failed\n";

    // ---------- Load terrain ----------
    okTerrain = terrain.Load(
        GetResourcePath("resources/textures/heightmap.png"),
        GetResourcePath("resources/textures/grass.jpg"),
        25.0f, 400.0f
    );
    if (!okTerrain) std::cerr << "ERROR: terrain failed to load\n";


    MeshGL_Model treeMesh;
    if (!LoadOBJWithMaterials(GetResourcePath("resources/models/Tree1.obj"), treeMesh)) {
        std::cerr << "Failed to load Tree1.obj\n";
    }
    else {
        std::cout << "Tree loaded. submeshes: " << treeMesh.submeshes.size()
            << " materials: " << treeMesh.materials.size() << "\n";
    }


    treeShader.LoadFromFiles(GetResourcePath("resources/shaders/tree_inst.vert"), GetResourcePath("resources/shaders/tree_inst.frag"));
    GLuint treeShaderID = treeShader.ID;


    TreeInstancer inst;
    TreeInstancer instancer;
    inst.GenerateInstances(100, -90.0f, 90.0f, -90.0f, 90.0f, terrain, 0.4f, 1.0f);
    inst.UploadInstancesToGPU(treeMesh);

 


    // ---------- Load skybox ----------
    std::vector<std::string> faces = {
        GetResourcePath("resources/skybox/right.png"),
        GetResourcePath("resources/skybox/left.png"),
        GetResourcePath("resources/skybox/top.png"),
        GetResourcePath("resources/skybox/bottom.png"),
        GetResourcePath("resources/skybox/front.png"),
        GetResourcePath("resources/skybox/back.png")
    };
    if (!sky.Load(faces)) {
        std::cerr << "Warning: Skybox failed to load faces\n";
    }

    terrainDefaultTex = terrain.GetTexture();
    bool dummyAlpha = false;
    sandTexture = LoadTexture2D(GetResourcePath("resources/textures/sand.jpg"), dummyAlpha);
    if (!sandTexture) {
        std::cerr << "Warning: failed to load sand texture\n";
        
    }

     std::vector<std::string> deadFaces = {
GetResourcePath("resources/skybox_dry/right.png"),
GetResourcePath("resources/skybox_dry/left.png"),
GetResourcePath("resources/skybox_dry/top.png"),
GetResourcePath("resources/skybox_dry/bottom.png"),
GetResourcePath("resources/skybox_dry/front.png"),
GetResourcePath("resources/skybox_dry/back.png")
 };
    if (!skyDead.Load(deadFaces)) {
        std::cerr << "Warning: failed to load alternate skybox (skybox_dry)";
        
    }

    bool okSunShader = sunShader.LoadFromFiles(
        GetResourcePath("resources/shaders/sun.vert"),
        GetResourcePath("resources/shaders/sun.frag")
    );
    if (!okSunShader) std::cerr << "ERROR: sun shader failed\n";

    CreatePostResources(WIN_W, WIN_H);
    postShader.LoadFromFiles(GetResourcePath("resources/shaders/post.vs"),
        GetResourcePath("resources/shaders/post.fs"));

    // ---------- Create env-mapped sphere ----------
    reflectiveSphere = CreateUVSphere(48, 48, 6.0f);
    // initial placement: above terrain so it's visible (adjust later to sample terrain height)
    reflectiveSphere.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 40.0f, 0.0f));
    reflectiveSphere.model = glm::scale(reflectiveSphere.model, glm::vec3(2.0f));
    sunSphere = CreateUVSphere(24, 24, 15.0f);

    // ---------- Camera ----------
    gCamera.mode = CamMode::AUTO;
    gCamera.pos = glm::vec3(0.0f, 30.0f, 80.0f);

    // ---------- Small black texture fallback ----------
    {
        unsigned char px[3] = { 0, 0, 0 };
        glGenTextures(1, &blackTex);
        glBindTexture(GL_TEXTURE_2D, blackTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Example: attach spot to camera: place this after gCamera has been set up
    gSpot.position = gCamera.pos + glm::vec3(0.0f, 0.5f, 0.0f);   // just above camera
    gSpot.direction = gCamera.front; // if your camera has a 'front' vector
    gSpot.enabled = true;


    //bool uiReady = false;
    //if (ui.Init(WIN_W, WIN_H)) {
    //    uiReady = ui.LoadFont(GetResourcePath("resources/fonts/atlas.png"), 16, 16);
    //    if (!uiReady) std::cerr << "Warning: font atlas missing - HUD disabled\n";
    //}

    // timing
    auto startTime = std::chrono::high_resolution_clock::now();
    lastFrameTime = 0.0f;

    // FPS smoothing
    double fpsTimer = 0.0;
    int fpsFrames = 0;
    double fpsValue = 0.0;

    // ---------- Main loop ----------
    while (!glfwWindowShouldClose(gWindow)) {
        // timing
        auto now = std::chrono::high_resolution_clock::now();
        float t = std::chrono::duration<float>(now - startTime).count();
        float delta = t - lastFrameTime;
        lastFrameTime = t;
        globalTime = t;

        // input + camera update
        process_free_camera_input(delta);
        if (gCamera.mode == CamMode::AUTO) gCamera.UpdateAuto(globalTime);

        // clear buffers
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glViewport(0, 0, WIN_W, WIN_H);
        glClearColor(0.53f, 0.80f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // prepare matrices
        float aspect = (float)WIN_W / (float)WIN_H;
        glm::mat4 view = gCamera.GetViewMatrix();
        glm::mat4 proj = gCamera.GetProjection(aspect);

        // ---------- Day / Night ----------
        const float dayLength = 180.0f;
        float cycle = fmod(globalTime / dayLength, 1.0f);
        float sunAngle = cycle * glm::two_pi<float>();
        glm::vec3 sunDir = glm::normalize(glm::vec3(cos(sunAngle), sin(sunAngle), 0.25f));
        glm::vec3 sunPos = sunDir * 500.0f;
        float dayFactor = glm::clamp((sunDir.y + 0.1f) / 1.1f, 0.0f, 1.0f);

        glm::vec3 daySunColor = glm::vec3(1.0f, 0.95f, 0.85f);
        glm::vec3 nightSunColor = glm::vec3(0.05f, 0.08f, 0.2f);
        glm::vec3 sunColor = glm::mix(nightSunColor, daySunColor, dayFactor);

        glm::vec3 sunObjectColor = glm::vec3(1.0f, 1.0f, 0.8f) * (dayFactor * 2.0f + 0.5f);

        glm::vec3 ambientDay = glm::vec3(0.25f);
        glm::vec3 ambientNight = glm::vec3(0.06f);
        glm::vec3 ambient = glm::mix(ambientNight, ambientDay, dayFactor);
        glm::vec3 moonTint = glm::vec3(0.02f, 0.03f, 0.06f);
        ambient += (1.0f - dayFactor) * moonTint * 0.8f;

        // ---------- Render opaque geometry (terrain, meshes) ----------
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        // Terrain
        if (okTerrainShader && okTerrain) {
            terrainShader.Use();
            terrainShader.SetMat4("uView", view);
            terrainShader.SetMat4("uProj", proj);
            terrainShader.SetMat4("uModel", terrain.model);

            bool attachSpotToCamera = true;
            if (attachSpotToCamera) {
                gSpot.position = gCamera.pos + glm::vec3(0.0f, 0.5f, 0.0f);
                gSpot.direction = gCamera.front; // adapt to your camera API
                UploadSpotToShader(terrainShader, "spot", gSpot);
            }
            
            terrainShader.SetVec3("lightPos", sunPos);
            terrainShader.SetVec3("viewPos", gCamera.pos);
            terrainShader.SetVec3("lightColor", sunColor);
            terrainShader.SetVec3("ambientColor", ambient);
            terrainShader.SetInt("uTex", 0);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, terrain.GetTexture());
            terrain.Draw();
            
        }

        // Environment-mapped reflective sphere
        if (okEnvShader) {
            envShader.Use();
            envShader.SetMat4("uView", view);
            envShader.SetMat4("uProj", proj);
            envShader.SetMat4("uModel", reflectiveSphere.model);

            envShader.SetVec3("viewPos", gCamera.pos);
            envShader.SetFloat("uDayFactor", dayFactor);
            envShader.SetVec3("baseColor", glm::vec3(0.95f, 0.85f, 0.7f));
            envShader.SetFloat("metal", 1.0f);
            envShader.SetFloat("roughness", 0.2f);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, sky.getCubemapID());
            envShader.SetInt("skybox", 1);

            glBindVertexArray(reflectiveSphere.VAO);
            glDrawElements(GL_TRIANGLES, reflectiveSphere.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }


         //---------- Skybox (draw last) ----------
        GLuint cubemapToUse = (timeJumpOn ? skyDead.getCubemapID() : sky.getCubemapID());

        if (okSkyShader) {
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);

            glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
            skyShader.Use();
            skyShader.SetMat4("uView", viewNoTrans);
            skyShader.SetMat4("uProj", proj);
            skyShader.SetFloat("uDayFactor", dayFactor);
            skyShader.SetInt("skybox", 0); // sampler unit 0

 
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapToUse);

            // now call Draw with overrideCubemap
            sky.Draw(skyShader.ID, cubemapToUse);

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }

        if (okSunShader && dayFactor > 0.0f) {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE); // Additive blend

            sunShader.Use();
            glm::mat4 sunModel = glm::translate(glm::mat4(1.0f), sunPos);
            sunShader.SetMat4("uModel", sunModel);
            sunShader.SetMat4("uView", view);
            sunShader.SetMat4("uProj", proj);
            sunShader.SetVec3("uSunColor", sunObjectColor);

            glBindVertexArray(sunSphere.VAO);
            glDrawElements(GL_TRIANGLES, sunSphere.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // Reset GL state
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDisable(GL_DEPTH_TEST);
        postShader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneColorTex);
        postShader.SetInt("uScene", 0);

        // animate jumpAnim to target smoothly
        float speed = 6.0f;
        jumpAnim = glm::mix(jumpAnim, jumpTarget, glm::clamp(delta * speed, 0.0f, 1.0f));

        // swap assets at mid-point to hide popping (Option B)
        static bool swappedAlready = false;
        if (jumpAnim > 0.5f && !swappedAlready && jumpTarget == 1.0f) {
            // perform swap ONCE mid-transition
            terrain.SetTexture(sandTexture);
            swappedAlready = true;
        }
        if (jumpAnim < 0.5f && swappedAlready && jumpTarget == 0.0f) {
            // revert when returning to normal
            terrain.SetTexture(terrainDefaultTex);
            swappedAlready = false;
        }

        postShader.SetFloat("uJump", jumpAnim);
        postShader.SetFloat("uTime", globalTime);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        // ---------- HUD (optional) ----------
        fpsFrames++;
        fpsTimer += delta;
        if (fpsTimer >= 0.5) {
            fpsValue = fpsFrames / fpsTimer;
            fpsFrames = 0;
            fpsTimer = 0.0;
        }

        //if (uiReady) {
        //    char buf[64];
        //    snprintf(buf, sizeof(buf), "FPS: %.0f", fpsValue);
        //    std::string fpsStr = buf;

        //    float scale = 1.0f;
        //    float charW = 8.0f * scale;
        //    float padding = 8.0f;
        //    float textWidth = (float)fpsStr.size() * charW;
        //    float x = (float)WIN_W - textWidth - padding;
        //    float y = padding + 2.0f;

        //    ui.RenderText(fpsStr, x, y, scale, glm::vec3(1.0f, 1.0f, 1.0f));

        //    std::vector<std::string> controls = {
        //        "Controls:",
        //        "F - toggle camera",
        //        "WASD - move (free)",
        //        "Q/E - up/down",
        //        "T - time jump"
        //    };
        //    float y2 = y + 18.0f;
        //    for (size_t i = 0; i < controls.size(); ++i) {
        //        ui.RenderText(controls[i], x, y2 + i * 14.0f, 0.8f, glm::vec3(0.95f, 0.9f, 0.6f));
        //    }
        //}

        //if (treeInstanceCount > 0) {

        //    glActiveTexture(GL_TEXTURE0);
        //    glBindTexture(GL_TEXTURE_2D, treeDiffuse);

        //    treeShader.Use();
        //    treeShader.SetMat4("uView", view);
        //    treeShader.SetMat4("uProj", proj);
        //    treeShader.SetVec3("lightPos", sunPos);
        //    treeShader.SetVec3("lightColor", sunColor);
        //    treeShader.SetVec3("ambientColor", ambient);
        //    treeShader.SetVec3("viewPos", gCamera.pos);
        //    treeShader.SetInt("uTex", 0);

        //    glEnable(GL_BLEND);
        //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //    inst.DrawInstanced(treeMesh,1);
        //    glDisable(GL_BLEND);
        //}

        // swap/poll
        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }

    // ---------- Cleanup ----------
    DestroyMesh(reflectiveSphere);
    DestroyMesh(sunSphere);

    if (blackTex) { glDeleteTextures(1, &blackTex); blackTex = 0; }

     if (sceneColorTex) { glDeleteTextures(1, &sceneColorTex); sceneColorTex = 0; }
     if (sceneDepthRBO) { glDeleteRenderbuffers(1, &sceneDepthRBO); sceneDepthRBO = 0; }
     if (sceneFBO) { glDeleteFramebuffers(1, &sceneFBO); sceneFBO = 0; }
     if (quadVBO) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
     if (quadVAO) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
        // delete alternate textures
        if (sandTexture) { glDeleteTextures(1, &sandTexture); sandTexture = 0; }
    
        glfwTerminate();
    return 0;
}



// ----------------- Callbacks -----------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    WIN_W = width; WIN_H = height;
    glViewport(0, 0, width, height);
    ui.Resize(WIN_W, WIN_H);
    CreatePostResources(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (gCamera.mode != CamMode::FREE) {
        // ignore mouse for auto camera
        return;
    }
    static bool firstMouse = true;
    static double lastX = 0.0, lastY = 0.0;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float dx = (float)(xpos - lastX);
    float dy = (float)(lastY - ypos);
    lastX = xpos; lastY = ypos;
    gCamera.ProcessMouseMovement(dx, dy);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // toggle camera mode with F
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        gCamera.mode = (gCamera.mode == CamMode::AUTO) ? CamMode::FREE : CamMode::AUTO;
        if (gCamera.mode == CamMode::FREE) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
    }

    // time jump (T) - placeholder (you can implement actual transition)
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        // example: fast-forward dayTime by half a cycle (instant swap)
        // You can replace with a proper transition effect later
        globalTime += 30.0f;
    }

    // record keys for free camera movement
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keysDown[key] = true;
        else if (action == GLFW_RELEASE) keysDown[key] = false;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}


