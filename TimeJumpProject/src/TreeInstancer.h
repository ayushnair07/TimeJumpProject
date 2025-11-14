#pragma once

// Make sure GL loader provides GLuint etc (glad recommended)
#include <glad/glad.h>

#include <vector>
#include <glm/glm.hpp>
#include "ModelLoader.h"


class Terrain;

class TreeInstancer {
public:
    TreeInstancer();
    ~TreeInstancer();


    void GenerateInstances(int count,
        float minX, float maxX,
        float minZ, float maxZ,
        const Terrain& terrain,
        float minScale = 0.8f, float maxScale = 1.4f);

    // uploads mats[] to GPU (creates instanceVBO if needed)
    void UploadInstancesToGPU(const MeshGL_Model& mesh);

    // instanceCount = number of instances generated
    void DrawInstanced(const MeshGL_Model& mesh, int instanceCount);

    // optional: clear CPU-side mats
    void Clear() { mats.clear(); }

private:
    std::vector<glm::mat4> mats;
    GLuint instanceVBO = 0;   // declared here (fixes 'undeclared identifier' errors)
};
