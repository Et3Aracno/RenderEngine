#include "Emitter.h"
#include "External Lib/Parser/tinyxml2.h"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace tinyxml2;

// si none default
static float readFloat(const XMLElement* elem, const char* name, float defaultVal)
{
	float val = defaultVal;
	if (elem) elem->QueryFloatAttribute(name, &val);
	return val;
}

static int readInt(const XMLElement* elem, const char* name, int defaultVal)
{
	int val = defaultVal;
	if (elem) elem->QueryIntAttribute(name, &val);
	return val;
}

static RandomValue readRandomized(const XMLElement* parent, const char* tag)
{
	RandomValue random;
	const XMLElement* elem = parent->FirstChildElement(tag);
	random.value = readFloat(elem, "value", 0.f);
	random.randMin = readFloat(elem, "randMin", 1.f);
	random.randMax = readFloat(elem, "randMax", 1.f);
	return random;
}

static FloatRange readRange(const XMLElement* parent, const char* tag)
{
	FloatRange range;
	const XMLElement* elem = parent->FirstChildElement(tag);
	range.min = readFloat(elem, "min", 0.f);
	range.max = readFloat(elem, "max", 0.f);
	return range;
}

static Color readColor(const XMLElement* parent, const char* tag)
{
	const XMLElement* elem = parent->FirstChildElement(tag);
	return Color(static_cast<uint8_t>(readInt(elem, "r", 255)),
		static_cast<uint8_t>(readInt(elem, "g", 255)),
		static_cast<uint8_t>(readInt(elem, "b", 255)),
		static_cast<uint8_t>(readInt(elem, "a", 255)));
}

std::vector<Emitter> Emitter::loadFromFile(const std::string& path)
{
	std::vector<Emitter> emitters;

	XMLDocument doc;
	if (doc.LoadFile(path.c_str()) != XML_SUCCESS) {
		std::cerr << "Cannot load emitter file: " << path << " (" << doc.ErrorStr() << ")\n";
		return emitters;
	}

	const XMLElement* root = doc.FirstChildElement("EmitterConfig");
	if (!root) {
		std::cerr << "Missing <EmitterConfig> in " << path << "\n";
		return emitters;
	}

	for (const XMLElement* elem = root->FirstChildElement("emitter"); elem != nullptr; elem = elem->NextSiblingElement("emitter"))
	{
		Emitter emitter;

		const char* type = elem->Attribute("type");
		emitter.type_ = type ? type : "";

		emitter.thetaRange_ = readRange(elem, "thetaRange");
		emitter.angleRange_ = readRange(elem, "angleRange");

		emitter.rotSpeed_ = readRandomized(elem, "rotSpeed");
		emitter.startSpeed_ = readRandomized(elem, "startSpeed");
		emitter.endSpeed_ = readRandomized(elem, "endSpeed");
		emitter.startSize_ = readRandomized(elem, "startSize");
		emitter.endSize_ = readRandomized(elem, "endSize");
		emitter.emitVariance_ = readRandomized(elem, "emitVariance");
		emitter.maxParticleLife_ = readRandomized(elem, "maxParticleLife");

		if (const XMLElement* number = elem->FirstChildElement("emitNumber"))
			emitter.emitNumber_ = readInt(number, "value", 1);

		if (const XMLElement* life = elem->FirstChildElement("lifetime"))
			emitter.lifetime_ = readFloat(life, "value", -1.f);

		emitter.startColor_ = readColor(elem, "startColor");
		emitter.endColor_ = readColor(elem, "endColor");

		// A voir pour le rajout du blend 

		emitters.push_back(std::move(emitter));
	}
	return emitters;
}

int Emitter::rollEmitCount(std::mt19937& rng) const
{
	return emitNumber_ + static_cast<int>(emitVariance_.randomized(rng));
}

float Emitter::rollTheta(std::mt19937& rng) const
{
	return std::uniform_real_distribution<float>(thetaRange_.min, thetaRange_.max)(rng);
}

float Emitter::rollAngle(std::mt19937& rng) const
{
	return std::uniform_real_distribution<float>(angleRange_.min, angleRange_.max)(rng);
}

int Emitter::getPoolSize() const
{
	return (maxParticleLife_.value + 1) * emitNumber_;
}
