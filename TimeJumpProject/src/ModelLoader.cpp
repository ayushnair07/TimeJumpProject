// ModelLoader.cpp
#include "tiny_obj_loader.h"

#include "stb_image.h"

#include "ModelLoader.h"

#include <iostream>
#include <filesystem>
#include <unordered_map>

static std::string GetFolder(const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) return p.parent_path().string() + "/";
    return "./";
}

GLuint LoadTexture2D(const std::string& path, bool& outHasAlpha) {
    outHasAlpha = false;
    if (path.empty()) return 0;
    stbi_set_flip_vertically_on_load(true);
    int w = 0, h = 0, comp = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 0);
    if (!data) {
        std::cerr << "LoadTexture2D: failed to load " << path << "\n";
        return 0;
    }
    GLenum fmt = (comp == 3) ? GL_RGB : GL_RGBA;
    outHasAlpha = (comp == 4);
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    stbi_image_free(data);
    return tex;
}

bool LoadOBJWithMaterials(const std::string& path, MeshGL_Model& outModel, const std::string& workingFolderFallback) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string folder = GetFolder(path);
    if (folder.empty() && !workingFolderFallback.empty()) folder = workingFolderFallback;

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str(), folder.c_str(), true);
    if (!warn.empty()) std::cout << "tinyobj warn: " << warn << "\n";
    if (!err.empty()) std::cerr << "tinyobj err: " << err << "\n";
    if (!ok) return false;

    // We'll expand vertices (no dedupe). Build global vertex array & indices,
    // and create submesh ranges using per-face material ids.
    struct V { float px, py, pz; float nx, ny, nz; float u, v; };
    std::vector<V> vertices;
    std::vector<unsigned int> indices;
    std::vector<SubMeshRange> submeshes;

    // Map to track per-material submesh accumulation
    int globalIndexOffset = 0;

    // tinyobj stores per-face material id in shape.mesh.material_ids. We'll iterate faces.
    for (const auto& shape : shapes) {
        size_t faceOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            int matId = -1;
            if (f < shape.mesh.material_ids.size()) matId = shape.mesh.material_ids[f];

            // If last submesh has same materialId, extend, else create new submesh
            if (submeshes.empty() || submeshes.back().materialId != matId) {
                SubMeshRange r;
                r.materialId = matId;
                r.indexOffset = indices.size();
                r.indexCount = 0;
                submeshes.push_back(r);
            }

            for (int v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[faceOffset + v];
                V vert{};
                if (idx.vertex_index >= 0) {
                    vert.px = attrib.vertices[3 * idx.vertex_index + 0];
                    vert.py = attrib.vertices[3 * idx.vertex_index + 1];
                    vert.pz = attrib.vertices[3 * idx.vertex_index + 2];
                }
                if (idx.normal_index >= 0) {
                    vert.nx = attrib.normals[3 * idx.normal_index + 0];
                    vert.ny = attrib.normals[3 * idx.normal_index + 1];
                    vert.nz = attrib.normals[3 * idx.normal_index + 2];
                }
                else {
                    vert.nx = vert.ny = vert.nz = 0.0f;
                }
                if (idx.texcoord_index >= 0) {
                    vert.u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vert.v = attrib.texcoords[2 * idx.texcoord_index + 1];
                }
                else {
                    vert.u = vert.v = 0.0f;
                }

                unsigned int newIndex = static_cast<unsigned int>(vertices.size());
                vertices.push_back(vert);
                indices.push_back(newIndex);
                submeshes.back().indexCount++;
            }
            faceOffset += fv;
        }
    }

    // Create GL buffers
    glGenVertexArrays(1, &outModel.VAO);
    glGenBuffers(1, &outModel.VBO);
    glGenBuffers(1, &outModel.EBO);

    glBindVertexArray(outModel.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, outModel.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(V), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outModel.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Setup attribs: pos(0), normal(1), uv(2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)offsetof(V, px));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)offsetof(V, nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(V), (void*)offsetof(V, u));

    glBindVertexArray(0);

    outModel.indexCount = indices.size();
    outModel.submeshes = std::move(submeshes);

    // Build material list & load textures
    outModel.materials.resize(materials.size());
    for (size_t i = 0; i < materials.size(); ++i) {
        outModel.materials[i].name = materials[i].name;
        std::string diff = materials[i].diffuse_texname;
        if (!diff.empty()) {
            std::string full = folder + diff;
            bool hasAlpha = false;
            outModel.materials[i].diffuseTex = LoadTexture2D(full, hasAlpha);
            outModel.materials[i].usesAlpha = hasAlpha;
            if (outModel.materials[i].diffuseTex == 0) {
                std::cerr << "Warning: failed to load texture " << full << "\n";
            }
        }
    }

    // If no materials present, create one default material (so materialId indexes are safe)
    if (outModel.materials.empty()) {
        outModel.materials.push_back(MaterialGL{ "default", 0, false });
    }

    return true;
}
