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
#include "Texture.h"

#define WIDTH  800
#define HEIGHT 600
#define PI 3.14159265358979323846f

#define CULL_BACKFACES true
#define USE_ZBUFFER true
#define PERSPECTIVE_CORRECT true
#define AMBIENT 0.18f
#define SPEC_STRENGTH  0.55f   // 0 = no highlight, 1 = as bright as the light


static uint32_t framebuffer[WIDTH * HEIGHT];
static float    zbuffer[WIDTH * HEIGHT];

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)0xFFu << 24) | ((uint32_t)b << 16)
		| ((uint32_t)g << 8) | (uint32_t)r;
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

static inline int clampi(int v, int lo, int hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}



// Promote a 3D point to a 4D homogeneous point (w = 1).
static inline Maths::Vec4S FromPoint(Maths::Vec3S p) {
	Maths::Vec4S v = { p.x, p.y, p.z, 1.0f };
	return v;
} 

// Scale an ARGB texel by a brightness in [0, 1], per channel.
static inline uint32_t color_scale(uint32_t c, float k) {
	if (k < 0.0f) k = 0.0f;
	if (k > 1.0f) k = 1.0f;
	uint32_t r = (uint32_t)((float)((c >> 16) & 0xFF) * k);
	uint32_t g = (uint32_t)((float)((c >> 8) & 0xFF) * k);
	uint32_t b = (uint32_t)((float)(c & 0xFF) * k);
	return ((uint32_t)0xFFu << 24) | (r << 16) | (g << 8) | b;
}

// Per-triangle surface normal in MODEL space. Computed once at load:
// the cross product of two edges is perpendicular to the triangle,
// and we normalise it to unit length. The winding (chapter 8) makes
// this point OUT of the front face.
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

// Textured + lit triangle: perspective-correct UV (chapter 9) plus a
// single flat brightness multiplying every sampled texel.
static void draw_triangle_lit(int x0, int y0, float z0, float iw0, Maths::Vec2S uv0,
	int x1, int y1, float z1, float iw1, Maths::Vec2S uv1,
	int x2, int y2, float z2, float iw2, Maths::Vec2S uv2,
	const Texture* tex, float brightness) {
	int area = edge(x0, y0, x1, y1, x2, y2);
	if (area == 0) return;

	if (area < 0) {
		int tx = x1, ty = y1; float tz = z1, tiw = iw1; Maths::Vec2S tuv = uv1;
		x1 = x2; y1 = y2; z1 = z2; iw1 = iw2; uv1 = uv2;
		x2 = tx; y2 = ty; z2 = tz; iw2 = tiw; uv2 = tuv;
		area = -area;
	}

	int minx = clampi(x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2), 0, WIDTH - 1);
	int miny = clampi(y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), 0, HEIGHT - 1);
	int maxx = clampi(x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2), 0, WIDTH - 1);
	int maxy = clampi(y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2), 0, HEIGHT - 1);

	float inv_area = 1.0f / (float)area;
	float u0w = uv0.x * iw0, v0w = uv0.y * iw0;
	float u1w = uv1.x * iw1, v1w = uv1.y * iw1;
	float u2w = uv2.x * iw2, v2w = uv2.y * iw2;

	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			int e0 = edge(x1, y1, x2, y2, x, y);
			int e1 = edge(x2, y2, x0, y0, x, y);
			int e2 = edge(x0, y0, x1, y1, x, y);
			if ((e0 | e1 | e2) < 0) continue;

			float l0 = e0 * inv_area;
			float l1 = e1 * inv_area;
			float l2 = e2 * inv_area;

			float z = l0 * z0 + l1 * z1 + l2 * z2;
			int idx = y * WIDTH + x;
			if (z >= zbuffer[idx]) continue;

			float uw = l0 * u0w + l1 * u1w + l2 * u2w;
			float vw = l0 * v0w + l1 * v1w + l2 * v2w;
			float iw = l0 * iw0 + l1 * iw1 + l2 * iw2;
			uint32_t texel = tex->sample(uw / iw, vw / iw);

			zbuffer[idx] = z;
			framebuffer[idx] = color_scale(texel, brightness);
		}
	}
}

// Combine a texel with diffuse shading and a white specular add.
// diffuse_k multiplies the texel; spec_k adds white·spec_k. Clamped.
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

