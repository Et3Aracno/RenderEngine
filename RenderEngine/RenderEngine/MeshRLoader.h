#pragma once
#include <string>
#include <vector>
#include "External Lib/MathLibrary/MathLib.h"

struct Mesh
{
	std::vector<Maths::Vec3S> verts;
	std::vector<Maths::Vec3S> normals;
	std::vector<Maths::Vec2S> textures;
	std::vector<int> texFaces;
	std::vector<int> normFaces;
	std::vector<int> faces;   // 3 ints par triangle
};

class MeshLoader
{
public:
	static bool loadMesh(const std::string& path, Mesh& mesh);
};