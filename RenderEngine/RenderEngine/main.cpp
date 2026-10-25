#include <iostream>
#include <vector>
#include <algorithm>
#include <atomic>
#include <barrier>
#include <memory>
#include <thread>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits>
#include "GLFW/glfw3.h"
#include "External Lib/MathLibrary/MathLib.h"
#include "MeshRLoader.h"
#include "Texture.h"
#include "clip.h"

#define WIDTH  800
#define HEIGHT 600
#define PI 3.14159265358979323846f

#define CULL_BACKFACES true
#define USE_ZBUFFER true
#define PERSPECTIVE_CORRECT true
#define BILINEAR true
#define AMBIENT 0.18f
#define SPEC_STRENGTH  0.55f

#define TILE_SIZE 32   // 32 ou 64 mais same pour le test a voir ave ajout du binning

static uint32_t framebuffer[WIDTH * HEIGHT];
static float    zbuffer[WIDTH * HEIGHT];

static const uint32_t CLEAR_COLOR = 0xFF000000u | (24u << 16) | (16u << 8) | 16u; // = rgb(16,16,24)

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)0xFFu << 24) | ((uint32_t)b << 16)
		| ((uint32_t)g << 8) | (uint32_t)r;
}

inline int edge(int x0, int y0, int x1, int y1, int Px, int Py)
{
	return (x1 - x0) * (Py - y0) - (y1 - y0) * (Px - x0);
}

static inline int clampi(int v, int lo, int hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}

static inline Maths::Vec4S FromPoint(Maths::Vec3S p) {
	Maths::Vec4S v = { p.x, p.y, p.z, 1.0f };
	return v;
}

static void compute_face_normals(const Mesh* mesh, Maths::Vec3S* out) {
	for (int f = 0; f < mesh->faces.size() / 3; f++) {
		Maths::Vec3S v0 = mesh->verts[mesh->faces[f * 3 + 0]];
		Maths::Vec3S v1 = mesh->verts[mesh->faces[f * 3 + 1]];
		Maths::Vec3S v2 = mesh->verts[mesh->faces[f * 3 + 2]];
		Maths::Vec3S e1 = v1 - v0;
		Maths::Vec3S e2 = v2 - v0;
		out[f] = (e1.Cross(e2)).Normalize();
	}
}

static void compute_vertex_normals(const Mesh* mesh, const Maths::Vec3S* face_normals, Maths::Vec3S* out) {
	for (int i = 0; i < mesh->verts.size(); i++) out[i] = Maths::Vec3S(0, 0, 0);
	for (int f = 0; f < mesh->faces.size() / 3; f++) {
		for (int k = 0; k < 3; k++) {
			int v = mesh->faces[f * 3 + k];
			out[v] = out[v] + face_normals[f];
		}
	}
	for (int i = 0; i < mesh->verts.size(); i++) out[i] = out[i].Normalize();
}

