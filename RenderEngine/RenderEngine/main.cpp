#include <iostream>
#include <memory>
#include "Renderer.h"
#include "ECS.h"

int main()
{
	ECS ecs;
	EntityManager& em = ecs.getEntityManager();

	int rendererID = em.createComponentStorage<Renderer>();   
	int entity = em.createEntity();                       

	Renderer renderer;
	em.addComponent(entity, rendererID, &renderer); 

	ecs.addSystem([rendererID](EntityManager& em) {           
		for (int e : em.fetchAllEntities({ rendererID }))     
		{
			Renderer* r = static_cast<Renderer*>(em.getComponent(e, rendererID));
			
			r->init();
			r->update();                                      
		}
	});

	ecs.executeSystems();                                     
	return 0;
}
