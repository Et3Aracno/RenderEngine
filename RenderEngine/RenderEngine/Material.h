#pragma once
#include <string>
#include <memory>
#include <algorithm>
#include <stdint.h>
#include "External Lib/MathLibrary/MathLib.h"
#include "Texture.h"

class Material {
private:
	std::shared_ptr<Texture> albedoMap_ = nullptr;
	float ambient_ = 0.18f;   // puissance de la lumière ambiante sur la surface
	float metallic_ = 0.0f;
	float smoothness_ = 0.45f;
	float specStrength_ = 0.55f;   // puissance du reflet de la lumière sur la surface
	bool  isTextureFiltering_ = true;   // true = bilinear, false = voisin le plus proche
	float opacity_ = 1.0f;

	static uint32_t shadeChannel(uint32_t texel, int shift, float diffuseK, float specK, float metallic);

public:
	uint32_t rdPass(uint32_t src, uint32_t dst) const;
	uint32_t shade(float u, float v, float nDotL, float nDotH) const;

	std::shared_ptr<Texture> getAlbedoMap() const { return albedoMap_; }
	float getAmbient() const { return ambient_; }
	float getMetallic() const { return metallic_; }
	float getSmoothness() const { return smoothness_; }
	float getSpecStrength() const { return specStrength_; }
	bool  isTextureFiltering() const { return isTextureFiltering_; }
	float getOpacity() const { return opacity_; }

	// si on met nullptr il n'y a plus de texture et la surface est blanche
	void setAlbedoMap(std::shared_ptr<Texture> texture) { albedoMap_ = std::move(texture); }

	// devient imprévisible au dessus de 1 : la face dans l'ombre devient plus claire que la face éclairée,
	// en dessous de 0 la face dans l'ombre reçoit un diffuse négatif et la couleur
	void setAmbient(float value) { ambient_ = std::clamp(value, 0.0f, 1.0f); }

	// si on sort de l'intervalle le diffuse devient négatif ou le reflet prend une couleur glitch
	void setMetallic(float value) { metallic_ = std::clamp(value, 0.0f, 1.0f); }

	// un smoothness négatif donne un exposant sous 2 (reflet énorme et plat),
	// et un très grand donne un exposant gigantesque
	void setSmoothness(float value) { smoothness_ = std::clamp(value, 0.0f, 1.0f); }

	// en dessous de 0 le reflet se soustrait et assombrit la zone,
	// au dessus de 1 le reflet sature simplement en blanc
	void setSpecStrength(float value) { specStrength_ = std::clamp(value, 0.0f, 1.0f); }

	// true = bilinear (lisse de près), false = voisin le plus proche (blocs visibles de près)
	void setTextureFiltering(bool value) { isTextureFiltering_ = value; }

	void setOpacity(float value) { opacity_ = std::clamp(value, 0.0f, 1.0f); }
};