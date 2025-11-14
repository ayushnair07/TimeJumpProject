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

/// Loads OBJ (using tinyobj) into a single VAO/EBO and builds material list & submesh index ranges.
/// objPath: path to .obj file
/// outMesh: filled with GL objects and metadata (caller must glDelete* when appropriate)
/// workingFolderFallback: optional folder to use to find .mtl / textures if obj doesn't contain path
bool LoadOBJWithMaterials(const std::string& objPath, MeshGL_Model& outMesh, const std::string& workingFolderFallback = "");

// Load a 2D texture (PNG/JPG). Returns 0 on failure and sets outHasAlpha.
GLuint LoadTexture2D(const std::string& path, bool& outHasAlpha);
