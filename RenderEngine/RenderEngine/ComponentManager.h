#pragma once
#include <array>
#include <cassert>



class IComponentManager
{
public:
	virtual ~IComponentManager() = default;

	virtual void removeComponent(int entity) = 0;
	virtual bool hasComponent(int entity) const = 0;
};


template <typename T>
class ComponentManager
{
private:

	//memo : Je vais changer mes méthodes de add ect , elles vont check si ce qui à mis en entrer est une enfnat de IComponent , si oui on ajoute à l'array . si non on return
	//CompManager va etre dans chaque object , chacun d'entre eux aura son propre comp manager , donc pas de skeleton dessus . prévoir un constructeur 
	// prévoir aussi la manière de gerer le check d'héritage sur Icompt , passer le comp manager en comp manager final pour éviter tout héritage non voulu
	// ne pas mettre de template changer pour void* et test si ça fonctionne comme prévu , préparer chacun des component avec un Getqui return l'intance du component 
	// exemple compMaterial.get() me permet d'acceder à Material et ses fonction, derrière je peux faire .rdPass()
	// j'ai l'impression de que c'est pas la bonne manière , besoin de demander demain pour etre aiguiller .

	 std::array<T, 100> componentStorage;
	 std::array<bool, 100> hasComponentArray;


public:

	ComponentManager();
	~ComponentManager();

	void addComponent(int entity, T& value);
	T& GetComponent(int entity);
	void removeComponent(int entity);
	bool hasComponent(int entity) const;

};

#include "ComponentManager.inl"
