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


	if (!path.ends_with(".obj")) 
	{
		std::cout << "File format not supported. Please provide a .obj file." << std::endl;
		return false;
	}
	//Potentiellement ajouter un check du format du fichier pour s'assurer que c'est bien un .obj
	//pas besoin de check si c'est un fbx ou autre car on ne gère pas ce format, on ne fait que du .obj

	mesh.verts.clear();
	mesh.normals.clear();
	mesh.textures.clear();
	mesh.faces.clear();
	mesh.texFaces.clear();
	mesh.normFaces.clear();


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
				mesh.textures.push_back(Maths::Vec2S(u,v));
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
				// Découpe "v", "v/vt", "v//vn" ou "v/vt/vn" en 3 morceaux
				std::stringstream ts(token);
				std::string v_str, vt_str, vn_str;
				std::getline(ts, v_str, '/');
				std::getline(ts, vt_str, '/');
				std::getline(ts, vn_str, '/');

				// Position (toujours présente)
				mesh.faces.push_back(std::stoi(v_str) - 1);

				
				mesh.texFaces.push_back(vt_str.empty() ? -1 : std::stoi(vt_str) - 1);

				
				mesh.normFaces.push_back(vn_str.empty() ? -1 : std::stoi(vn_str) - 1);
			}
		}
		// toute autre ligne est ignorée
	}

	return true;
}