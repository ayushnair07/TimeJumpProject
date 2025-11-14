// TextRenderer.cpp
#include "TextRenderer.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

// Vertex shader (embedded)
static const char* textVS = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos; // pixel coords
layout(location = 1) in vec2 aUV;
uniform mat4 uOrtho;
out vec2 vUV;
void main() {
    gl_Position = uOrtho * vec4(aPos.xy, 0.0, 1.0);
    vUV = aUV;
}
)glsl";

// Fragment shader (embedded)
static const char* textFS = R"glsl(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uFont;
uniform vec3 uTextColor;
void main() {
    vec4 s = texture(uFont, vUV);
    float alpha = s.a;
    FragColor = vec4(uTextColor, alpha);
    if (FragColor.a < 0.01) discard;
}
)glsl";

TextRenderer::TextRenderer() {}
TextRenderer::~TextRenderer() {
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (fontTex) glDeleteTextures(1, &fontTex);
    if (shaderID) glDeleteProgram(shaderID);
}

unsigned int TextRenderer::CompileShader(const char* vsSrc, const char* fsSrc) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    GLint ok; glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(vs, 1024, nullptr, log); std::cerr << "Text VS error: " << log << "\n"; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(fs, 1024, nullptr, log); std::cerr << "Text FS error: " << log << "\n"; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log); std::cerr << "Text link error: " << log << "\n"; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

bool TextRenderer::Init(int screenW, int screenH) {
    scrW = screenW; scrH = screenH;
    shaderID = CompileShader(textVS, textFS);
    if (!shaderID) return false;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // allocate a moderate dynamic buffer (enough for many glyphs)
    glBufferData(GL_ARRAY_BUFFER, 128 * 1024, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    inited = true;
    return true;
}

bool TextRenderer::LoadFont(const std::string& atlasPath, int c, int r) {
    cols = c; rows = r;
    int w, h, n;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(atlasPath.c_str(), &w, &h, &n, 4);
    if (!data) {
        std::cerr << "TextRenderer: failed to load atlas: " << atlasPath << "\n";
        return false;
    }
    glGenTextures(1, &fontTex);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return true;
}

void TextRenderer::Resize(int screenW, int screenH) {
    scrW = screenW; scrH = screenH;
}

bool TextRenderer::IsReady() const { return inited && fontTex != 0 && shaderID != 0; }

void TextRenderer::RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    if (!IsReady()) return;

    // build vertices for the whole string
    std::vector<float> verts;
    verts.reserve(text.size() * 6 * 4);

    float glyphW = 1.0f / (float)cols;
    float glyphH = 1.0f / (float)rows;
    float px = x;
    float py = y;

    // base glyph pixel size (tweak depending on atlas)
    float baseW = 8.0f * scale;
    float baseH = 16.0f * scale;

    for (char ch : text) {
        if (ch == '\n') { px = x; py += baseH; continue; }
        if ((unsigned char)ch < 0) continue;
        int ci = (unsigned char)ch;
        float gx = (ci % cols) * glyphW;
        float gy = (ci / cols) * glyphH;

        // tri1
        verts.insert(verts.end(), { px,       py,        gx,         gy });
        verts.insert(verts.end(), { px + baseW, py,        gx + glyphW, gy });
        verts.insert(verts.end(), { px + baseW, py + baseH, gx + glyphW, gy + glyphH });
        // tri2
        verts.insert(verts.end(), { px,       py,        gx,         gy });
        verts.insert(verts.end(), { px + baseW, py + baseH, gx + glyphW, gy + glyphH });
        verts.insert(verts.end(), { px,       py + baseH, gx,         gy + glyphH });

        px += baseW;
    }

    // upload
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(float), verts.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(shaderID);
    glm::mat4 ortho = glm::ortho(0.0f, (float)scrW, (float)scrH, 0.0f);
    int loc = glGetUniformLocation(shaderID, "uOrtho");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &ortho[0][0]);

    int locc = glGetUniformLocation(shaderID, "uTextColor");
    glUniform3f(locc, color.r, color.g, color.b);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    int locf = glGetUniformLocation(shaderID, "uFont");
    glUniform1i(locf, 0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 4));
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