static inline uint32_t color_phong(uint32_t texel, float diffuse_k, float spec_k) {
	float r = (float)((texel >> 16) & 0xFF) * diffuse_k + 255.0f * spec_k;
	float g = (float)((texel >> 8) & 0xFF) * diffuse_k + 255.0f * spec_k;
	float b = (float)(texel & 0xFF) * diffuse_k + 255.0f * spec_k;
	if (r > 255.0f) r = 255.0f;
	if (g > 255.0f) g = 255.0f;
	if (b > 255.0f) b = 255.0f;
	return ((uint32_t)0xFFu << 24)
		| ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

struct ScreenTri {
	int   x[3], y[3];
	float z[3], iw[3]; 
	float uw[3], vw[3];  
	float nx[3], ny[3], nz[3];  
	float invArea;
	int   minx, miny, maxx, maxy; 
};

struct Tile { int x0, y0, x1, y1; }; 

// No lock car read only
static std::vector<Tile> tiles;
static std::vector<ScreenTri> tris;
static int tilesX = 0, tilesY = 0;

static const Texture* tex = nullptr;
static Maths::Vec3S L, H;

// Only once
static void BuildTiles()
{
	tiles.clear();
	tilesY = (HEIGHT + TILE_SIZE - 1) / TILE_SIZE;
	for (int ty = 0; ty < tilesY; ty++)
		for (int tx = 0; tx < tilesX; tx++)
			tiles.push_back({
				tx * TILE_SIZE, ty * TILE_SIZE,
				std::min((tx + 1) * TILE_SIZE, WIDTH),
				std::min((ty + 1) * TILE_SIZE, HEIGHT) });
}

static void projectAndQueue(const ClipVertex* tri)
{
	int   sx[3], sy[3];
	float ndcZ[3], invW[3];
	for (int k = 0; k < 3; k++) {
		float inv = 1.0f / tri[k].clip.w;
		sx[k] = (int)((tri[k].clip.x * inv + 1.0f) * 0.5f * WIDTH);
		sy[k] = (int)((tri[k].clip.y * inv + 1.0f) * 0.5f * HEIGHT);
		ndcZ[k] = tri[k].clip.z * inv;
		invW[k] = inv;
	}

	int area = edge(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]);
	if (area <= 0) return;  

	int rminx = std::min({ sx[0], sx[1], sx[2] });
	int rmaxx = std::max({ sx[0], sx[1], sx[2] });
	int rminy = std::min({ sy[0], sy[1], sy[2] });
	int rmaxy = std::max({ sy[0], sy[1], sy[2] });

	// bye bye not on screen
	if (rmaxx < 0 || rminx >= WIDTH || rmaxy < 0 || rminy >= HEIGHT) return;

	ScreenTri st;
	for (int k = 0; k < 3; k++) {
		st.x[k] = sx[k];  st.y[k] = sy[k];
		st.z[k] = ndcZ[k]; st.iw[k] = invW[k];
		st.uw[k] = tri[k].uv.x * invW[k];
		st.vw[k] = tri[k].uv.y * invW[k];
		st.nx[k] = tri[k].normal.x * invW[k];
		st.ny[k] = tri[k].normal.y * invW[k];
		st.nz[k] = tri[k].normal.z * invW[k];
	}
	st.invArea = 1.0f / (float)area;
	st.minx = clampi(rminx, 0, WIDTH - 1);
	st.maxx = clampi(rmaxx, 0, WIDTH - 1);
	st.miny = clampi(rminy, 0, HEIGHT - 1);
	st.maxy = clampi(rmaxy, 0, HEIGHT - 1);

	tris.push_back(st);  //a voir encore avec le binning 
}

// taille box tuile
static void RasterizeTriInTile(const ScreenTri& t, const Tile& tile)
{
	int minx = std::max(t.minx, tile.x0);
	int maxx = std::min(t.maxx, tile.x1 - 1);
	int miny = std::max(t.miny, tile.y0);
	int maxy = std::min(t.maxy, tile.y1 - 1);
	if (minx > maxx || miny > maxy) return;

	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			int e0 = edge(t.x[1], t.y[1], t.x[2], t.y[2], x, y);
			int e1 = edge(t.x[2], t.y[2], t.x[0], t.y[0], x, y);
			int e2 = edge(t.x[0], t.y[0], t.x[1], t.y[1], x, y);
			if ((e0 | e1 | e2) < 0) continue;

			float l0 = e0 * t.invArea;
			float l1 = e1 * t.invArea;
			float l2 = e2 * t.invArea;

			float z = l0 * t.z[0] + l1 * t.z[1] + l2 * t.z[2];
			int idx = y * WIDTH + x;
			if (z >= zbuffer[idx]) continue;

			float iw = l0 * t.iw[0] + l1 * t.iw[1] + l2 * t.iw[2];
			float inv_iw = 1.0f / iw;

			float u = (l0 * t.uw[0] + l1 * t.uw[1] + l2 * t.uw[2]) * inv_iw;
			float v = (l0 * t.vw[0] + l1 * t.vw[1] + l2 * t.vw[2]) * inv_iw;

			Maths::Vec3S N = Maths::Vec3S(
				(l0 * t.nx[0] + l1 * t.nx[1] + l2 * t.nx[2]) * inv_iw,
				(l0 * t.ny[0] + l1 * t.ny[1] + l2 * t.ny[2]) * inv_iw,
				(l0 * t.nz[0] + l1 * t.nz[1] + l2 * t.nz[2]) * inv_iw);
			N = N.Normalize();

			float ndotl = N.Dot(L);
			if (ndotl < 0.0f) ndotl = 0.0f;
			float diffuse_k = AMBIENT + (1.0f - AMBIENT) * ndotl;

			float ndoth = N.Dot(H);
			if (ndoth < 0.0f) ndoth = 0.0f;
			float spec_k = std::powf(ndoth, 32) * SPEC_STRENGTH;

#if BILINEAR
			uint32_t texel = tex->sampleBilinear(u, v);
#else
			uint32_t texel = g_tex->sample(u, v);
#endif
			zbuffer[idx] = z;
			framebuffer[idx] = color_phong(texel, diffuse_k, spec_k);
		}
	}
}

