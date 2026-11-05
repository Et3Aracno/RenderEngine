#pragma once
#include <functional>
#include <vector>

#include "EntityManager.h"


using System = std::function<void(EntityManager&)>;


class ECS
{
public:
	// Retourne une référence à l'EntityManager pour ajouter/modifier des entités depuis l'extérieur
	EntityManager& getEntityManager() { return em_; }

	// Ajoute un nouveau système à la liste des systèmes à exécuter
	void addSystem(System system) { systems_.push_back(std::move(system)); }

	// Exécute tous les systèmes enregistrés en passant l'EntityManager à chacun d'eux
	// Cela signifie que chaque système peut accéder aux entités et les modifier
	void executeSystems()
	{
		for (System& system : systems_)
			system(em_);  // Le système traite toutes les entités via l'EntityManager
	}

private:
	EntityManager em_;            // Gestionnaire de toutes les entités du jeu
	std::vector<System> systems_;       // Liste de tous les systèmes à exécuter chaque frame
};				