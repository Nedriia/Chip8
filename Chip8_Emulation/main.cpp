#include "Chip8.h"
#include <windows.h>
#include "Display.h"

static Chip8 m_oChip8;

#define REFRESH_RATE 60.0f
static double lastTime = 0.0f;

int Quit()
{
	Display::GetInstance()->DestroyWindow();
	return -1;
}

int main( int argc, char* argv[] )
{
	if( Display::GetInstance()->Init() == -1 )
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
		
		m_oChip8.EmulateCycle( true );
		Display::GetInstance()->Update( quit, bRefresh );

		if( bRefresh )
			lastTime = glfwGetTime();
	}
	
	Quit();

	return 0;
}