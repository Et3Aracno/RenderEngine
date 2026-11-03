#pragma once
#include <array>
#include <bitset>
#include <cassert>
#include <initializer_list>
#include <vector>

#include "ComponentManager.h"




class EntityManager
{
public:
	EntityManager();
	~EntityManager();

	EntityManager(const EntityManager&) = delete;
	EntityManager& operator=(const EntityManager&) = delete;


	template <typename T> int createComponentStorage();

	int  createEntity();
	void destroyEntity(int entity);



	void  addComponent(int entity, int componentID, void* value);
	void  removeComponent(int entity, int componentID);
	bool  hasComponent(int entity, int componentID) const;
	void* getComponent(int entity, int componentID);

	std::vector<int> fetchAllEntities(std::initializer_list<int> componentIDs) const;

private:
	int nextEntityID = 0;
	std::vector<int> freeEntityIDs;
	std::array<bool, MAX_ENTITIES> alive{};
	std::array<std::bitset<MAX_COMPONENTS>, MAX_ENTITIES>  entityComponents{};
	std::vector<IComponentManager*> componentManagers;
};

#include "EntityManager.inl"
