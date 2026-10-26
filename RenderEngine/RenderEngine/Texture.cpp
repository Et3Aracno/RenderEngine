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
    int x = (u * width);
    int y = ((1.0f - v) * height);

    // u ou v peuvent valoir exactement 1.0 après l'arrondi flottant
    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);

    return pixels[(size_t)y * width + x];
}


uint32_t Texture::sampleBilinear(float u, float v) const
{
    if (pixels.empty()) return 0xFFFF00FF;   // magenta = texture absente

    // Wrap + flip de V, comme dans sample()
    u -= std::floor(u);
    v -= std::floor(v);

    // Position en coordonnées texel. Le -0.5 décale vers les CENTRES des texels :
    // sans ça, l'image serait décalée d'un demi-texel.
    float fx = u * width - 0.5f;
    float fy = (1.0f - v) * (float)height - 0.5f;

    int x0 = std::floor(fx);
    int y0 = std::floor(fy);
    float tx = fx - x0;   // partie fractionnaire = poids horizontal
    float ty = fy - y0;   // partie fractionnaire = poids vertical

    // Les 4 texels voisins, avec wrap aux bords
    auto wrap = [](int i, int n) { return ((i % n) + n) % n; };
    int xa = wrap(x0, width), xb = wrap(x0 + 1, width);
    int ya = wrap(y0, height), yb = wrap(y0 + 1, height);

    uint32_t c00 = pixels[(size_t)ya * width + xa];
    uint32_t c10 = pixels[(size_t)ya * width + xb];
    uint32_t c01 = pixels[(size_t)yb * width + xa];
    uint32_t c11 = pixels[(size_t)yb * width + xb];

    // Les 4 poids (leur somme vaut 1)
    float w00 = (1.0f - tx) * (1.0f - ty);
    float w10 = tx * (1.0f - ty);
    float w01 = (1.0f - tx) * ty;
    float w11 = tx * ty;

    // Mélange chaque canal (octet) séparément
    uint32_t result = 0;
    for (int shift = 0; shift <= 24; shift += 8)
    {
            float ch = ((c00 >> shift) & 0xFF) * w00
                     + ((c10 >> shift) & 0xFF) * w10
                     + ((c01 >> shift) & 0xFF) * w01
                     + ((c11 >> shift) & 0xFF) * w11;
        result |= ((uint32_t)(ch + 0.5f) & 0xFF) << shift;
    }
    return result;
}