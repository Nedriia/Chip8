#include "Chip8.h"
#include <windows.h>
#include "Display.h"

static Chip8 m_oChip8;
static Display m_oDisplay;

#define REFRESH_RATE 60.0f
static double lastTime = 0.0f;

int Quit()
{
	m_oDisplay.DestroyWindow();
	return -1;
}

int main( int argc, char* argv[] )
{
	if( m_oDisplay.Init() == -1 )
		return Quit();

	const char* sROMToLoad = nullptr;
	if( argc >= 1 )
		sROMToLoad = argv[1];

	if( sROMToLoad == nullptr )
		Quit();

	m_oChip8.Init( sROMToLoad );

	//Loop
	bool quit = false;
	while( !quit )
	{
		double now = glfwGetTime();
		bool bRefresh = ( ( now - lastTime ) > ( 1.0f / REFRESH_RATE ) );
		
		m_oChip8.EmulateCycle( bRefresh );
		m_oDisplay.Update( quit, bRefresh );

		if( bRefresh )
			lastTime = glfwGetTime();
	}
	
	Quit();

	return 0;
}