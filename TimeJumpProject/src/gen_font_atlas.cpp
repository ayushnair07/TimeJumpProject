// gen_font_atlas.cpp
// Build: compile with your normal compiler (example below).
// Produces resources/fonts/atlas.png (256x256, 16x16 glyphs).
//
// Requires stb_truetype.h and stb_image_write.h in same folder.
// You must provide a path to a TTF font on your system (see FONT_PATH).
//
// Example compile (Windows Visual Studio): add file to project and build.
// Example compile (gcc/clang):
//   g++ gen_font_atlas.cpp -O2 -std=c++17 -o gen_font_atlas
//   ./gen_font_atlas

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

int main(int argc, char** argv) {
    // Font path - change this to a font that exists on your system if needed.
    // Windows default (Arial) example:
    std::string FONT_PATH = "C:\\Windows\\Fonts\\arial.ttf";
    // Mac example: "/Library/Fonts/Arial.ttf"
    // Linux example: "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    if (argc >= 2) FONT_PATH = argv[1];

    // Output atlas path
    std::filesystem::create_directories("resources/fonts");
    const char* outPath = "resources/fonts/atlas.png";

    // Atlas settings
    const int atlasW = 256;
    const int atlasH = 256;
    const int cols = 16;
    const int rows = 16;
    const int cellW = atlasW / cols;   // 16
    const int cellH = atlasH / rows;   // 16

    // Font bake settings
    const float fontPixelHeight = 14.0f; // glyph pixel height; tweak if glyphs too small/large

    // Read font file
    std::ifstream ifs(FONT_PATH, std::ios::binary);
    if (!ifs) {
        std::cerr << "Failed to open font at: " << FONT_PATH << "\n";
        std::cerr << "Usage: gen_font_atlas [path-to-ttf]\n";
        return 1;
    }
    std::vector<unsigned char> fontBuffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    // Prepare bitmap for atlas (RGBA)
    std::vector<unsigned char> image(atlasW * atlasH * 4);
    for (size_t i = 0; i < image.size(); ++i) image[i] = 0; // transparent

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, fontBuffer.data(), 0)) {
        std::cerr << "stbtt_InitFont failed\n";
        return 2;
    }

    // Bake each glyph into its cell
    for (int ch = 0; ch < cols * rows; ++ch) {
        int cx = ch % cols;
        int cy = ch / cols;

        unsigned char glyphBitmap[cellW * cellH];
        // Clear glyph bitmap
        for (int i = 0; i < cellW * cellH; ++i) glyphBitmap[i] = 0;

        // Bake single glyph into a small bitmap using stb_truetype's rasterization
        unsigned char* bmp = nullptr;
        int w = 0, h = 0, xoff = 0, yoff = 0;
        // use stbtt_GetCodepointBitmap to get alpha map directly
        bmp = stbtt_GetCodepointBitmap(&info, 0.0f, fontPixelHeight, ch, &w, &h, &xoff, &yoff);
        if (bmp) {
            // center glyph inside cell
            int startX = cx * cellW + (cellW - w) / 2;
            int startY = cy * cellH + (cellH - h) / 2;
            if (startX < 0) startX = 0;
            if (startY < 0) startY = 0;
            for (int yy = 0; yy < h; ++yy) {
                for (int xx = 0; xx < w; ++xx) {
                    int sx = startX + xx;
                    int sy = startY + yy;
                    if (sx < 0 || sx >= atlasW || sy < 0 || sy >= atlasH) continue;
                    unsigned char a = bmp[yy * w + xx];
                    size_t idx = (sy * atlasW + sx) * 4;
                    // white color with alpha
                    image[idx + 0] = 255;
                    image[idx + 1] = 255;
                    image[idx + 2] = 255;
                    image[idx + 3] = a;
                }
            }
            stbtt_FreeBitmap(bmp, nullptr);
        }
        else {
            // if no bitmap (space or non-renderable), leave cell transparent
        }
    }

    // Write PNG
    if (!stbi_write_png(outPath, atlasW, atlasH, 4, image.data(), atlasW * 4)) {
        std::cerr << "Failed to write atlas PNG\n";
        return 3;
    }

    std::cout << "Atlas written to: " << outPath << "\n";
    std::cout << "Font used: " << FONT_PATH << "\n";
    return 0;
}
