#pragma once
#include "Color.h"

class Emitter {
	float speed_;
	float startSize_, endSize_;

	int poolSize_;
	int lifetime_;
	int emitNumber_;
	int emissionRate_;      // variance
	int maxParticleLife_;
	int anglePhiMin_, anglePhiMax_;
	int angleThetaMin_, angleThetaMax_;

	Color startColor_, endColor_;

public:
	Emitter(float speed, float startSize, float endSize, int poolSize, int lifetime, int emitNumber,
		int emissionRate, int maxParticleLife, int anglePhiMin, int anglePhiMax, int angleThetaMin,
		int angleThetaMax, Color startColor, Color endColor);
};