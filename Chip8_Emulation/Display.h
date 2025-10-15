#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"

class Display
{

public:
	Display();
	int Init();
	void Update( bool& quit );
	void DestroyWindow();

private:
	int _CreateWindowChip();
	void _InitTexture();
	void _InitRenderer();
	void _DestroyRenderer();

	static void framebuffer_size_callback( GLFWwindow* window, int width, int height );
	void processInput( GLFWwindow* window );

	GLFWwindow* window;
	Shader shaderProgram;

	unsigned int texture;	
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;
};
