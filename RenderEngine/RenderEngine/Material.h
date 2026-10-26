#pragma once
#include <string>
#include "External Lib/MathLibrary/MathLib.h"
#include "Texture.h"
#include <memory>
#include <algorithm>

class Material {
private:

    std::shared_ptr<Texture> albedoMap = nullptr;
	float ambient = 0.18f; // puissance de la lumière ambiante sur la surface
    float metallic = 0.0f;
    float smoothness = 0.45f; 
	float specStrength = 0.55f;  //puissance du reflet de la lumière sur la surface
	bool  texturefiltering = true; // true = bilinear, false = voisin le plus proche
	float opacity = 1.0f; 


    static uint32_t shade_channel(uint32_t texel, int shift, float diffuse_k, float spec_k, float metallic);


public:


	uint32_t rdPass(uint32_t src, uint32_t dst) const ;
    uint32_t Shade(float u, float v, float ndotl, float ndoth) const;
	
	std::shared_ptr<Texture> getAlbedoMap() const { return albedoMap;}
	float getAmbient() const { return ambient; }
	float getMetallic() const { return metallic; }
	float getSmoothness() const { return smoothness; }
	float getSpecStrength() const { return specStrength; }
	bool isTextureFiltering() const { return texturefiltering; }
	float getOpacity() const { return opacity; }


	//si on met nullptr il n'y a plus de texture et la surface est blanche
	void setAlbedoMap(std::shared_ptr<Texture> texture) { albedoMap = texture; }

	// devient imprévisible au dessus de 1 la face dans l'ombre devient plus claire que la face éclairée, 
	// en dessous de 0 la face dans l'ombre reçoit un diffuse négatif et la couleur
	void setAmbient(float value) { ambient = std::clamp(value, 0.0f, 1.0f); }

	// si on sort de l'interval le diffuse devient négatif ou le reflet prend une couleur glitch
	void setMetallic(float value) { metallic = std::clamp(value, 0.0f, 1.0f); }

	// un smoothness négatif donne un exposant sous 2 (reflet énorme et plat), 
	// et un très grand donne un exposant gigantesque
	void setSmoothness(float value) { smoothness = std::clamp(value, 0.0f, 1.0f); }

	// en dessous de 0 le reflet se soustrait et assombrit la zone,
	// au dessus de 1 le reflet sature simplement en blanc
	void setSpecStrength(float value) { specStrength = std::clamp(value, 0.0f, 1.0f); }

	// true = bilinear (lisse de près), false = voisin le plus proche (blocs visibles de près)
	void setTextureFiltering(bool value) { texturefiltering = value; }

	void setOpacity(float value) { opacity = std::clamp(value, 0.0f, 1.0f); }


};



