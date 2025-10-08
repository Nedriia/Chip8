#include "Chip8.h"
#include <windows.h>

Chip8 m_oChip8;

int main( int argc, char* argv[] )
{
	const char* sROMToLoad = nullptr;
	if( argc >= 1 )
		sROMToLoad = argv[1];

	if( sROMToLoad == nullptr )
		return -1;

	m_oChip8.Init( sROMToLoad );

	//Loop
	uint8_t state = 0;
	while( state == 0 )
	{
		m_oChip8.EmulateCycle();
		if(GetAsyncKeyState(VK_ESCAPE))
		{
			state = 1;
		}
	}

	return 0;
}