#pragma once
#include "Color.h"

class Emitter{

	float speed;
	float startSize, endSize;

	int poolSize;
	int lifetime;
	int emitNumber;
	int emisionRate; // variance
	int maxParticleLife;
	int anglePhimin, anglePhiMax;
	int angleThetamin, angleThetaMax;

	Color startColor, endColor;

public:
	Emitter(float speed, float startSize, float endSize, int poolSize, int lifetime, int emitNumber,
		int emisionRate, int maxParticleLife, int anglePhimin, int anglePhiMax, int angleThetamin,
		int angleThetaMax, Color startColor, Color endColor);


};