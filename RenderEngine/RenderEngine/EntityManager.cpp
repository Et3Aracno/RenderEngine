#include "EntityManager.h"
#include <stdexcept>

EntityManager::EntityManager()
{
}

EntityManager::~EntityManager()
{
	for (IComponentManager* manager : componentManagers_)
		delete manager;
}

int EntityManager::createEntity()
{
	int entity;
	if (!freeEntityIds_.empty())
	{
		entity = freeEntityIds_.back();
		freeEntityIds_.pop_back();
	}
	else
	{
		assert(nextEntityId_ < MAX_ENTITIES);

		entity = nextEntityId_++;
	}
	alive_[entity] = true;
	return entity;
}

void EntityManager::destroyEntity(int entity)
{
	assert(entity >= 0 && entity < MAX_ENTITIES && alive_[entity]);

	for (int c = 0; c < static_cast<int>(componentManagers_.size()); ++c)
	{
		if (entityComponents_[entity].test(c))
			componentManagers_[c]->removeComponent(entity);
	}
	entityComponents_[entity].reset();
	alive_[entity] = false;
	freeEntityIds_.push_back(entity);
}




void EntityManager::addComponent(int entity, int componentID, void* value)
{
	if (entity < 0 || entity >= MAX_ENTITIES || !alive_[entity])
		throw std::out_of_range("entité invalide");

	if (componentID < 0 || componentID >= static_cast<int>(componentManagers_.size()))
		throw std::out_of_range("componentID invalide");

	componentManagers_[componentID]->addComponent(entity, value);
	entityComponents_[entity].set(componentID);
}

void EntityManager::removeComponent(int entity, int componentID)
{
	//besoin d'ajouter un verif if pour le release histoire de sécuriser
	assert(componentID >= 0 && componentID < static_cast<int>(componentManagers_.size()));

	componentManagers_[componentID]->removeComponent(entity);
	entityComponents_[entity].reset(componentID);
}

bool EntityManager::hasComponent(int entity, int componentID) const
{
	return entityComponents_[entity].test(componentID) && componentManagers_[componentID]->hasComponent(entity);
}

void* EntityManager::getComponent(int entity, int componentID)
{
	//besoin d'ajouter un verif if pour le release histoire de sécuriser
	assert(componentID >= 0 && componentID < static_cast<int>(componentManagers_.size()));

	return componentManagers_[componentID]->getComponent(entity);
}

std::vector<int> EntityManager::fetchAllEntities(std::initializer_list<int> componentIDs) const
{
	std::bitset<MAX_COMPONENTS> query;
	for (int id : componentIDs)
		query.set(id);

	std::vector<int> result;
	for (int e = 0; e < nextEntityId_; ++e)
	{
		if (alive_[e] && (entityComponents_[e] & query) == query)
			result.push_back(e);
	}
	return result;
}