// Phong-shaded textured triangle: per-vertex world-space normals
// interpolated perspective-correctly, re-normalised per pixel, then
// Lambert diffuse + Blinn-Phong specular evaluated per pixel.
static void draw_triangle_phong(
	int x0, int y0, float z0, float iw0, Maths::Vec2S uv0, Maths::Vec3S n0,
	int x1, int y1, float z1, float iw1, Maths::Vec2S uv1, Maths::Vec3S n1,
	int x2, int y2, float z2, float iw2, Maths::Vec2S uv2, Maths::Vec3S n2,
	const Texture* tex, Maths::Vec3S L, Maths::Vec3S H) {
	int area = edge(x0, y0, x1, y1, x2, y2);
	if (area == 0) return;

	if (area < 0) {
		int   tx = x1, ty = y1; float tz = z1, tiw = iw1; Maths::Vec2S tuv = uv1; Maths::Vec3S	 tn = n1;
		x1 = x2; y1 = y2; z1 = z2; iw1 = iw2; uv1 = uv2; n1 = n2;
		x2 = tx; y2 = ty; z2 = tz; iw2 = tiw; uv2 = tuv; n2 = tn;
		area = -area;
	}

	int minx = clampi(x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2), 0, WIDTH - 1);
	int miny = clampi(y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), 0, HEIGHT - 1);
	int maxx = clampi(x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2), 0, WIDTH - 1);
	int maxy = clampi(y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2), 0, HEIGHT - 1);

	float inv_area = 1.0f / (float)area;

	// Pre-divide every perspective-interpolated attribute by w.
	float u0w = uv0.x * iw0, v0w = uv0.y * iw0;
	float u1w = uv1.x * iw1, v1w = uv1.y * iw1;
	float u2w = uv2.x * iw2, v2w = uv2.y * iw2;
	Maths::Vec3S n0w = n0 * iw0;
	Maths::Vec3S n1w = n1 * iw1;
	Maths::Vec3S n2w = n2 * iw2;

	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			int e0 = edge(x1, y1, x2, y2, x, y);
			int e1 = edge(x2, y2, x0, y0, x, y);
			int e2 = edge(x0, y0, x1, y1, x, y);
			if ((e0 | e1 | e2) < 0) continue;

			float l0 = e0 * inv_area;
			float l1 = e1 * inv_area;
			float l2 = e2 * inv_area;

			float z = l0 * z0 + l1 * z1 + l2 * z2;
			int idx = y * WIDTH + x;
			if (z >= zbuffer[idx]) continue;

			float iw = l0 * iw0 + l1 * iw1 + l2 * iw2;
			float inv_iw = 1.0f / iw;

			float u = (l0 * u0w + l1 * u1w + l2 * u2w) * inv_iw;
			float v = (l0 * v0w + l1 * v1w + l2 * v2w) * inv_iw;

			// Perspective-correct interpolated normal, then re-normalise:
			// averaging unit vectors gives a non-unit result.
			Maths::Vec3S N = Maths::Vec3S((l0 * n0w.x + l1 * n1w.x + l2 * n2w.x) * inv_iw,
				(l0 * n0w.y + l1 * n1w.y + l2 * n2w.y) * inv_iw,
				(l0 * n0w.z + l1 * n1w.z + l2 * n2w.z) * inv_iw);
			N = N.Normalize();

			float ndotl = N.Dot(L);
			if (ndotl < 0.0f) ndotl = 0.0f;
			float diffuse_k = AMBIENT + (1.0f - AMBIENT) * ndotl;

			float ndoth = N.Dot(H);
			if (ndoth < 0.0f) ndoth = 0.0f;
			float spec_k = std::powf(ndoth, 32) * SPEC_STRENGTH;

			uint32_t texel = tex->sample(u, v);
			zbuffer[idx] = z;
			framebuffer[idx] = color_phong(texel, diffuse_k, spec_k);
		}
	}
}


