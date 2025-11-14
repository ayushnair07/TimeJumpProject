#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct MeshGL {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei indexCount = 0;
    glm::mat4 model = glm::mat4(1.0f);
};

MeshGL CreateUVSphere(int latSegments = 32, int longSegments = 32, float radius = 5.0f);
MeshGL CreateScreenQuad();
void DestroyMesh(MeshGL& m);
