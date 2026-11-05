#pragma once
#include "ComponentManager.h"

template<typename T>
ComponentManager<T>::ComponentManager()
{
	hasComponentArray_.fill(false);
}

template<typename T>
void ComponentManager<T>::addComponent(int entity, void* value)
{
	//besoin d'ajouter un verif if pour le release histoire de sécuriser
	assert(entity >= 0 && entity < MAX_ENTITIES);
	componentStorage_[entity] = componentStorage_[entity].emplace(*static_cast<T*>(value));
	hasComponentArray_[entity] = true;
}

template<typename T>
void ComponentManager<T>::removeComponent(int entity)
{
	//besoin d'ajouter un verif if pour le release histoire de sécuriser
	assert(entity >= 0 && entity < MAX_ENTITIES);
	componentStorage_[entity].reset();
	hasComponentArray_[entity] = false;
}

template<typename T>
bool ComponentManager<T>::hasComponent(int entity) const
{
	//besoin d'ajouter un verif if pour le release histoire de sécuriser
	assert(entity >= 0 && entity < MAX_ENTITIES);
	return hasComponentArray_[entity];
}

template<typename T>
void* ComponentManager<T>::getComponent(int entity)
{
	//besoin d'ajouter un verif if pour le release histoire de sécuriser 
	assert(hasComponentArray_[entity] && "No component at index");
	return &*componentStorage_[entity];
}