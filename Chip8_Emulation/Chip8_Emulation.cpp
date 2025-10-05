#include "Chip8.h"

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
	m_oChip8.EmulateCycle();

	return 0;
}