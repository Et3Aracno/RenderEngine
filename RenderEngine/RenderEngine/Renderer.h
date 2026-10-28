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
#include "MeshRLoader.h"
#include "Texture.h"
#include "clip.h"
#include "Color.h"
#include "Material.h"
#include "Font.h"





class Renderer {

private:

	//
	static const int defaultWidth = 1100;
	static const int defaultHeight = 900;
	static const int SizeWxH = defaultWidth * defaultHeight;
	Color defaultBcColor;

	//
	static uint32_t framebuffer[defaultWidth * defaultHeight];
	static float    zbuffer[defaultWidth * defaultHeight];


	//
	static int edge(Maths::Vec2S v1, Maths::Vec2S v2, Maths::Vec2S P);
	static void clearColor(uint32_t color);
	static void clearZbuffer();
	static inline int clampi(int v, int lo, int hi);
	static void compute_face_normals(const Mesh* mesh, Maths::Vec3S* out);
	static void compute_vertex_normals(const Mesh* mesh, const Maths::Vec3S* face_normals, Maths::Vec3S* out);
protected:



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


	void init();
	void update();





	Color GetBcColor() const { return defaultBcColor; }
	void SetBcColor(Color color) { defaultBcColor = color; }








};