#pragma once
#include <array>
#include <cassert>



class IComponentManager
{
public:
	virtual ~IComponentManager() = default;


	virtual void removeComponent(int entity) = 0;
	virtual bool hasComponent(int entity) const = 0;
	virtual void addComponent(int entity, void*& value) = 0;
	virtual void*& getComponent(int entity) = 0;
};

template <typename T>
class ComponentManager : public IComponentManager
{
private:


	 std::array<void*, 100> componentStorage;
	 std::array<bool, 100> hasComponentArray;


public:



	ComponentManager();
	~ComponentManager();

	void addComponent(int entity, void*& value) override;
	void*& getComponent(int entity) override;
	void removeComponent(int entity) override ;
	bool hasComponent(int entity) const override;

};

#include "ComponentManager.inl"
