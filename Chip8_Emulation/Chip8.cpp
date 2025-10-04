#include "Chip8.h"
#include <iostream>
#include <fstream>

#define START_FONT_MEMORY_ADDRESS 0x050
#define START_ROM_MEMORY_ADDRESS 0x200
#define MEMORY_SIZE 4096
#define DEFAULT_PARENT_ROM_FOLDER "..\\"

Chip8::Chip8() :
	I( 0 ),
	SP( 0 ),
	PC( 0 )
{}

void Chip8::Init( const char* sROMToLoad)
{
	LoadFont();
	LoadROM( sROMToLoad );

	PC = START_ROM_MEMORY_ADDRESS;
	SP = 0;
}

void Chip8::LoadFont()
{
	for( uint8_t i = 0; i < 80; ++i )
	{
		memory[ START_FONT_MEMORY_ADDRESS + i ] = fontset[i];
	}
}

void Chip8::LoadROM( const char* sROMToLoad )
{
	std::string sPath = DEFAULT_PARENT_ROM_FOLDER;
	std::ifstream file( sPath + sROMToLoad, std::ios::binary | std::ios::in | std::ios::ate );

	if( file.is_open() )
	{
		std::streamsize size = file.tellg();
		if( size <= 0 || size > MEMORY_SIZE - START_ROM_MEMORY_ADDRESS )
		{
			throw std::runtime_error( "Size ROM invalid" );
			return;
		}

		char* memblock = new char [ size ];
		file.seekg( 0, std::ios::beg );
		file.read( memblock, size );
		file.close();

		std::streamsize bytesRead = file.gcount();
		if( bytesRead != size )
		{
			throw std::runtime_error( "Size read not conform" );
			delete[] memblock;
			return;
		}

		for( uint16_t i = 0; i < bytesRead; ++i )
		{
			memory[START_ROM_MEMORY_ADDRESS + i] = static_cast<uint8_t>( memblock[i] );
		}

		delete[] memblock;
	}
}