#include "EnvSphere.h"
#include <cmath>
#include <glm/gtc/constants.hpp>

MeshGL CreateUVSphere(int latSegments, int longSegments, float radius) {
    MeshGL mesh;
    std::vector<float> verts; // pos.x, pos.y, pos.z, normal.x, normal.y, normal.z
    std::vector<unsigned int> inds;

    for (int y = 0; y <= latSegments; ++y) {
        float v = (float)y / (float)latSegments;
        float theta = v * glm::pi<float>(); // 0..pi
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int x = 0; x <= longSegments; ++x) {
            float u = (float)x / (float)longSegments;
            float phi = u * glm::two_pi<float>(); // 0..2pi
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float px = radius * sinTheta * cosPhi;
            float py = radius * cosTheta;
            float pz = radius * sinTheta * sinPhi;

            glm::vec3 pos(px, py, pz);
            glm::vec3 n = glm::normalize(pos);

            verts.push_back(pos.x);
            verts.push_back(pos.y);
            verts.push_back(pos.z);
            verts.push_back(n.x);
            verts.push_back(n.y);
            verts.push_back(n.z);
        }
    }

    // indices
    for (int y = 0; y < latSegments; ++y) {
        for (int x = 0; x < longSegments; ++x) {
            int a = y * (longSegments + 1) + x;
            int b = a + longSegments + 1;
            inds.push_back(a);
            inds.push_back(b);
            inds.push_back(a + 1);

            inds.push_back(a + 1);
            inds.push_back(b);
            inds.push_back(b + 1);
        }
    }

    // create GL buffers
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);

    // pos (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));

    // normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    mesh.indexCount = (GLsizei)inds.size();
    return mesh;
}

MeshGL CreateScreenQuad() {
    MeshGL mesh;
    // pos(2), uv(2)
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    unsigned int quadIndices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    mesh.indexCount = 6;
    return mesh;
}

void DestroyMesh(MeshGL& m) {
    if (m.EBO) { glDeleteBuffers(1, &m.EBO); m.EBO = 0; }
    if (m.VBO) { glDeleteBuffers(1, &m.VBO); m.VBO = 0; }
    if (m.VAO) { glDeleteVertexArrays(1, &m.VAO); m.VAO = 0; }
    m.indexCount = 0;
}
