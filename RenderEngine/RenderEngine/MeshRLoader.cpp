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

		else if (type == "vt")
		{
			float u, v;
			if (iss >> u >> v)
				mesh.texture.push_back(Maths::Vec2S(u,v));
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
			while (iss >> token)
			{
				int index = std::stoi(token);   // lit le nombre du début, s'arrête au '/'
				mesh.faces.push_back(index - 1); // OBJ commence à 1
			}
			std::size_t slash = token.find('/');
		}
		// toute autre ligne est ignorée
	}

	return true;
}