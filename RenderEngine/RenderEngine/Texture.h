#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Texture
{
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;   // ARGB8888, même layout que le framebuffer

    bool load(const std::string& path);
    bool empty() const { return pixels.empty(); }

	// pixel ARGB pour que open gl puisse l'utiliser directement
    uint32_t sample(float u, float v) const;
    uint32_t sampleBilinear(float u, float v) const;
};