// clear + rasterize
static void RenderTile(int tileIndex)
{
	const Tile& tile = tiles[tileIndex];

	for (int y = tile.y0; y < tile.y1; y++) {
		std::fill(framebuffer + y * WIDTH + tile.x0, framebuffer + y * WIDTH + tile.x1, CLEAR_COLOR);
		std::fill(zbuffer + y * WIDTH + tile.x0, zbuffer + y * WIDTH + tile.x1, std::numeric_limits<float>::max());
	}

	// si pas ds boxTile dégage
	for (const ScreenTri& tri : tris)
		RasterizeTriInTile(tri, tile);
}


static std::vector<std::thread> threads;
static std::unique_ptr<std::barrier<>> startBarrier;
static std::unique_ptr<std::barrier<>> endBarrier;
static std::atomic<int> nextTile{ 0 };
static std::atomic<bool> quit{ false };


static inline void ProcessTiles()
{
	int t;
	while ((t = nextTile.fetch_add(1)) < (int)tiles.size())
		RenderTile(t);
}

static void WorkerLoop()
{
	for (;;) {
		startBarrier->arrive_and_wait(); 
		if (quit.load()) return;
		ProcessTiles();
		endBarrier->arrive_and_wait();     
	}
}

static void StartThreadPool()
{
	int nbT = (int)std::thread::hardware_concurrency();
	if (nbT < 1) nbT = 1;
	int workers = nbT - 1;                 

	startBarrier = std::make_unique<std::barrier<>>(workers + 1);
	endBarrier = std::make_unique<std::barrier<>>(workers + 1);
	for (int i = 0; i < workers; i++)
		threads.emplace_back(WorkerLoop);
}

static void StopThreadPool()
{
	quit = true;
	startBarrier->arrive_and_wait();      // wakey wakey
	for (auto& t : threads) t.join();
	threads.clear();
}

static void RenderAllTiles()
{
	nextTile = 0;
	startBarrier->arrive_and_wait();      
	ProcessTiles();                         
	endBarrier->arrive_and_wait();       
}


