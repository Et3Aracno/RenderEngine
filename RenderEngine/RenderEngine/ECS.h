#pragma once
#include <functional>
#include <vector>

#include "EntityManager.h"

// Définition d'un type "System" : c'est une fonction qui prend en paramètre une référence à EntityManager
// Les Systèmes sont les logiques de jeu (physique, collision, rendu, etc.) qui traitent les entités
using System = std::function<void(EntityManager&)>;

// Classe ECS (Entity Component System) : c'est le gestionnaire principal du pattern ECS
// ECS est une architecture qui sépare les données (Entités et Composants) de la logique (Systèmes)
// Cela permet une meilleure flexibilité et réutilisabilité du code
class ECS
{
public:
	// Retourne une référence à l'EntityManager pour ajouter/modifier des entités depuis l'extérieur
	EntityManager& getEntityManager() { return em; }

	// Ajoute un nouveau système à la liste des systèmes à exécuter
	// Exemple : ajouter un système de un système de rendu, etc.
	void addSystem(System system) { systems.push_back(std::move(system)); }

	// Exécute tous les systèmes enregistrés en passant l'EntityManager à chacun d'eux
	// Cela signifie que chaque système peut accéder aux entités et les modifier
	void executeSystems()
	{
		for (System& system : systems)
			system(em);  // Le système traite toutes les entités via l'EntityManager
	}

private:
	EntityManager em;            // Gestionnaire de toutes les entités du jeu
	std::vector<System> systems;       // Liste de tous les systèmes à exécuter chaque frame
};				