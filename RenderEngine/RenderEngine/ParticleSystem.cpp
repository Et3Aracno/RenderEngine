#include "ParticleSystem.h"

Particle::Particle() : Next(nullptr) {}

void Particle::update(float dt){
	Life--;

	attributes.vel.x = speed * sin(theta) * cos(phi);
	attributes.vel.y = speed * sin(phi);
	attributes.vel.z = speed * cos(theta) * cos(phi);

	attributes.pos += attributes.vel * dt;
}

void Particle::DrawParticle(const Material& mat){
	    
}

void Particle::Init(const Maths::Vec3S& pos, const Maths::Vec3S& vel){
	attributes.pos = pos;
	attributes.vel = vel;
}

void ParticlePool::Generate(){
	assert(FirstAvailable != nullptr);

	Particle* newParticle = FirstAvailable;
	FirstAvailable = FirstAvailable->GetNext();

	newParticle->Init(position, velocity); /////////// More to see with the emitter & if parameters necessay
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ParticlePool::ParticlePool(/*Emitter* emitter*/) {
	// poolSize = Emitter->GetPoolSize();
	ParticleArray = new Particle[poolSize];

	FirstAvailable = &ParticleArray[0];

	for (int i = 0; i < poolSize - 1; i++) {
		ParticleArray[i].SetNext(&ParticleArray[i + 1]);

	}
}

bool ParticlePool::update(float dt, const Material& mat)
{
	for (int i = 0; i < poolSize; i++) {
		if (ParticleArray[i].isAlive()){
			ParticleArray[i].update(dt);
			ParticleArray[i].DrawParticle(mat);
			return true;
		}
		else {
			ParticleArray[i].SetNext(FirstAvailable);
			FirstAvailable = &ParticleArray[i];
		}
	}
	return false;
}
