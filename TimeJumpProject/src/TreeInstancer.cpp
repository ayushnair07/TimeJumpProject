#include "TreeInstancer.h"
#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Include your Terrain header so the compiler can find Terrain::GetHeightAt
#include "Terrain.h"   // adjust path as needed

TreeInstancer::TreeInstancer() : instanceVBO(0) {}
TreeInstancer::~TreeInstancer() {
    if (instanceVBO) {
        glDeleteBuffers(1, &instanceVBO);
        instanceVBO = 0;
    }
}

void TreeInstancer::GenerateInstances(int count,
    float minX, float maxX,
    float minZ, float maxZ,
    const Terrain& terrain,
    float minScale, float maxScale)
{
    mats.clear();
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> distX(minX, maxX);
    std::uniform_real_distribution<float> distZ(minZ, maxZ);
    std::uniform_real_distribution<float> distScale(minScale, maxScale);
    std::uniform_real_distribution<float> distRot(0.0f, glm::two_pi<float>());

    for (int i = 0; i < count; ++i) {
        float x = distX(rng);
        float z = distZ(rng);
        float y = terrain.GetHeightAt(x, z); // make sure Terrain has this method public
        if (!std::isfinite(y)) continue;      // skip if terrain returned invalid height

        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(x, y, z));
        float rot = distRot(rng);
        M = glm::rotate(M, rot, glm::vec3(0, 1, 0));
        float s = distScale(rng);
        M = glm::scale(M, glm::vec3(s));
        mats.push_back(M);
    }
}

void TreeInstancer::UploadInstancesToGPU(const MeshGL_Model& mesh) {
    if (instanceVBO == 0) glGenBuffers(1, &instanceVBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, mats.size() * sizeof(glm::mat4), mats.data(), GL_STATIC_DRAW);

    // set attribute locations 3,4,5,6 (4 vec4 per mat)
    std::size_t vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
        glVertexAttribDivisor(3 + i, 1);
    }

    glBindVertexArray(0);
}

void TreeInstancer::DrawInstanced(const MeshGL_Model& mesh, int instanceCount)
{
    if (instanceCount <= 0) return;
    if (mesh.VAO == 0 || mesh.indexCount == 0) return;

    glBindVertexArray(mesh.VAO);

    // Use simple instanced draw. This is widely supported:
    glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0, instanceCount);

    glBindVertexArray(0);
}
