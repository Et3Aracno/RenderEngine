#include "Texture.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION      // seulement dans CE fichier
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4244 4456 4457 4459 4505 4267 4996)
#endif
#include "assets/stb_image.h"
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

bool Texture::load(const std::string& path)
{
    pixels.clear();
    width = height = 0;

    int w = 0, h = 0, channels = 0;
    // On force 4 canaux : stb remplit alpha = 255 si l'image n'en a pas
    uint8_t* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << path
            << " (" << stbi_failure_reason() << ")" << std::endl;
        return false;
    }
    if (w <= 0 || h <= 0)
    {
        stbi_image_free(data);
        return false;
    }

    width = w;
    height = h;
    pixels.resize((size_t)w * (size_t)h);

    
    for (size_t i = 0; i < pixels.size(); i++)
    {
        uint32_t r = data[i * 4 + 0];
        uint32_t g = data[i * 4 + 1];
        uint32_t b = data[i * 4 + 2];
        uint32_t a = data[i * 4 + 3];
        pixels[i] = (a << 24) | (b << 16) | (g << 8) | r;
    }

    stbi_image_free(data);
    return true;
}

uint32_t Texture::sample(float u, float v) const
{
    if (pixels.empty()) return 0xFFFF00FF;   // magenta = texture absente

    // Wrap : garde la partie fractionnaire
    u -= std::floor(u);
    v -= std::floor(v);

    // Plus proche voisin, avec flip de V (OBJ en bas, image en haut)
    int x = (int)(u * (float)width);
    int y = (int)((1.0f - v) * (float)height);

    // u ou v peuvent valoir exactement 1.0 après l'arrondi flottant
    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);

    return pixels[(size_t)y * width + x];
}