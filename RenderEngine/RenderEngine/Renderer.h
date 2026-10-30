#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <atomic>
#include <barrier>
#include <memory>
#include <thread>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits>
#include "External Lib/MathLibrary/MathLib.h"
#include "Color.h"
#include "GLFW/glfw3.h"


class Mesh;
class ClipVertex;
class Material;



struct ScreenTri {
	int   x[3], y[3];
	float z[3], iw[3];
	float uw[3], vw[3];
	float nx[3], ny[3], nz[3];
	float invArea;
	int   minx, miny, maxx, maxy;
};

struct Tile { int x0, y0, x1, y1; };



class Renderer {

private:

	//
	static constexpr int defaultWidth = 800;
	static constexpr int defaultHeight = 600;
	static constexpr int tileSize = 32; // 32 ou 64 mais same pour le test a voir ave ajout du binning
	static constexpr int SizeWxH = defaultWidth * defaultHeight;
	Color defaultBcColor;
	static Renderer* r_instance;
	GLFWwindow* window = nullptr;

	static inline std::vector<std::thread> threads;
	static inline std::unique_ptr<std::barrier<>> startBarrier;
	static inline std::unique_ptr<std::barrier<>> endBarrier;
	static inline std::atomic<int> nextTile{ 0 };
	static inline std::atomic<bool> quit{ false };
	

	static inline std::vector<Tile> tiles;
	static inline std::vector<ScreenTri> tris;
	static inline int tilesX, tilesY;

	//


	//
	static int edge(Maths::Vec2S v1, Maths::Vec2S v2, Maths::Vec2S P);
	static void clearColor(uint32_t color); //////////// A VOIR
	static void clearZbuffer();
	static inline int clampi(int v, int lo, int hi);
	static void compute_face_normals(const Mesh* mesh, Maths::Vec3S* out);
	static void compute_vertex_normals(const Mesh* mesh, const Maths::Vec3S* face_normals, Maths::Vec3S* out);

	static void StartThreadPool(const Material* mat);
	static void StopThreadPool();
	static void WorkerLoop(const Material* mat);

protected:

	static void BuildTiles(); // Done once or window changes size
	static void RenderTile(int tileIndex, const Material* mat);
	static inline void ProcessTiles(const Material* mat);
	static void RenderAllTiles(const Material* mat);

	static void drawMeshTextured(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp, Maths::Matrix4x4SIMD model, Maths::Vec3S* face_normals, Maths::Vec3S* vertex_normals, const Material* mat);
	static void project(const ClipVertex* tri);
	void drawPx(int sizeX, int sizeY);
	static inline void put_pixel(int x, int y, Color color);
	static void draw_line(Maths::Vec2S x , Maths::Vec2S y, uint32_t color);
	void clearAllBuffer(); 

	static void RasterizeTri(const ScreenTri& t, const Tile& tile, const Material* mat);


public:

	static uint32_t framebuffer[defaultWidth * defaultHeight];
	static float    zbuffer[defaultWidth * defaultHeight];

	void init();
	void update();


	Renderer();
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	static Renderer* Instance() { return r_instance; }


	Color GetBcColor() const { return defaultBcColor; }
	void SetBcColor(Color color) { defaultBcColor = color; }
	static constexpr int GetSizeWxH() { return SizeWxH; }

};