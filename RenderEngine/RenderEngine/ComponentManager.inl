#include "ComponentManager.h"


template<typename T>
inline void ComponentManager<T>::addComponent(int entity, T& value)
{
	GetcomponentStorage()[entity] = value;
	GethasComponentArray()[entity] = true;
}



template<typename T>
inline T& ComponentManager<T>::GetComponent(int entity)
{
	static_assert(hasComponent(entity),"No component at index")
	return GetcomponentStorage().at(entity);
}

template<typename T>
inline void ComponentManager<T>::removeComponent(int entity)
{ 
	GethasComponentArray()[entity] = false;
}

template<typename T>
inline bool ComponentManager<T>::hasComponent(int entity)
{
	return GethasComponentArray().at(entity);
	GethasComponentArray().at(entity);
	return true;
}
