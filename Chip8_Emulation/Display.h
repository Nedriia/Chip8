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

	void Update( const std::chrono::steady_clock::time_point& time, bool cpuPaused );

	const unsigned int& GetFBOTexture() const { return m_iFBOTexture; }
	const unsigned int& GetTexture() const { return m_iTexture; }
	const unsigned int& GetFBO() const { return m_iFBO; }
	const uint8_t* GetPixels() const{ return m_pPixels; }
	GLFWwindow* GetWindow() const { return m_pWindow; }
	const std::chrono::steady_clock::time_point& GetLastTimeUpdate() const { return m_iLastTimeUpdate; }

	static const uint8_t CHIP8_DISPLAY_WIDTH;
	static const uint8_t CHIP8_DISPLAY_HEIGHT;

	static Display* GetInstance()
	{
		if (m_pSingleton == nullptr)
			m_pSingleton = new Display;
		return m_pSingleton;
	}

private:
	Display();
	~Display();
	static Display* m_pSingleton;

	int _CreateWindowChip();
	void _InitTexture();
	void _InitFramebuffer();
	void _InitRenderer();
	void _DestroyRenderer();
	static void _InitPixelsData();
	static void _XORedPixelsData( int xPos, int yPos, uint8_t oData );
	static bool _IsPixelErase( int xPos, int yPos );

	static void framebuffer_size_callback( GLFWwindow* m_pWindow, int width, int height );

	GLFWwindow* m_pWindow;
	Shader m_sShaderProgram;

	static unsigned int m_iFBOTexture;	
	unsigned int m_iTexture;	
	unsigned int m_iVAO;
	unsigned int m_iVBO;
	unsigned int m_iEBO;
	unsigned int m_iFBO;

	static uint8_t* m_pPixels;

	std::chrono::steady_clock::time_point m_iLastTimeUpdate;

	static bool		m_bDirtyFrame;
};
