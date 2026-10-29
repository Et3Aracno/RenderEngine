#include "ComponentManager.h"


template<typename T>
inline void ComponentManager<T>::addComponent(int entity, T& value)
{
	assert(entity >= 0 && entity < 100);
	GetcomponentStorage()[entity] = value;
	GethasComponentArray()[entity] = true;
}

template<typename T>
inline T& ComponentManager<T>::GetComponent(int entity)
{
	assert(hasComponent(entity) && "No component at index");
	return GetcomponentStorage().at(entity);
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