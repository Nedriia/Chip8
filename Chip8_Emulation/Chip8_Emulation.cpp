#include "Chip8.h"

Chip8 m_oChip8;

int main( int argc, char* argv[] )
{
	const char* sROMToLoad = nullptr;
	if( argc >= 1 )
		sROMToLoad = argv[1];

	if( sROMToLoad != nullptr )
		m_oChip8.Init( sROMToLoad );

	return 0;
}