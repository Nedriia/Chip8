#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include <chrono>

#define REFRESH_TICK_MicroSec 16666
static std::chrono::microseconds refreshTick( REFRESH_TICK_MicroSec );

class Chip8;

class Display
{

public:
	class KeyDisplayAccess
	{
		friend int main( int argc,char** argv );
		friend int Quit();
		friend class Chip8;
		friend class Chip8_Debugger;
		KeyDisplayAccess(){}
	};

	int Init( const KeyDisplayAccess& oKey, const Chip8* pCpu );
	static void ClearScreen( const KeyDisplayAccess& oKey );
	static void DrawPixelAtPos( const KeyDisplayAccess& oKey, uint8_t xPos,uint8_t yPos,uint8_t oValue,bool& bErased );
	void DestroyWindow( const KeyDisplayAccess& oKey );

	void Update( const std::chrono::steady_clock::time_point& time, bool cpuPaused, bool& quit );

	const unsigned int& GetTexture() const { return texture; }
	const unsigned int& GetFBO() const { return FBO; }
	const uint8_t* GetPixels() const{ return pixels; }
	const GLFWwindow* GetWindow() const { return window; }
	const std::chrono::steady_clock::time_point& GetLastTimeUpdate() const { return lastTimeUpdate; }

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

	std::chrono::steady_clock::time_point lastTimeUpdate;
};
