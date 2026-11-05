#include "ParticleSystem.h"

Particle::Particle() : next_(nullptr) {}

void Particle::update(float dt)
{
	life_ -= dt;   // life_ en secondes, indépendant du framerate

	/*attributes_.vel.x = speed * sin(theta) * cos(phi);
	attributes_.vel.y = speed * sin(phi);
	attributes_.vel.z = speed * cos(theta) * cos(phi);*/

	attributes_.pos += attributes_.vel * dt;
}

void Particle::drawParticle(const Material& mat)
{

}

void Particle::init(const Maths::Vec3S& pos, const Maths::Vec3S& vel, float life)
{
	attributes_.pos = pos;
	attributes_.vel = vel;
	life_ = life;
}

void ParticlePool::generate(const Maths::Vec3S& pos, const Maths::Vec3S& vel, float life)
{
	if (firstAvailable_ == nullptr) return;   // pool plein : on ignore le spawn

	Particle* newParticle = firstAvailable_;
	firstAvailable_ = newParticle->getNext();   // lire next_ AVANT init (même mémoire)

	newParticle->init(pos, vel, life);          // plus d'emitter pour l'instant : paramètres directs
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ParticlePool::ParticlePool(/*Emitter* emitter*/)
{
	// poolSize_ = emitter->getPoolSize();
	poolSize_ = 1024;   // provisoire
	particleArray_ = new Particle[poolSize_];

	firstAvailable_ = &particleArray_[0];

	for (int i = 0; i < poolSize_ - 1; i++){
		particleArray_[i].setNext(&particleArray_[i + 1]);
	}
	particleArray_[poolSize_ - 1].setNext(nullptr);
}

bool ParticlePool::update(float dt, const Material& mat)
{
	for (int i = 0; i < poolSize_; i++) {

		if (particleArray_[i].isAlive()) {

			particleArray_[i].update(dt);
			particleArray_[i].drawParticle(mat);
			return true;
		}
		else {
			particleArray_[i].setNext(firstAvailable_);
			firstAvailable_ = &particleArray_[i];
		}
	}
	return false;
}