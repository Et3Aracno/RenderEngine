#include <iostream>
#include "External Lib/glfw-3.5.1.bin.WIN64/include/GLFW/glfw3.h"




int main()
{
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
		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;

}

