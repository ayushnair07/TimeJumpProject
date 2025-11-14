#pragma once
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>

class Terrain {
public:
    Terrain();
    ~Terrain();

    // Load a greyscale heightmap (path relative to exe or resources folder),
    // and an albedo texture for the terrain.
    bool Load(const std::string& heightmapPath, const std::string& texturePath,
        float heightScale = 20.0f, float size = 100.0f);

    void Draw(); // binds texture and draws mesh
    float GetHeightAt(float worldX, float worldZ) const;
    // optional transform
    glm::mat4 model = glm::mat4(1.0f);

    void SetTexture(GLuint tex) { textureID = tex; }
    GLuint GetTexture() const { return textureID; }

private:
    bool BuildFromImage(unsigned char* data, int w, int h, int channels,
        float heightScale, float size);
    void ComputeNormals();

    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int textureID = 0;
    int width = 0, height = 0;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> indices;
        int hmWidth = 0;
    int hmHeight = 0;
    std::vector<float> hmData; // row-major: hmData[row*hmWidth + col]
    float worldScaleY = 25.0f; // how heightmap values map to world Y
    float worldSizeX = 200.0f; // X dimension in world units (full terrain width)
    float worldSizeZ = 200.0f; // Z dimension in world units (full terrain depth)
};


