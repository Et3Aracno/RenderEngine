#include "EntityManager.h"


EntityManager::EntityManager()
{
}

EntityManager::~EntityManager()
{
	for (IComponentManager* manager : componentManagers)
		delete manager;
}

int EntityManager::createEntity()
{
	int entity;
	if (!freeEntityIDs.empty())
	{
		entity = freeEntityIDs.back();
		freeEntityIDs.pop_back();
	}
	else
	{
		assert(nextEntityID < MAX_ENTITIES);

		entity = nextEntityID++;
	}
	alive[entity] = true;
	return entity;
}

void EntityManager::destroyEntity(int entity)
{
	assert(entity >= 0 && entity < MAX_ENTITIES && alive[entity]);

	for (int c = 0; c < static_cast<int>(componentManagers.size()); ++c)
	{
		if (entityComponents[entity].test(c))
			componentManagers[c]->removeComponent(entity);
	}
	entityComponents[entity].reset();
	alive[entity] = false;
	freeEntityIDs.push_back(entity);
}




void EntityManager::addComponent(int entity, int componentID, void* value)
{
	assert(alive[entity]);
	assert(componentID >= 0 && componentID < static_cast<int>(componentManagers.size()));

	componentManagers[componentID]->addComponent(entity, value);
	entityComponents[entity].set(componentID);
}

void EntityManager::removeComponent(int entity, int componentID)
{
	assert(componentID >= 0 && componentID < static_cast<int>(componentManagers.size()));

	componentManagers[componentID]->removeComponent(entity);
	entityComponents[entity].reset(componentID);
}

bool EntityManager::hasComponent(int entity, int componentID) const
{
	return entityComponents[entity].test(componentID);
}

void* EntityManager::getComponent(int entity, int componentID)
{
	assert(componentID >= 0 && componentID < static_cast<int>(componentManagers.size()));

	return componentManagers[componentID]->getComponent(entity);
}

std::vector<int> EntityManager::fetchAllEntities(std::initializer_list<int> componentIDs) const
{
	std::bitset<MAX_COMPONENTS> query;
	for (int id : componentIDs)
		query.set(id);

	std::vector<int> result;
	for (int e = 0; e < nextEntityID; ++e)
	{
		if (alive[e] && (entityComponents[e] & query) == query)
			result.push_back(e);
	}
	return result;
}