static void drawMeshTextured(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp, Maths::Matrix4x4SIMD model,
	const Texture& tex_, Maths::Vec3S* face_normals, Maths::Vec3S* vertex_normals)
{
	const int vcount = (int)mesh.verts.size();

	L = (Maths::Vec3S(0.4f, 0.8f, 0.45f)).Normalize();
	const Maths::Vec3S view_dir = Maths::Vec3S(0, 0, 1);
	H = (L + view_dir).Normalize();
	tex = &tex_;

	static std::vector<Maths::Vec4S> clipV;
	static std::vector<Maths::Vec3S> normalWorld;
	clipV.resize(vcount);
	normalWorld.resize(vcount);
	for (int i = 0; i < vcount; i++) {
		clipV[i] = mvp * FromPoint(mesh.verts[i]);
		normalWorld[i] = model * vertex_normals[i];
	}

	tris.clear();

	for (int fidx = 0; fidx < (int)mesh.faces.size() / 3; fidx++) {
		int a = mesh.faces[fidx * 3 + 0];
		int b = mesh.faces[fidx * 3 + 1];
		int c = mesh.faces[fidx * 3 + 2];

		int ua = mesh.texFaces[fidx * 3 + 0];
		int ub = mesh.texFaces[fidx * 3 + 1];
		int uc = mesh.texFaces[fidx * 3 + 2];

		ClipVertex v0, v1, v2;
		v0.clip = clipV[a]; v0.normal = normalWorld[a]; v0.uv = (ua >= 0) ? mesh.textures[ua] : Maths::Vec2S(0, 0);
		v1.clip = clipV[b]; v1.normal = normalWorld[b]; v1.uv = (ub >= 0) ? mesh.textures[ub] : Maths::Vec2S(0, 0);
		v2.clip = clipV[c]; v2.normal = normalWorld[c]; v2.uv = (uc >= 0) ? mesh.textures[uc] : Maths::Vec2S(0, 0);

		ClipVertex out[6];
		int nTris;
		ClipTriangleNear(v0, v1, v2, out, &nTris);

		for (int t = 0; t < nTris; t++)
			projectAndQueue(&out[t * 3]);
	}
	RenderAllTiles();
}

int main()
{
	if (!glfwInit()) {
		std::cout << "Échec de glfwInit\n";
		return -1;
	}

	Texture texture;
	if (!texture.load("assets/spot_texture.png"))
		return 1;

	Mesh mesh;
	if (!MeshLoader::loadMesh("assets/spot.obj", mesh))
		return 1;

	Maths::Vec3S* face_normals = (Maths::Vec3S*)malloc(mesh.faces.size() * sizeof(Maths::Vec3S));
	Maths::Vec3S* vertex_normals = (Maths::Vec3S*)malloc(mesh.verts.size() * sizeof(Maths::Vec3S));
	compute_face_normals(&mesh, face_normals);
	compute_vertex_normals(&mesh, face_normals, vertex_normals);

	std::cout << "loaded: " << mesh.verts.size() << " vertices, "
		<< mesh.faces.size() / 3 << " triangles\n";

	const uint64_t freq = glfwGetTimerFrequency();
	const uint64_t start = glfwGetTimerValue();

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "hello", NULL, NULL);

	const float aspect = (float)WIDTH / (float)HEIGHT;
	const Maths::Matrix4x4SIMD projection = Maths::Matrix4x4SIMD::Perspective(Maths::Matrix4x4SIMD::radians(60.0f), aspect, 0.1f, 100.0f);

	const Maths::Vec3S eye{ 0, 0, 3 };
	const Maths::Vec3S target{ 0, 0, 0 };
	const Maths::Vec3S up{ 0, 1, 0 };
	const Maths::Matrix4x4SIMD view = Maths::Matrix4x4SIMD::LookAt(eye, target, up);

	glfwMakeContextCurrent(window);

	BuildTiles();
	StartThreadPool();

	float lastTime = glfwGetTime();
	int frameCount = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		double seconds = (double)(glfwGetTimerValue() - start) / (double)freq;
		float angle = (float)seconds;

		Maths::Matrix4x4SIMD model = Maths::Matrix4x4SIMD::RotationY(angle * 0.7f);
		Maths::Matrix4x4SIMD mvp = projection * view * model;

		drawMeshTextured(mesh, mvp, model, texture, face_normals, vertex_normals);

		glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
		glfwSwapBuffers(window);

		float currentTime = glfwGetTime();
		frameCount++;

		if (currentTime - lastTime >= 1) {
			double fps = frameCount / (currentTime - lastTime);

			char title[256];
			snprintf(title, sizeof(title), "FPS Counter [%.1f FPS]", fps);
			glfwSetWindowTitle(window, title);

			frameCount = 0;
			lastTime = currentTime;
		}
	}

	StopThreadPool();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
