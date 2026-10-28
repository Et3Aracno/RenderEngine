#include "Material.h"
#include "Color.h"
#include <cmath>


// shift = position du canal dans le texel : 0, 8 ou 16.
inline uint32_t Material::shade_channel(uint32_t texel, int shift,
                                        float diffuse_k, float spec_k, float metallic)
{
    float c = ((texel >> shift) & 0xFF) / 255.0f;

    float spec_color = 1.0f + (c - 1.0f) * metallic;
    float val = c * diffuse_k * (1.0f - metallic) + spec_color * spec_k;

    if (val > 1.0f) val = 1.0f;
    return (uint32_t)(val * 255.0f);
}

uint32_t Material::rdPass(uint32_t src, uint32_t dst) const
{
	Color csrc(src);
	Color cdst(dst);
    const float alpha = getOpacity();
    const float invAlpha = 1.0f - alpha;


    uint8_t r = alpha * csrc.r() + invAlpha * cdst.r();
    uint8_t rt = alpha * src + invAlpha * dst;
    uint8_t g = alpha * csrc.g() + invAlpha * cdst.g();
    uint8_t gt = alpha * (src << 16) + invAlpha * (dst << 16);
    uint8_t b = alpha * csrc.b() + invAlpha * cdst.b();
    uint8_t bt = alpha * (src << 24) + invAlpha * (dst << 24);

	return 0xFF000000u | (b << 16) | (g << 8) | r;
}

uint32_t Material::Shade(float u, float v, float ndotl, float ndoth) const {

	uint32_t texel = 0xFFFFFFFF; // base color = white
    if (albedoMap) {
        texel = texturefiltering ? albedoMap->sampleBilinear(u, v) : albedoMap->sample(u, v);
    }

    float diffuse_k = ambient + (1.0f - ambient) * ndotl;

    float exposant = std::exp2(1.0f + 9.0f * smoothness);
    float spec_k = std::pow(ndoth, exposant) * specStrength;

    const float textureAlpha = ((texel >> 24) & 0xFF) / 255.0f;
    const float alpha = textureAlpha * getOpacity();

    uint32_t r = shade_channel(texel, 0, diffuse_k, spec_k, metallic);
    uint32_t g = shade_channel(texel, 8, diffuse_k, spec_k, metallic);
    uint32_t b = shade_channel(texel, 16, diffuse_k, spec_k, metallic);
    uint32_t a = alpha * 255.0f + 0.5f;

    return (a<<24) | (b << 16) | (g << 8) | r;
}