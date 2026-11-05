#pragma once
#include "EntityManager.h"

template<typename T>
inline int EntityManager::createComponentStorage()
{
	assert(componentManagers_.size() < MAX_COMPONENTS);

	componentManagers_.push_back(new ComponentManager<T>());
	return static_cast<int>(componentManagers_.size()) - 1;
}

