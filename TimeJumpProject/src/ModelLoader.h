#pragma once

// ModelLoader.h
#include <glad/glad.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// Simple submesh / material containers for GL
struct SubMeshRange {
    int materialId = -1;      // index into material list
    size_t indexOffset = 0;   // offset in indices (not bytes)
    size_t indexCount = 0;    // number of indices (triangles*3)
};

struct MaterialGL {
    std::string name;
    GLuint diffuseTex = 0;
    bool usesAlpha = false;
};

struct MeshGL_Model {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    size_t indexCount = 0;                     // total indices
    std::vector<SubMeshRange> submeshes;       // ranges by material / shape
    std::vector<MaterialGL> materials;         // materials
};


bool LoadOBJWithMaterials(const std::string& objPath, MeshGL_Model& outMesh, const std::string& workingFolderFallback = "");


GLuint LoadTexture2D(const std::string& path, bool& outHasAlpha);
