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
	float Life = 0.f;  

	union {
		ParticleAttributes attributes;
		Particle* Next;
	};

public:
	Particle();
	~Particle() = default;

	void update(float dt);
	void DrawParticle(const Material& mat);
	void Init(const Maths::Vec3S& pos, const Maths::Vec3S& vel);

	bool isAlive() { return Life > 0; };
	Particle* GetNext() { return Next; };
	void SetNext(Particle* next) { Next = next; };
};


class ParticlePool {
	int poolSize = 0;
	Particle* FirstAvailable = nullptr;
	Particle* ParticleArray = nullptr;

public:
	ParticlePool(/*Emitter* emitter*/); //// A CHANGER
	~ParticlePool() = default;

	void Generate();
	bool update(float dt, const Material& mat);
};



class ParticleSystem{
	ParticlePool pool;
	Material mat;

	void Render(const Material& mat);
};

