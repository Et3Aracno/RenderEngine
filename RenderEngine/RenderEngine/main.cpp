#include <iostream>
#include <vector>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits>
#include "GLFW/glfw3.h"
#include "External Lib/MathLibrary/MathLib.h"
#include "MeshRLoader.h"

#define WIDTH  800
#define HEIGHT 600
#define PI 3.14159265358979323846f

#define CULL_BACKFACES true
#define USE_ZBUFFER true


static uint32_t framebuffer[WIDTH * HEIGHT];
static float    zbuffer[WIDTH * HEIGHT];

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) { // traduction rapide binaire rgb pour les couleur
	return ((uint32_t)0xFFu << 24) | ((uint32_t)r << 16)
		| ((uint32_t)g << 8) | (uint32_t)b;
}

static void clear(uint32_t color) {
	for (int i = 0; i < WIDTH * HEIGHT; i++) framebuffer[i] = color;
}

static void clear_zbuffer(void) {
	for (int i = 0; i < WIDTH * HEIGHT; i++) zbuffer[i] = std::numeric_limits<float>::max();
}

static inline void put_pixel(int x, int y, uint32_t color) { // ajoute 1 pixel 
	if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
	framebuffer[y * WIDTH + x] = color;
}

void drawPixels( int sizeX, int sizeY) // ajoute des pixel d'une coordonné x à une coodooné y puis draw le tout 
{
	for (int y = 0; y < HEIGHT; ++y)
		for (int x = 0; x < WIDTH; ++x)
			framebuffer[y * WIDTH + x] = 0xFF000000 | (uint32_t)(x * 255 / WIDTH) << 16 | (uint32_t)(y * 255 / HEIGHT) << 8;

	glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
}

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color) // draw une ligne x1 et y1 doivent etre plus grand que x0 et y0 et ce sont eux qui modifie lal size de la ligne
{

	//int dx = x1 - x0; // delta de x
	//int dy = y1 - y0; // delta de y
	//int E = 2 * dy - dx; // calcul de E par rapport à delta de x et y 
	//int y = y0;
	//for (int x = x0; x < x1; x++) 
	//{
	//	put_pixel(x, y,rgb(0,0,250));
	//	if (E > 0) 
	//	{
	//		y = y + 1;
	//		E = E + 2 * (dy - dx);
	//	}
	//	else 
	//	{
	//		E = E + 2 * dy;
	//	}
	//}

	int dx = abs(x1 - x0);
	int dy = -abs(y1 - y0);          // dy is NEGATIVE on purpose, see below
	int sx = (x0 < x1) ? 1 : -1;     // step direction in x
	int sy = (y0 < y1) ? 1 : -1;     // step direction in y
	int err = dx + dy;               // = dx - |dy|

	while (true) {
		put_pixel(x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }   // step in x?
		if (e2 <= dx) { err += dx; y0 += sy; }   // step in y?
	}
}

inline int edge(int x0, int y0, int x1 ,int y1, int Px, int Py) 
{
	return (x1 - x0) * (Py - y0) - (y1 - y0) * (Px - x0);

}

void drawTriangle(int x0, int y0, int x1, int y1, int Px, int Py, uint32_t color) {

	int area = edge(x0, y0, x1, y1, Px, Py);

	if (area == 0)
		return;

	int minx = x0 < x1 ? (x0 < Px ? x0 : Px) : (x1 < Px ? x1 : Px);
	int miny = y0 < y1 ? (y0 < Py ? y0 : Py) : (y1 < Py ? y1 : Py);
	int maxx = x0 > x1 ? (x0 > Px ? x0 : Px) : (x1 > Px ? x1 : Px);
	int maxy = y0 > y1 ? (y0 > Py ? y0 : Py) : (y1 > Py ? y1 : Py);
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (maxx >= WIDTH)  maxx = WIDTH - 1;
	if (maxy >= HEIGHT) maxy = HEIGHT - 1;

	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			int e0 = edge(x0, y0, x1, y1, x, y);
			int e1 = edge(x1, y1, Px, Py, x, y);
			int e2 = edge(Px, Py, x0, y0, x, y);

			if ((e0 | e1 | e2) >= 0) {// Comparateur binaire qui signifie que si l'un des 3 possède un bit à 1 donc négatif , if = false en revanche true si les bit des signe sont à 0 alors les pixel sont interne au triangle.
				framebuffer[y * WIDTH + x] = color;
			}

		}


	}

}


