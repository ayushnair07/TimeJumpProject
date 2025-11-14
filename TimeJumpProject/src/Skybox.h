#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>

class Skybox {
public:
    Skybox() = default;
    ~Skybox();
    // faces: order -> right, left, top, bottom, front, back
    bool Load(const std::vector<std::string>& faces);
    void Draw(unsigned int shaderID, GLuint overrideCubemap = 0); // shader should be bound and have samplerCube=0
    GLuint getCubemapID() const { return cubemapTex; }
private:
    unsigned int cubemapTex = 0;
    unsigned int VAO = 0, VBO = 0;
    bool BuildCube();
};
