#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"

class Display
{

public:
	Display();
	int Init();
	void Update( bool& quit, bool bRefreshFrame );
	void DestroyWindow();
	static void ClearScreen();
	static void DrawPixelAtPos( uint8_t xPos, uint8_t yPos, uint8_t oValue, bool& bErased );
	void RenderInTexture();
	const unsigned int& GetTexture() const { return texture; }

private:
	int _CreateWindowChip();
	void _InitTexture();
	void _InitFramebuffer();
	void _InitRenderer();
	void _DestroyRenderer();

	static void framebuffer_size_callback( GLFWwindow* window, int width, int height );
	void processInput( GLFWwindow* window );
	void UpdateTexture();

	GLFWwindow* window;
	Shader shaderProgram;

	unsigned int texture;	
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;
	unsigned int FBO;
};
