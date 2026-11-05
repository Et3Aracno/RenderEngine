#pragma once
#include <array>
#include <cassert>
#include <optional>

constexpr int MAX_ENTITIES = 100;
constexpr int MAX_COMPONENTS = 64;

class IComponentManager
{
public:
	virtual ~IComponentManager() = default;

	virtual void  addComponent(int entity, void* value) = 0;   // value = pointeur vers un T à copier
	virtual void  removeComponent(int entity) = 0;
	virtual bool  hasComponent(int entity) const = 0;
	virtual void* getComponent(int entity) = 0;                // adresse du composant
};



template <typename T>
class ComponentManager : public IComponentManager
{
private:
	std::array<std::optional<T>, MAX_ENTITIES> componentStorage_;
	std::array<bool, MAX_ENTITIES> hasComponentArray_;

public:
	ComponentManager();
	~ComponentManager() override = default;

	void  addComponent(int entity, void* value) override;
	void  removeComponent(int entity) override;
	bool  hasComponent(int entity) const override;
	void* getComponent(int entity) override;
};

#include "ComponentManager.inl"
