#include <iostream>
#include <vector>
#include <cstdint>
#include "External Lib/glfw-3.5.1.bin.WIN64/include/GLFW/glfw3.h"



void drawPixel(GLFWwindow* window, int sizeX, int sizeY)
{
	std::vector<uint32_t> data(sizeX * sizeY);

	for (int y = 0; y < sizeY; ++y)
		for (int x = 0; x < sizeX; ++x)
			data[y * sizeX + x] = 0xFF000000
			| (uint32_t)(x * 255 / sizeX) << 16
			| (uint32_t)(y * 255 / sizeY) << 8;

	glDrawPixels(sizeX, sizeY, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
}


int main()
{

	int data[250][250];
	for (size_t y = 0; y < 250; ++y)
	{
		for (size_t x = 0; x < 250; ++x)
		{
			data[y][x] = 0xFF000000 | (x * 255 / 250) << 16 | (y * 255 / 250) << 8;// les operation binaire change les couleur
		}
	}



	if (!glfwInit()) {
		std::cout << "Échec de glfwInit\n";
		return -1;
	}



	GLFWwindow* window = glfwCreateWindow(600, 800, "hello", NULL, NULL);

	if (!window) {

		std::cout << "hello";
	}

	glfwMakeContextCurrent(window);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		//glDrawPixels(250, 250, GL_RGBA, GL_UNSIGNED_BYTE, data);
		drawPixel(window, 500, 500);
		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;

}

