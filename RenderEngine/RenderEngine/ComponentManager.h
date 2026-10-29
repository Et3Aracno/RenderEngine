#pragma once
#include <array>
template <typename T>
class ComponentManager
{
private:

	const std::array<T, 100> componentStorage;
	const std::array<bool, 100> hasComponentArray;


public:

	ComponentManager();
	~ComponentManager();

	void addComponent(int entity, T& value);
	void removeComponent(int entity);
	bool hasComponent(int entity);
	T& GetComponent(int entity);


	std::array<T, 100>& GetcomponentStorage() const { return componentStorage; }
	std::array<bool, 100>& GethasComponentArray() const { return hasComponentArray; }


};


#include "ComponentManager.inl"
