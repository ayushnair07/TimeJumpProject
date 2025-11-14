#pragma once

// Make sure GL loader provides GLuint etc (glad recommended)
#include <glad/glad.h>

#include <vector>
#include <glm/glm.hpp>
#include "ModelLoader.h"


class Terrain; // forward declare your Terrain class

class TreeInstancer {
public:
    TreeInstancer();
    ~TreeInstancer();

    // generate 'count' model matrices randomly inside bounds (minX..maxX, minZ..maxZ)
    // uses Terrain::GetHeightAt(x,z) to place trees on terrain. Provide min/max scale.
    void GenerateInstances(int count,
        float minX, float maxX,
        float minZ, float maxZ,
        const Terrain& terrain,
        float minScale = 0.8f, float maxScale = 1.4f);

    // uploads mats[] to GPU (creates instanceVBO if needed)
    void UploadInstancesToGPU(const MeshGL_Model& mesh);

    // draw the mesh with instancing (assumes shader already in use and mesh bound)
    // instanceCount = number of instances generated
    void DrawInstanced(const MeshGL_Model& mesh, int instanceCount);

    // optional: clear CPU-side mats
    void Clear() { mats.clear(); }

private:
    std::vector<glm::mat4> mats;
    GLuint instanceVBO = 0;   // declared here (fixes 'undeclared identifier' errors)
};
