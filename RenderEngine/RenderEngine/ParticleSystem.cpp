#include "ParticleSystem.h"

namespace {
	constexpr float pi = 3.14159265f;
	constexpr float framesPerSecond = 60.f;

	float toRadians(float degrees) { return degrees * pi / 180.f; }
	float lerp(float a, float b, float t) { return a + t * (b - a); }
}

Particle::Particle() : next_(nullptr) {}

void Particle::update(float dt)
{
	life_ -= dt;
	if (life_ <= 0.f) {             // vient de mourir : on ne touche plus à attributes_
		life_ = 0.f;
		return;
	}

	const float t = getLifeRatio();
	const float speed = lerp(attributes_.speedStart, attributes_.speedEnd, t);

	attributes_.pos.x += attributes_.dir.x * speed * dt;
	attributes_.pos.y += attributes_.dir.y * speed * dt;
	attributes_.rotation += attributes_.rotSpeed * dt;
}

float Particle::getSize() const
{
	return lerp(attributes_.sizeStart, attributes_.sizeEnd, getLifeRatio());
}

void Particle::init(const Maths::Vec3S& pos, const ParticleAttributes& attributes)
{
	attributes_ = attributes;   
	attributes_.pos = pos;
	life_ = attributes_.maxLife;
}


/////////////////////////////////////////////////////////////////////////////


ParticlePool::ParticlePool(const Emitter& emitter)
{
	poolSize_ = emitter.getPoolSize();
	particleArray_ = std::make_unique<Particle[]>(poolSize_);

	firstAvailable_ = &particleArray_[0];

	for (int i = 0; i < poolSize_ - 1; i++) {
		particleArray_[i].setNext(&particleArray_[i + 1]);
	}
	particleArray_[poolSize_ - 1].setNext(nullptr);
}

void ParticlePool::generate(const Maths::Vec3S& pos, const Emitter& emitter, std::mt19937& rng)
{
	if (firstAvailable_ == nullptr) return;   // pool plein on dégage

	Particle* newParticle = firstAvailable_;
	firstAvailable_ = newParticle->getNext();

	const float theta = toRadians(emitter.rollTheta(rng));

	ParticleAttributes attributes;
	attributes.pos = pos;
	attributes.dir = Maths::Vec3S(std::cos(-theta), std::sin(-theta), 0.f);
	attributes.speedStart = emitter.rollSpeedStart(rng);
	attributes.speedEnd = emitter.rollSpeedEnd(rng);
	attributes.sizeStart = emitter.rollSizeStart(rng);
	attributes.sizeEnd = emitter.rollSizeEnd(rng);
	attributes.rotation = 0.f;
	attributes.rotSpeed = emitter.rollRotSpeed(rng);
	attributes.maxLife = std::max(0.001f, emitter.rollLife(rng) / framesPerSecond);

	newParticle->init(pos, attributes);
}

bool ParticlePool::update(float dt)
{
	for (int i = 0; i < poolSize_; i++) {
		if (!particleArray_[i].isAlive()) continue;

		if (particleArray_[i].isAlive()) {

			particleArray_[i].update(dt);
			return true;
		}
		else {
			particleArray_[i].setNext(firstAvailable_);
			firstAvailable_ = &particleArray_[i];
		}
	}
	return false;
}


////////////////////////////////////////////////////////////////////////////


ParticleSystem::ParticleSystem(const Emitter& emitter, const Material& mat, const Maths::Vec3S& position)
	: emitter_(emitter), pool_(emitter), mat_(mat), position_(position){}

void ParticleSystem::update(float dt)
{
	elapsed_ += dt;

	const bool canEmit = emitter_.isInfinite() || elapsed_ < emitter_.getLifetime();
	if (canEmit) {
		const int count = emitter_.rollEmitCount(rng_);
		for (int i = 0; i < count; i++)
			pool_.generate(position_, emitter_, rng_);
	}

	hasAlive_ = pool_.update(dt);
}

void ParticleSystem::render(std::vector<ParticleSprite>& out) const
{
	for (const Particle* p = pool_.begin(); p != pool_.end(); ++p) {
		if (!p->isAlive()) continue;            

		const float t = p->getLifeRatio();

		ParticleSprite sprite;
		sprite.pos = p->getAttributes().pos;
		sprite.size = p->getSize();
		sprite.rotation = p->getAttributes().rotation;
		sprite.color = emitter_.getStartColor();
		out.push_back(sprite);
	}
}
