#pragma once
#include <array>
#include <memory>
#include <vector>

#include "ComponentManager.h"




class EntityManager
{
public:
	EntityManager();
	~EntityManager();


	template <typename T>
	int createManager();
	void addComponent(int managerIndex, int componentIndex);
	void*& getComponent(int managerIndex , int componentIndex);
	bool hasComponent(int managerIndex, int componentIndex) const ;

private:
	std::vector <IComponentManager*> componentManagers;
};


