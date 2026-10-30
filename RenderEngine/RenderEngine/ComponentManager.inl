#pragma once
#include "ComponentManager.h"

template<typename T>
inline void ComponentManager<T>::addComponent(int entity, void*& value)
{
	assert(entity >= 0 && entity < 100);
	componentStorage[entity] = value;
	hasComponentArray[entity] = true;
}

template<typename T>
inline void*& ComponentManager<T>::getComponent(int entity)
{
	assert(hasComponent(entity) && "No component at index");
	return componentStorage[entity];
}

template <typename T>
void ComponentManager<T>::removeComponent(int entity)
{
	assert(entity >= 0 && entity < 100);
	hasComponentArray[entity] = false;
}

template <typename T>
bool ComponentManager<T>::hasComponent(int entity) const
{
	assert(entity >= 0 && entity < 100);
	return hasComponentArray[entity];
}