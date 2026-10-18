#include "MeshRLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool MeshLoader::loadMesh(const std::string& path, Mesh& mesh)
{
	std::ifstream file(path);
	if (!file)
	{
		std::cerr << "Failed to open file: " << path << std::endl;
		return false;
	}

	//Potentiellement ajouter un check du format du fichier pour s'assurer que c'est bien un .obj
	//pas besoin de check si c'est un fbx ou autre car on ne gère pas ce format, on ne fait que du .obj

	mesh.verts.clear();
	mesh.normals.clear();
	mesh.faces.clear();

	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string type;
		iss >> type;               // lit le premier mot : "v", "vn", "f", "#"...

		if (type == "v")
		{
			float x, y, z;
			if (iss >> x >> y >> z)
				mesh.verts.push_back(Maths::Vec3S(x, y, z));
		}
		else if (type == "vn")
		{
			float x, y, z;
			if (iss >> x >> y >> z)
				mesh.normals.push_back(Maths::Vec3S(x, y, z));
		}
		else if (type == "f")
		{
			std::string token;
			while (iss >> token)   // "1", "1/2", "1/2/3" ou "1//3"
			{
				int index = std::stoi(token);   // lit le nombre du début, s'arrête au '/'
				mesh.faces.push_back(index - 1); // OBJ commence à 1
			}
		}
		// toute autre ligne est ignorée
	}

	return true;
}