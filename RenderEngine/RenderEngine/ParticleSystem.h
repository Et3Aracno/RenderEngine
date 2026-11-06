#pragma once
#include <iostream>
#include <memory>
#include <random>
#include <assert.h>
#include "External Lib/MathLibrary/MathLib.h"
#include "Material.h"
#include "Emitter.h"


// Donnees qui varient d'une particule a l'autre (tirees dans l'Emitter a la creation).
// Types triviaux uniquement : obligatoire pour la partager avec next_ dans l'union.
struct ParticleAttributes {
	Maths::Vec3S pos;
	Maths::Vec3S dir;
	float speedStart, speedEnd;
	float sizeStart, sizeEnd;
	float rotation, rotSpeed;
	float maxLife; // secondes
};

struct ParticleSprite {
	Maths::Vec3S pos;
	float size;
	float rotation;
	Color color;
};

class Particle {
	float life_ = 0.f;  // elle a dead ça

	union {
		ParticleAttributes attributes_;
		Particle* next_;
	};

public:
	Particle();
	~Particle() = default;

	void init(const Maths::Vec3S& pos, const ParticleAttributes& attributes);
	void update(float dt);

	// t = 1 = mort
	float getLifeRatio() const { return 1.f - life_ / attributes_.maxLife; }
	float getSize() const;
	const ParticleAttributes& getAttributes() const { return attributes_; }

	bool isAlive() const { return life_ > 0.f; }
	Particle* getNext() const { return next_; }
	void setNext(Particle* next) { next_ = next; }
};


class ParticlePool {
	int poolSize_ = 0;
	Particle* firstAvailable_ = nullptr;
	std::unique_ptr<Particle[]> particleArray_;

public:
	ParticlePool(const Emitter& emitter);
	~ParticlePool() = default;

	ParticlePool(ParticlePool&& other) noexcept : poolSize_(std::exchange(other.poolSize_, 0)),
		firstAvailable_(std::exchange(other.firstAvailable_, nullptr)),
		particleArray_(std::move(other.particleArray_)){}

	ParticlePool& operator=(ParticlePool&& other) noexcept
	{
		if (this != &other) {
			poolSize_ = std::exchange(other.poolSize_, 0);
			firstAvailable_ = std::exchange(other.firstAvailable_, nullptr);
			particleArray_ = std::move(other.particleArray_);
		}
		return *this;
	}

	void generate(const Maths::Vec3S& pos, const Emitter& emitter, std::mt19937& rng);

	// vivantes ? true = oui
	bool update(float dt);

	const Particle* begin() const { return particleArray_.get(); }
	const Particle* end() const { return particleArray_.get() + poolSize_; }
};


class ParticleSystem {
	Emitter emitter_; 
	ParticlePool pool_;
	Material mat_;           
	std::mt19937 rng_{ std::random_device{}() };

	Maths::Vec3S position_;
	float elapsed_ = 0.f;  // compare lifetime_
	float emitAccumulator_ = 0.f;
	bool hasAlive_ = false;

	static constexpr float tickRate_ = 60.f; // seconds

public:
	ParticleSystem(const Emitter& emitter, const Material& mat, const Maths::Vec3S& position);

	void update(float dt); 
	void render(std::vector<ParticleSprite>& out) const; // pre operation of creating a triangle

	void setPosition(const Maths::Vec3S& position) { position_ = position; }
};