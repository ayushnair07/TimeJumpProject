#include "Terrain.h"
#include <stb_image.h>
#include <iostream>

// simple single-channel loader assumes stb_image is included in project
Terrain::Terrain() {}
Terrain::~Terrain() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (textureID) glDeleteTextures(1, &textureID);
}

bool Terrain::Load(const std::string& heightmapPath, const std::string& texturePath,
    float heightScale, float size)
{
    int comp;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(heightmapPath.c_str(), &width, &height, &comp, 1);
    if (!data) {
        std::cerr << "Terrain: failed to load heightmap: " << heightmapPath << "\n";
        return false;
    }
    if (!BuildFromImage(data, width, height, 1, heightScale, size)) {
        stbi_image_free(data); return false;
    }
    stbi_image_free(data);

    // load texture (albedo)
    int tw, th, tc;
    unsigned char* tdata = stbi_load(texturePath.c_str(), &tw, &th, &tc, 0);
    if (!tdata) { std::cerr << "Terrain: failed to load texture: " << texturePath << "\n"; return false; }
    GLenum fmt = (tc == 3 ? GL_RGB : GL_RGBA);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, tw, th, 0, fmt, GL_UNSIGNED_BYTE, tdata);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(tdata);

    // create VAO/VBO/EBO buffer interleaved: pos(3), normal(3), uv(2)
    struct Vertex { glm::vec3 p; glm::vec3 n; glm::vec2 uv; };
    std::vector<Vertex> verts;
    verts.reserve(positions.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        verts.push_back({ positions[i], normals[i], uvs[i] });
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // layout
    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
    glEnableVertexAttribArray(2); // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    glBindVertexArray(0);
    return true;
}

bool Terrain::BuildFromImage(unsigned char* data, int w, int h, int channels,
    float heightScale, float size)
{
    if (!data || w <= 1 || h <= 1) return false;
    positions.clear(); normals.clear(); uvs.clear(); indices.clear();

    // generate grid
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            float sx = (float)i / (w - 1);
            float sz = (float)j / (h - 1);
            float x = (sx - 0.5f) * size;
            float z = (sz - 0.5f) * size;
            unsigned char v = data[j * w + i]; // 0..255
            float y = (v / 255.0f) * heightScale;
            positions.push_back(glm::vec3(x, y, z));
            uvs.push_back(glm::vec2(sx * 10.0f, sz * 10.0f)); // tiled UV
            normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // temp
        }
    }
    // indices (triangles)
    for (int j = 0; j < h - 1; ++j) {
        for (int i = 0; i < w - 1; ++i) {
            unsigned int a = j * w + i;
            unsigned int b = j * w + (i + 1);
            unsigned int c = (j + 1) * w + i;
            unsigned int d = (j + 1) * w + (i + 1);

            // triangle 1: a, c, b  (CCW)
            indices.push_back(a); indices.push_back(c); indices.push_back(b);
            // triangle 2: b, c, d  (CCW)
            indices.push_back(b); indices.push_back(c); indices.push_back(d);
        }
    }

    ComputeNormals();
    return true;
}

void Terrain::ComputeNormals() {
    size_t vcount = positions.size();
    normals.assign(vcount, glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int ia = indices[i + 0], ib = indices[i + 1], ic = indices[i + 2];
        glm::vec3 A = positions[ia], B = positions[ib], C = positions[ic];
        glm::vec3 N = glm::normalize(glm::cross(B - A, C - A));
        normals[ia] += N; normals[ib] += N; normals[ic] += N;
    }
    for (size_t i = 0; i < normals.size(); ++i) normals[i] = glm::normalize(normals[i]);
}

void Terrain::Draw() {
    if (textureID) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
