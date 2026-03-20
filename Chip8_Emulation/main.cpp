#include "Chip8.h"
#include <windows.h>
#include "Display.h"
#include <chrono>

static Chip8 m_oChip8;

int Quit()
{
	Display::GetInstance()->DestroyWindow();
	return -1;
}

int main( int argc,char* argv[] )
{
	const char* sROMToLoad = nullptr;
	if( argc >= 1 )
		sROMToLoad = argv[ 1 ];

	if( sROMToLoad == nullptr )
		Quit();

	Chip8::KeyAccess oKey;
	m_oChip8.Init( oKey,sROMToLoad );

	if( Display::GetInstance()->Init( &m_oChip8 ) == -1 )
		return Quit();

	//Loop
	bool quit = false;

	while( !quit )
	{
		std::chrono::steady_clock::time_point time = std::chrono::high_resolution_clock::now();

		m_oChip8.EmulateCycle( oKey,time );
		Display::GetInstance()->Update( time,m_oChip8.IsPause(),quit );
	}

	Quit();

	return 0;
}