#pragma once
#include <string>
#include <vector>
#include <random>
#include "Color.h"

// enum class BlendMode { add, alpha };

struct RandomValue {
	float value = 0.f;
	float randMin = 1.f;
	float randMax = 1.f;

	float randomized(std::mt19937& rng) const {
		std::uniform_real_distribution<float> dist(randMin, randMax);
		return value * dist(rng);
	}
};

struct FloatRange { float min = 0.f, max = 0.f; };

class Emitter {
private:
	std::string type_;                  

	FloatRange thetaRange_;
	FloatRange angleRange_;             

	RandomValue rotSpeed_;
	RandomValue startSpeed_, endSpeed_;
	RandomValue startSize_, endSize_;
	RandomValue emitVariance_;
	RandomValue maxParticleLife_;

	int emitNumber_ = 1;
	float lifetime_ = -1.f;             // infini

	Color startColor_, endColor_;
	//BlendMode blendMode_ = BlendMode::alpha;

public:
	Emitter() = default;

	// Charge tous les <emitter> d'un fichier XML (necessite une lib XML, ex. tinyxml2)
	static std::vector<Emitter> loadFromFile(const std::string& path);

	// Tirages par particule
	int rollEmitCount(std::mt19937& rng) const;
	float rollSpeedStart(std::mt19937& rng) const { return startSpeed_.randomized(rng); }
	float rollSpeedEnd(std::mt19937& rng) const { return endSpeed_.randomized(rng); }
	float rollSizeStart(std::mt19937& rng) const { return startSize_.randomized(rng); }
	float rollSizeEnd(std::mt19937& rng) const { return endSize_.randomized(rng); }
	float rollRotSpeed(std::mt19937& rng) const { return rotSpeed_.randomized(rng); }
	float rollLife(std::mt19937& rng) const { return maxParticleLife_.randomized(rng); }
	float rollTheta(std::mt19937& rng) const;
	float rollAngle(std::mt19937& rng) const;

	// Taille de pool necessaire (pire cas)
	int getPoolSize() const;

	const std::string& getType() const { return type_; }
	const FloatRange& getThetaRange() const { return thetaRange_; }
	const FloatRange& getAngleRange() const { return angleRange_; }
	int getEmitNumber() const { return emitNumber_; }
	float getLifetime() const { return lifetime_; }
	bool isInfinite() const { return lifetime_ < 0.f; }
	Color getStartColor() const { return startColor_; }
	Color getEndColor() const { return endColor_; }

	//BlendMode getBlendMode() const { return blendMode_; }
	//Color getColorAt(float t) const;
};