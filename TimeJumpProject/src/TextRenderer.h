#pragma once
#include <string>
#include <glm/glm.hpp>

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Initialize with screen size in pixels
    bool Init(int screenW, int screenH);

    // Load a font atlas image (16x16 default ASCII grid)
    bool LoadFont(const std::string& atlasPath, int cols = 16, int rows = 16);

    // Render text at pixel coords (0,0) top-left. scale in pixels multiplier.
    void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color);

    // Update when window resized
    void Resize(int screenW, int screenH);

    // Helper
    bool IsReady() const;

private:
    unsigned int VAO = 0, VBO = 0;
    unsigned int fontTex = 0;
    unsigned int shaderID = 0;
    int scrW = 800, scrH = 600;
    int cols = 16, rows = 16;
    bool inited = false;

    // compile helper (internal)
    unsigned int CompileShader(const char* vsSrc, const char* fsSrc);
};