static void drawMeshTextured(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp, Maths::Matrix4x4SIMD model,  const Texture& tex, Maths::Vec3S* face_normals)
{
	const int vcount = (int)mesh.verts.size();
	const int tcount = (int)mesh.textures.size();
	std::vector<int>   screen_x(vcount), screen_y(vcount);
	std::vector<float> ndc_z(vcount), inv_w(vcount);
	std::vector<char>  visible(vcount);

	const Maths::Vec3S light = (Maths::Vec3S(0.4f, 0.8f, 0.45f)).Normalize();

	const Maths::Vec3S view_dir = Maths::Vec3S(0, 0, 1);
	const Maths::Vec3S half_vec = (light, view_dir).Normalize();
	Maths::Vec3S* normal_world = (Maths::Vec3S*)malloc(mesh.verts.size() * sizeof(Maths::Vec3S));

	for (int i = 0; i < vcount; i++) {
		Maths::Vec4S clip = mvp * FromPoint(mesh.verts[i]);

		if (clip.w <= 0.0001f) { visible[i] = false; continue; }

		float inv = 1.0f / clip.w;
		float ndc_x = clip.x * inv;
		float ndc_y = clip.y * inv;

		screen_x[i] = (int)((ndc_x + 1.0f) * 0.5f * WIDTH);
		screen_y[i] = (int)((ndc_y + 1.0f) * 0.5f * HEIGHT);
		ndc_z[i] = clip.z * inv;
		inv_w[i] = inv;
		visible[i] = true;
	}

	for (int fidx = 0; fidx < mesh.faces.size() / 3; fidx++) {
		int a = mesh.faces[fidx * 3 + 0];
		int b = mesh.faces[fidx * 3 + 1];
		int c = mesh.faces[fidx * 3 + 2];
		if (!visible[a] || !visible[b] || !visible[c]) continue;

		if (edge(screen_x[a], screen_y[a], screen_x[b], screen_y[b], screen_x[c], screen_y[c]) <= 0) continue;  // cull

		// Lambert shading for this face. Rotate the normal into
		// world space, then brightness = ambient + diffuse, where
		// diffuse = max(0, N·L). The model matrix here is a pure
		// rotation, so the normal stays unit length.
		Maths::Vec3S n = model * face_normals[fidx];
		float ndotl = n.Dot(light);
		if (ndotl < 0.0f) ndotl = 0.0f;
		float brightness = AMBIENT + (1.0f - AMBIENT) * ndotl;

		int ua = mesh.texFaces[fidx * 3 + 0];
		int ub = mesh.texFaces[fidx * 3 + 1];
		int uc = mesh.texFaces[fidx * 3 + 2];
		Maths::Vec2S uva = (ua >= 0) ? mesh.textures[ua] : Maths::Vec2S(0, 0);
		Maths::Vec2S uvb = (ub >= 0) ? mesh.textures[ub] : Maths::Vec2S(0, 0);
		Maths::Vec2S uvc = (uc >= 0) ? mesh.textures[uc] : Maths::Vec2S(0, 0);

		draw_triangle_phong(screen_x[a], screen_y[a], ndc_z[a], inv_w[a], uva, normal_world[a],
			screen_x[b], screen_y[b], ndc_z[b], inv_w[b], uvb, normal_world[b],
			screen_x[c], screen_y[c], ndc_z[c], inv_w[c], uvc, normal_world[c],
			&tex, light, half_vec);


	}
}


// One model-space normal per VERTEX: sum the normals of every
// triangle touching the vertex, then normalise. Unweighted average —
// good for a cleanly-modelled closed mesh like Spot. This is what
// makes the shading smooth: neighbouring triangles share vertex
// normals, so the interpolated normal flows continuously across edges.
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

	// One model-space normal per triangle, computed once.
	Maths::Vec3S* face_normals = (Maths::Vec3S*)malloc(mesh.faces.size() * sizeof(Maths::Vec3S));
	Maths::Vec3S* vertex_normals = (Maths::Vec3S*)malloc(mesh.verts.size() * sizeof(Maths::Vec3S));
	compute_face_normals(&mesh, face_normals);
	compute_vertex_normals(&mesh, face_normals, vertex_normals);


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
	const Maths::Vec3S eye{ 0, 0, 3 };
	const Maths::Vec3S target{ 0, 0, 0 };
	const Maths::Vec3S up{ 0, 1, 0 };
	const Maths::Matrix4x4SIMD view = Maths::Matrix4x4SIMD::LookAt(eye, target, up);

	// Directional light, fixed in WORLD space. L points FROM the
	// surface TOWARD the light — here, up and a bit to the front-right,
	// like a sun over your shoulder.



	glfwMakeContextCurrent(window);
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();


		
		double seconds = (double)(glfwGetTimerValue() - start) / (double)freq;		//tick / frequence  = temps en seconde					
		float angle = (float)seconds;   // 1 radian par seconde




		Maths::Matrix4x4SIMD model = Maths::Matrix4x4SIMD::RotationY(angle * 0.7f);
		Maths::Matrix4x4SIMD mvp = projection * view * model;



		clear(rgb(16, 16, 24));
		clear_zbuffer();

		

		drawMeshTextured(mesh, mvp, model, texture, face_normals);


		glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;

}

