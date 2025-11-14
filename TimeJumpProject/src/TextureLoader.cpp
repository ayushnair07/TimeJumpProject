#include "stb_image.h"
#include <iostream>
#include <string>
#include <glad/glad.h> 

static GLuint LoadTexture2D_File(const std::string& path, bool flipY = true) {
    stbi_set_flip_vertically_on_load(flipY);
    int w, h, n;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, 4); // force RGBA
    if (!data) {
        std::cerr << "LoadTexture2D_File: failed to load: " << path << "\n";
        return 0;
    }
    GLuint tid = 0;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    stbi_image_free(data);
    return tid;
}
