#include "Material.h"
#include "Color.h"
#include <cmath>

// shift = position du canal dans le texel : 0, 8 ou 16.
uint32_t Material::shadeChannel(uint32_t texel, int shift, //
    float diffuseK, float specK, float metallic)
{
	float c = ((texel >> shift) & 0xFF) / 255.0f;// Normalisation du canal de couleur

    float specColor = 1.0f + (c - 1.0f) * metallic; // Mélange entre la couleur du matériau et la couleur spéculaire (blanc) en fonction de la valeur de metallic
    float val = c * diffuseK * (1.0f - metallic) + specColor * specK; // Calcul de la valeur finale du canal en combinant la couleur diffuse et spéculaire, en tenant compte de la valeur de metallic

    if (val > 1.0f) val = 1.0f;
	return static_cast<uint32_t>(val * 255.0f);
}

uint32_t Material::rdPass(uint32_t src, uint32_t dst) const //rending pass : src = couleur du matériau, dst = couleur du framebuffer
{
    Color cSrc(src);
    Color cDst(dst);
	const float alpha = cSrc.a() / 255.0f;
    const float invAlpha = 1.0f - alpha;

    uint32_t r = static_cast<uint32_t>(alpha * cSrc.r() + invAlpha * cDst.r() + 0.5f);
    uint32_t g = static_cast<uint32_t>(alpha * cSrc.g() + invAlpha * cDst.g() + 0.5f);
    uint32_t b = static_cast<uint32_t>(alpha * cSrc.b() + invAlpha * cDst.b() + 0.5f);

	return 0xFF000000u | (b << 16) | (g << 8) | r;
}

uint32_t Material::shade(float u, float v, float nDotL, float nDotH) const
{
    uint32_t texel = 0xFFFFFFFF;   // base color = white
    if (albedoMap_) {
        texel = isTextureFiltering_ ? albedoMap_->sampleBilinear(u, v) : albedoMap_->sample(u, v);
    }

    float diffuseK = ambient_ + (1.0f - ambient_) * nDotL;

    float exponent = std::exp2(1.0f + 9.0f * smoothness_);
    float specK = std::pow(nDotH, exponent) * specStrength_;

    const float textureAlpha = ((texel >> 24) & 0xFF) / 255.0f;
    const float alpha = textureAlpha * getOpacity();

    uint32_t r = shadeChannel(texel, 0, diffuseK, specK, metallic_);
    uint32_t g = shadeChannel(texel, 8, diffuseK, specK, metallic_);
    uint32_t b = shadeChannel(texel, 16, diffuseK, specK, metallic_);
    uint32_t a = static_cast<uint32_t>(alpha * 255.0f + 0.5f);

    return (a << 24) | (b << 16) | (g << 8) | r;
}