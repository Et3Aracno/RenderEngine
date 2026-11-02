#pragma once
#include "EntityManager.h"

template<typename T>
inline int EntityManager::createComponentStorage()
{
	assert(componentManagers.size() < MAX_COMPONENTS);

	componentManagers.push_back(new ComponentManager<T>());
	return static_cast<int>(componentManagers.size()) - 1;
}

