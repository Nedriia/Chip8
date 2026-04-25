#include "Init_RomSettings.h"
#include <iostream>
#include <vector>
#include <fstream>
#include "Chip8.h"
#include "TinySHA1.hpp"
#include "json.hpp"

using json = nlohmann::json;

#define HASHES_DEFAULT_FOLDER "chip-8-database\\database\\sha1-hashes.json"
#define PROGRAMS_DEFAULT_FOLDER "chip-8-database\\database\\programs.json"

static std::stringstream sHash;

void Init_RomSettings::LookForDatabaseInfos( const char* memblock,const size_t& size )
{
	//Calculate SHA1 of current ROM
	int iIndex = _CalculateHash_RetrieveIndex( memblock,size );
	_LoadProgramsSettings( iIndex );
}

int Init_RomSettings::_CalculateHash_RetrieveIndex( const char* memblock,const size_t& size )
{
	sha1::SHA1 sSha;
	sSha.processBytes( memblock,size );
	uint32_t digest[ 5 ];
	sSha.getDigest( digest );

	sHash.clear();
	for( int i = 0; i < 5; ++i )
		sHash << std::hex << std::setw( 8 ) << std::setfill( '0' ) << digest[ i ];

	return _FindIndex();
}

int Init_RomSettings::_FindIndex()
{
	std::ifstream file( HASHES_DEFAULT_FOLDER );
	if( file.is_open() )
	{
		json data = json::parse( file );
		if( data.contains( sHash.str() ) )
		{
			file.close();
			return data[ sHash.str() ];
		}
		else
		{
			std::cerr << "ERROR::DATABASE::HASH_NOT_FOUND : " << sHash.str() << std::endl;
			file.close();
		}
	}
	else
	{
		std::cerr << "ERROR::DATABASE::CANT_FIND_HASHES_FILE : " << HASHES_DEFAULT_FOLDER << std::endl;
	}
	return -1;
}

void Init_RomSettings::_LoadProgramsSettings( const int iIndex )
{
	std::ifstream file( PROGRAMS_DEFAULT_FOLDER );
	if( file.is_open() )
	{
		json data = json::parse( file );
		if( iIndex < data.size() )
		{
			for( json::iterator it = data[ iIndex ].begin(); it != data[ iIndex ].end(); ++it )
				std::cout << *it << '\n';

			Display::SetGameTitle( data[ iIndex ][ "title" ] );
			if( data[ iIndex ][ "roms" ].contains( sHash.str() ) )
			{
				Chip8::SetInstructionPerFrame( data[ iIndex ][ "roms" ][ sHash.str() ][ "tickrate" ] );
				if( data[ iIndex ][ "roms" ][ sHash.str() ][ "colors" ].contains( "pixels" ) )
					Display::GetInstance()->AssignDatabaseColors( data[ iIndex ][ "roms" ][ sHash.str() ][ "colors" ][ "pixels" ].get<std::vector<std::string>>() );
			}
			file.close();
		}
		else
		{
			std::cerr << "ERROR::DATABASE::INDEX_NOT_VALID : " << iIndex << std::endl;
			file.close();
		}
	}
	else
	{
		std::cerr << "ERROR::DATABASE::CANT_FIND_PROGRAMS_FILE : " << HASHES_DEFAULT_FOLDER << std::endl;
	}
}
