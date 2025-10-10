#include "Chip8.h"
#include <windows.h>
#include "Display.h"

Chip8 m_oChip8;
Display m_oDisplay;

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
		m_oDisplay.Update( quit );
		m_oChip8.EmulateCycle();
	}

	return 0;
}