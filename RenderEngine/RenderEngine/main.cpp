#include <iostream>
#include <vector>
#include <cstdint>
#include "External Lib/glfw-3.5.1.bin.WIN64/include/GLFW/glfw3.h"
#define WIDTH  800
#define HEIGHT 600



static uint32_t framebuffer[WIDTH * HEIGHT];

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) { // traduction rapide binaire rgb pour les couleur
	return ((uint32_t)0xFFu << 24) | ((uint32_t)r << 16)
		| ((uint32_t)g << 8) | (uint32_t)b;
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

void drawTriangle() {


}

int main()
{

	if (!glfwInit()) {
		std::cout << "Échec de glfwInit\n";
		return -1;
	}



	GLFWwindow* window = glfwCreateWindow(800, 600, "hello", NULL, NULL);

	if (!window) {

		std::cout << "hello";
	}

	glfwMakeContextCurrent(window);
	const int cx = WIDTH / 2;
	const int cy = HEIGHT / 2;
	const int spokes = 100;
	const int radius = (HEIGHT / 2) - 20;
	const double TWO_PI = 6.2831853071795864;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		for (int i = 0; i < spokes; i++) {
			double t = (double)i / (double)spokes * TWO_PI;
			double ct = cos(t);
			double st = sin(t);
			int ex = cx + (int)(radius * ct);
			int ey = cy + (int)(radius * st);
			uint8_t r = (uint8_t)(127.0 + 127.0 * ct);
			uint8_t g = (uint8_t)(127.0 + 127.0 * st);
			draw_line(cx, cy, ex, ey, rgb(r, g, 200));
		}
		glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;

}

