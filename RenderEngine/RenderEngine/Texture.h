#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Texture
{
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;   // 0xAABBGGRR en little-endian (octets R,G,B,A), même layout que le framebuffer

    bool load(const std::string& path);
    bool empty() const { return pixels.empty(); }

    // renvoie un pixel au même format que le framebuffer, utilisable directement par glDrawPixels
    uint32_t sample(float u, float v) const;
    uint32_t sampleBilinear(float u, float v) const;
};