#pragma once
#include <iostream>
#include <vector>
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



class Renderer {

private:

	//
	static constexpr int defaultWidth = 800;
	static constexpr int defaultHeight = 600;
	static constexpr int SizeWxH = defaultWidth * defaultHeight;
	Color defaultBcColor;
	static Renderer* r_instance;
	GLFWwindow* window = nullptr;

	//


	//
	static int edge(Maths::Vec2S v1, Maths::Vec2S v2, Maths::Vec2S P);
	static void clearColor(uint32_t color);
	static void clearZbuffer();
	static inline int clampi(int v, int lo, int hi);
	static void compute_face_normals(const Mesh* mesh, Maths::Vec3S* out);
	static void compute_vertex_normals(const Mesh* mesh, const Maths::Vec3S* face_normals, Maths::Vec3S* out);

protected:



	static void drawMeshTextured(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp, Maths::Matrix4x4SIMD model, const Material& mat, Maths::Vec3S* face_normals, Maths::Vec3S* vertex_normals);
	static void projectAndDraw(const ClipVertex* tri, const Material* mat, Maths::Vec3S L, Maths::Vec3S H);
	void drawPx(int sizeX, int sizeY);
	static inline void put_pixel(int x, int y, Color color);
	static void draw_line(Maths::Vec2S x , Maths::Vec2S y, uint32_t color);
	void clearAllBuffer(); 

	static void draw_triangle_phong(
		Maths::Vec3S v0, float iw0, Maths::Vec2S uv0, Maths::Vec3S n0,
		Maths::Vec3S v1, float iw1, Maths::Vec2S uv1, Maths::Vec3S n1,
		Maths::Vec3S v2, float iw2, Maths::Vec2S uv2, Maths::Vec3S n2,
		const Material* mat, Maths::Vec3S L, Maths::Vec3S H);


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