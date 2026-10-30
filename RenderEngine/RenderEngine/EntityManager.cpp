#include "EntityManager.h"


EntityManager::EntityManager()
{
}

EntityManager::~EntityManager()
{
}


void EntityManager::addComponenet(int managerIndex, int componentIndex)
{
	//static_cast<T*>(componentManagers[managerIndex]);
	componentManagers[managerIndex]->addComponent(componentIndex, ));

}

bool EntityManager::hasComponent(int managerIndex, int componentIndex) const
{
	assert(componentManagers[managerIndex]);
	return componentManagers[managerIndex]->hasComponent(componentIndex);
}


template<typename T>
inline int EntityManager::createManager()
{
	ComponentManager<T> manager;
	componentManagers.push_back(&manager); //need rework
}





