#include <iostream>
#include <memory>
#include "Renderer.h"
#include "EntityManager.h"

int main()
{
	EntityManager em;
	em.createManager<Renderer>();


	Renderer renderer;
	renderer.init();
	renderer.update();
	return 0;
}

