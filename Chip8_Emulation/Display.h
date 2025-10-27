#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"

class Display
{

public:
	int Init();
	void Update( bool& quit, bool bRefreshFrame );
	void DestroyWindow();
	static void ClearScreen();
	static void DrawPixelAtPos( uint8_t xPos, uint8_t yPos, uint8_t oValue, bool& bErased );
	const unsigned int& GetTexture() const { return texture; }
	const unsigned int& GetFBO() const { return FBO; }
	const uint8_t* GetPixels() const{ return pixels; }

	static const uint8_t CHIP8_DISPLAY_WIDTH;
	static const uint8_t CHIP8_DISPLAY_HEIGHT;

	static Display* GetInstance()
	{
		if (singleton == nullptr)
			singleton = new Display;
		return singleton;
	}

private:
	Display();
	~Display();
	static Display* singleton;

	int _CreateWindowChip();
	void _InitTexture();
	void _InitFramebuffer();
	void _InitRenderer();
	void _DestroyRenderer();
	static void _InitPixelsData();
	static void _XORedPixelsData( int xPos, int yPos, uint8_t odata );
	static bool _IsPixelErase( int xPos, int yPos );

	static void framebuffer_size_callback( GLFWwindow* window, int width, int height );
	void processInput( GLFWwindow* window );

	GLFWwindow* window;
	Shader shaderProgram;

	unsigned int texture;	
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;
	unsigned int FBO;

	static uint8_t* pixels;
};
