#include "Terrain.h"
#include <stb_image.h>
#include <iostream>
#include <cmath>


Terrain::Terrain() {}
Terrain::~Terrain() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (textureID) glDeleteTextures(1, &textureID);
}

bool Terrain::Load(const std::string& heightmapPath,
    const std::string& texturePath,
    float heightScale,
    float size)
{
    // --- load heightmap (grayscale) ---
    int comp = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(heightmapPath.c_str(), &width, &height, &comp, 1); // force 1 channel
    if (!data) {
        std::cerr << "Terrain: failed to load heightmap: " << heightmapPath << "\n";
        return false;
    }

    // store normalized height values for GetHeightAt()
    hmWidth = width;
    hmHeight = height;
    hmData.resize((size_t)hmWidth * (size_t)hmHeight);
    for (int i = 0; i < hmWidth * hmHeight; ++i) {
        hmData[i] = (float)data[i] / 255.0f;
    }

    // save scale/size (used by GetHeightAt())
    worldScaleY = heightScale;
    worldSizeX = size;
    worldSizeZ = size;

    // Build mesh from image (your existing helper)
    if (!BuildFromImage(data, width, height, 1, heightScale, size)) {
        stbi_image_free(data);
        return false;
    }
    stbi_image_free(data);

    // --- load albedo texture ---
    int tw = 0, th = 0, tc = 0;
    unsigned char* tdata = stbi_load(texturePath.c_str(), &tw, &th, &tc, 0);
    if (!tdata) {
        std::cerr << "Terrain: failed to load texture: " << texturePath << "\n";
        return false;
    }

    // Set pixel unpack alignment for safety (in case row stride not multiple of 4)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum extFormat = (tc == 3 ? GL_RGB : GL_RGBA);
    GLenum internalFormat = (tc == 3 ? GL_RGB8 : GL_RGBA8);

    if (textureID == 0) glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tw, th, 0, extFormat, GL_UNSIGNED_BYTE, tdata);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(tdata);

    // restore default alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // --- create VAO/VBO/EBO from positions/normals/uvs/indices ---
    struct Vertex { glm::vec3 p; glm::vec3 n; glm::vec2 uv; };
    std::vector<Vertex> verts;
    verts.reserve(positions.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        verts.push_back({ positions[i], normals[i], uvs[i] });
    }

    // Delete old buffers if they exist (optional, safe)
    if (VAO == 0) glGenVertexArrays(1, &VAO);
    if (VBO == 0) glGenBuffers(1, &VBO);
    if (EBO == 0) glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // layout: pos(0), normal(1), uv(2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
    glEnableVertexAttribArray(2);
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

float Terrain::GetHeightAt(float worldX, float worldZ) const {
    if (hmWidth <= 0 || hmHeight <= 0 || hmData.empty()) return NAN;


    float halfX = worldSizeX * 0.5f;
    float halfZ = worldSizeZ * 0.5f;

    // Convert world coords to local [0..1] uv over the terrain
    float u = (worldX + halfX) / worldSizeX; // 0..1
    float v = (worldZ + halfZ) / worldSizeZ; // 0..1

    // if outside, return NAN
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return NAN;

    // map to pixel space
    float fx = u * (hmWidth - 1);
    float fz = v * (hmHeight - 1);

    int x0 = (int)floor(fx);
    int z0 = (int)floor(fz);
    int x1 = std::min(x0 + 1, hmWidth - 1);
    int z1 = std::min(z0 + 1, hmHeight - 1);

    float sx = fx - (float)x0; // frac in x
    float sz = fz - (float)z0; // frac in z

    // sample four heights
    float h00 = hmData[z0 * hmWidth + x0];
    float h10 = hmData[z0 * hmWidth + x1];
    float h01 = hmData[z1 * hmWidth + x0];
    float h11 = hmData[z1 * hmWidth + x1];

    // bilinear interpolation
    float hx0 = h00 * (1.0f - sx) + h10 * sx;
    float hx1 = h01 * (1.0f - sx) + h11 * sx;
    float h = hx0 * (1.0f - sz) + hx1 * sz;

    // convert from normalized 0..1 to world Y units
    return h * worldScaleY;
}

