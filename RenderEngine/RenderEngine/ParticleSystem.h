#pragma once
#include <iostream>
#include <assert.h>
#include "External Lib/MathLibrary/MathLib.h"
#include "Material.h"


struct ParticleAttributes {
	Maths::Vec3S pos;
	Maths::Vec3S vel;
};

class Particle {
	float life_ = 0.f;

	union {
		ParticleAttributes attributes_;
		Particle* next_;
	};

public:
	Particle();
	~Particle() = default;

	void update(float dt);
	void drawParticle(const Material& mat);
	void init(const Maths::Vec3S& pos, const Maths::Vec3S& vel, float life);

	bool isAlive() const { return life_ > 0.f; }
	Particle* getNext() const { return next_; }
	void setNext(Particle* next) { next_ = next; }
};


class ParticlePool {
	int poolSize_ = 0;
	Particle* firstAvailable_ = nullptr;
	Particle* particleArray_ = nullptr;

public:
	ParticlePool(/*Emitter* emitter*/); //// A CHANGER
	~ParticlePool() = default;

	void generate(const Maths::Vec3S& pos, const Maths::Vec3S& vel, float life);
	bool update(float dt, const Material& mat);
};


class ParticleSystem {
	ParticlePool pool_;
	Material mat_;

	void render(const Material& mat);
};