static inline int clampi(int v, int lo, int hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}



void drawTriangle_z(int x0, int y0, float z0, int x1, int y1, float z1, int x2, int y2, float z2, float zmin, float zrange) 
{
	int area = edge(x0, y0, x1, y1, x2, y2);

	if (area == 0)
		return;
	// Force one winding so "inside" means "all weights >= 0". We
	// accept BOTH facing directions here (back faces are removed in
	// chapter 8); the Z-buffer is what sorts them out for now, so we
	// must be able to fill a triangle of either winding. Swapping
	// vertices 1 and 2 flips a negative area positive — and the
	// depth z1/z2 must ride along with their vertices.
	if (area < 0) {
		int   tx = x1; int ty = y1; float tz = z1;
		x1 = x2; y1 = y2; z1 = z2;
		x2 = tx; y2 = ty; z2 = tz;
		area = -area;
	}

	// Bounding box, clamped to the screen.
	int minx = clampi(x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2), 0, WIDTH - 1);
	int miny = clampi(y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), 0, HEIGHT - 1);
	int maxx = clampi(x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2), 0, WIDTH - 1);
	int maxy = clampi(y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2), 0, HEIGHT - 1);

	float inv_area = 1.0f / (float)area;

	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			int e0 = edge(x1, y1, x2, y2, x, y);
			int e1 = edge(x2, y2, x0, y0, x, y);
			int e2 = edge(x0, y0, x1, y1, x, y);
			if ((e0 | e1 | e2) < 0) continue;   // outside the triangle

			// Barycentric weights, normalised to sum to 1.
			float l0 = e0 * inv_area;
			float l1 = e1 * inv_area;
			float l2 = e2 * inv_area;

			// Interpolate depth. NDC z is an AFFINE function of the
			// screen position, so plain screen-space barycentric
			// interpolation is exactly correct for it — no
			// perspective correction needed (that comes in chapter 9,
			// for textures). Smaller z = nearer the camera.
			float z = l0 * z0 + l1 * z1 + l2 * z2;

			int idx = y * WIDTH + x;
#if USE_ZBUFFER
			if (z >= zbuffer[idx]) continue;    // something nearer is already here
			zbuffer[idx] = z;
#endif
			// Shade by depth: map [zmin .. zmax] to bright .. dim.
			float far01 = (z - zmin) / zrange;          // 0 near, 1 far
			int level = 240 - (int)(far01 * 200.0f);    // 240 near .. 40 far
			level = clampi(level, 0, 255);
			framebuffer[idx] = rgb((uint8_t)level, (uint8_t)level, (uint8_t)level);
		}
	}




}

// Promote a 3D point to a 4D homogeneous point (w = 1).
static inline Maths::Vec4S FromPoint(Maths::Vec3S p) {
	Maths::Vec4S v = { p.x, p.y, p.z, 1.0f };
	return v;
}

