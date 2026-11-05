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



	void  addComponent(int entity, int componentId, void* value);
	void  removeComponent(int entity, int componentId);
	bool  hasComponent(int entity, int componentId) const;
	void* getComponent(int entity, int componentId);

	std::vector<int> fetchAllEntities(std::initializer_list<int> componentIds) const;

private:
	int nextEntityId_ = 0;
	std::vector<int> freeEntityIds_;
	std::array<bool, MAX_ENTITIES> alive_{};
	std::array<std::bitset<MAX_COMPONENTS>, MAX_ENTITIES> entityComponents_{};
	std::vector<IComponentManager*> componentManagers_;
};

#include "EntityManager.inl"