static void drawMeshZ(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp)
{
	const int vcount = (int)mesh.verts.size();
	std::vector<int>   screen_x(vcount), screen_y(vcount);
	std::vector<float> ndc_z(vcount);
	std::vector<char>  visible(vcount);

	
	float zmin = std::numeric_limits<float>::min(),

		  zmax = std::numeric_limits<float>::max();

	for (int i = 0; i < vcount; i++) {
		Maths::Vec4S clip = mvp * FromPoint(mesh.verts[i]);

		if (clip.w <= 0.0001f) { visible[i] = false; continue; }

		float ndc_x = clip.x / clip.w;
		float ndc_y = clip.y / clip.w;
		float nz = clip.z / clip.w;

		screen_x[i] = (int)((ndc_x + 1.0f) * 0.5f * WIDTH);
		screen_y[i] = (int)(( ndc_y + 1.0f) * 0.5f * HEIGHT);
		ndc_z[i] = nz;
		visible[i] = true;

		if (nz < zmin) zmin = nz;
		if (nz > zmax) zmax = nz;
	}
	float zrange = (zmax > zmin) ? (zmax - zmin) : 1.0f;

	int drawn = 0, culled = 0;

	for (size_t f = 0; f + 2 < mesh.faces.size(); f += 3) {
		int a = mesh.faces[f + 0];
		int b = mesh.faces[f + 1];
		int c = mesh.faces[f + 2];

		if (a < 0 || b < 0 || c < 0 || a >= vcount || b >= vcount || c >= vcount) continue; // en dehors du tb 
		if (!visible[a] || !visible[b] || !visible[c]) continue; // derriere la cam

		// Backface culling. The screen-space signed area tells us
		// the triangle's winding. Because the viewport flips y,
		// a FRONT-facing triangle comes out with a NEGATIVE area;
		// a back-facing one is positive (and zero is edge-on /
		// degenerate). So we keep only area < 0.
		int area = edge(screen_x[a], screen_y[a], screen_x[b], screen_y[b], screen_x[c], screen_y[c]);

#if CULL_BACKFACES
		if (area >= 0) { culled++; continue; }
#endif
		(void)area;
		drawn++;

		drawTriangle_z(screen_x[a], screen_y[a], ndc_z[a],
			screen_x[b], screen_y[b], ndc_z[b],
			screen_x[c], screen_y[c], ndc_z[c],
			zmin, zrange);
	}
}
int main()
{

	if (!glfwInit()) {
		std::cout << "Échec de glfwInit\n";
		return -1;
	}
	
	Mesh mesh;
	if (!MeshLoader::loadMesh("assets/spot.obj", mesh))
		return 1;

	std::cout << "loaded: " << mesh.verts.size() << " vertices, "
		<< mesh.faces.size() / 3 << " triangles\n";

	// Edge colour per axis (rgb() isn't a compile-time constant, so
	// we fill this at runtime rather than as a static initializer).


	const uint64_t freq = glfwGetTimerFrequency();
	const uint64_t start = glfwGetTimerValue();

	GLFWwindow* window = glfwCreateWindow(800, 600, "hello", NULL, NULL);


	// Projection and view stay fixed; only the model spins.
	const float aspect = (float)WIDTH / (float)HEIGHT;

	// 60-degree vertical field of view (radians() converts to the
	// radians the math needs); window shape; near and far clip
	// distances. See README section 5.6.
	const Maths::Matrix4x4SIMD projection = Maths::Matrix4x4SIMD::Perspective(Maths::Matrix4x4SIMD::radians(60.0f), aspect, 0.1f, 100.0f);

	// Camera: positioned 5 units back on +z (eye), aimed at the
	// origin (target), with world-up being +y. See README section 5.10.
	const Maths::Vec3S eye{ 0, 0, 10 };
	const Maths::Vec3S target{ 0, 0, 0 };
	const Maths::Vec3S up{ 0, 1, 0 };
	const Maths::Matrix4x4SIMD view = Maths::Matrix4x4SIMD::LookAt(eye, target, up);

	glfwMakeContextCurrent(window);
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();


		
		double seconds = (double)(glfwGetTimerValue() - start) / (double)freq;		//tick / frequence  = temps en seconde					
		float angle = (float)seconds;   // 1 radian par seconde




		Maths::Matrix4x4SIMD model = Maths::Matrix4x4SIMD::RotationY(angle * 0.7f);
		Maths::Matrix4x4SIMD mvp = projection * view * model;



		clear(rgb(16, 16, 24));
		clear_zbuffer();

		drawMeshZ(mesh, mvp); //fonctions drawMesh adapter pour le Zbuffer et le remplissage des triangles


		glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;

}

