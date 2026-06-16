#include "Init_RomSettings.h"
#include <iostream>
#include "Chip8.h"
#include "TinySHA1.hpp"
#include "json.hpp"

using json = nlohmann::json;

#define HASHES_DEFAULT_FOLDER "sha1-hashes.json"
#define PROGRAMS_DEFAULT_FOLDER "programs.json"
#define PLATFORMS_DEFAULT_FOLDER "platforms.json"

static std::stringstream sHash;

void Init_RomSettings::LookForDatabaseInfos( const char* memblock,const size_t& size )
{
	//Calculate SHA1 of current ROM
	int iIndex = _CalculateHash_RetrieveIndex( memblock,size );
	if( iIndex == -1 )
	{
		Display::GetInstance()->AssignDisplaySettings( true );
		return;
	}

	bool bSuccess = _LoadProgramsSettingsIsSuccesful( iIndex );
	if( !bSuccess )
		Display::GetInstance()->AssignDisplaySettings();
}

int Init_RomSettings::_CalculateHash_RetrieveIndex( const char* memblock,const size_t& size )
{
	sha1::SHA1 sSha;
	sSha.processBytes( memblock,size );
	uint32_t digest[ 5 ];
	sSha.getDigest( digest );

	sHash.str( std::string() );
	sHash.clear();
	for ( uint32_t i : digest )
		sHash << std::hex << std::setw( 8 ) << std::setfill( '0' ) << i;

	return _FindIndex();
}

int Init_RomSettings::_FindIndex()
{
	static std::string sAbsolutePath;
	if( sAbsolutePath.empty() )
		sAbsolutePath = std::string( PATH_DATABASE ) + HASHES_DEFAULT_FOLDER;
	std::ifstream file( sAbsolutePath,std::ios::in );

	if( file.is_open() )
	{
		json data = json::parse( file );
		if( data.contains( sHash.str() ) )
		{
			file.close();
			file.clear();
			return data[ sHash.str() ];
		}
		else
		{
			std::cerr << "ERROR::DATABASE::HASH_NOT_FOUND : " << sHash.str() << std::endl;
			file.close();
			file.clear();
		}
	}
	else
	{
		std::cerr << "ERROR::DATABASE::CANT_FIND_HASHES_FILE : " << std::string( PATH_DATABASE ) + HASHES_DEFAULT_FOLDER << std::endl;
		_ReturnAdditionnalInfoOnErrors( file );
	}

	return -1;
}

bool Init_RomSettings::_LoadProgramsSettingsIsSuccesful( const int iIndex )
{
	static std::string sProgramsAbsolutePath;
	if( sProgramsAbsolutePath.empty() )
		sProgramsAbsolutePath = std::string( PATH_DATABASE ) + PROGRAMS_DEFAULT_FOLDER;

	std::ifstream file( sProgramsAbsolutePath,std::ios::in );
	if( file.is_open() )
	{
		json data = json::parse( file );
		if( iIndex < data.size() )
		{
			std::cout << std::endl;
			for( json::iterator it = data[ iIndex ].begin(); it != data[ iIndex ].end(); ++it )
				std::cout << *it << '\n';
			std::cout << std::endl;

			Display::SetGameTitle( data[ iIndex ][ "title" ] );
			if( data[ iIndex ][ "roms" ].contains( sHash.str() ) )
			{
				//Tickrate
				int iRomCustomTickrate = 0;
				auto oRom = data[ iIndex ][ "roms" ][ sHash.str() ];
				if( oRom.contains( "tickrate" ) )
					iRomCustomTickrate = oRom[ "tickrate" ];

				std::map<std::string,int> aKeys;
				if( oRom.contains( "keys" ) )
					aKeys = oRom[ "keys" ].get<std::map<std::string,int>>();
				Input::GetInstance()->InitInputFromDatabase( aKeys );

				//Palette
				std::vector<std::string > sColors;
				if( oRom[ "colors" ].contains( "pixels" ) )
					sColors = oRom[ "colors" ][ "pixels" ].get<std::vector<std::string>>();
				Display::GetInstance()->AssignDisplaySettings( false, sColors );

				//Platforms Specs
				if( oRom.contains( "platforms" ) )
					return ( _LoadPlatformsSettingsIsSuccesful( oRom[ "platforms" ].get<std::vector<std::string>>(),iRomCustomTickrate ) ); //Load specs if platform found
			}
			else
			{
				std::cout<< "WARNING::ROMS_DONT_CONTAIN_HASH::CONTINUE_EXEC" << std::endl;
			}
			file.close();
			file.clear();
		}
		else
		{
			std::cerr << "ERROR::DATABASE::INDEX_NOT_VALID : " << iIndex << std::endl;
			file.close();
			file.clear();
			return false;
		}
	}
	else
	{
		std::cerr << "ERROR::DATABASE::CANT_FIND_PROGRAMS_FILE : " << PROGRAMS_DEFAULT_FOLDER << std::endl;
		_ReturnAdditionnalInfoOnErrors( file );
		return false;
	}
	return true;
}

bool Init_RomSettings::_LoadPlatformsSettingsIsSuccesful( const std::vector<std::string >& sPlatforms,const int iRomCustomTickrate )
{
	auto sSupportPlatforms = Chip8::GetPlatformsSupported();
	ptrdiff_t iSize = sSupportPlatforms->end() - sSupportPlatforms->begin();
	for( auto it = sSupportPlatforms->end() - 1; iSize > 0; --iSize )
	{
		for( int k = sPlatforms.size() - 1; k >= 0; --k ) //Check for newer platform in prior
		{
			if( sPlatforms[ k ] == ( *it ) )
			{
				_LoadPlatformsSpecs( *it,iRomCustomTickrate );
				std::cout << "LOAD_ROM_ON_::" << ( *it ) << std::endl;
				return true;
			}
		}

		if( iSize > 1 )
			--it;
	}
	std::cerr << "ERROR::PLATFORM::NO_SUPPORT_PLATFORM_FOR_THIS_ROM" << std::endl;
	Display::GetInstance()->SetResolution( 64, 32 );
	return false;
}

void Init_RomSettings::_LoadPlatformsSpecs( const std::string& sPlatform,const int iRomCustomTickrate )
{
	static std::string sPlatformsAbsolutePath;
	if( sPlatformsAbsolutePath.empty() )
		sPlatformsAbsolutePath = std::string( PATH_DATABASE ) + PLATFORMS_DEFAULT_FOLDER;

	std::ifstream file( sPlatformsAbsolutePath,std::ios::in );
	if( file.is_open() )
	{
		json data = json::parse( file );
		for( auto oData : data )
		{
			if( oData.contains( "id" ) && oData[ "id" ] == sPlatform )
			{
				//Resolution
				if( oData.contains( "displayResolutions" ) )
				{
					std::string sRes = oData[ "displayResolutions" ][ 0 ];
					size_t xPos = sRes.find( 'x' );

					if( xPos != std::string::npos )
					{
						int iWidth = std::stoi( sRes.substr( 0,xPos ) );
						int iHeight = std::stoi( sRes.substr( xPos + 1 ) );

						Display::GetInstance()->SetResolution( iWidth,iHeight );
					}
				}
				Chip8::SetInstructionPerFrame( iRomCustomTickrate == 0 ? static_cast< int >( oData[ "defaultTickrate" ] ) : iRomCustomTickrate );
#ifndef OVERRIDE_DATABASE_QUIRKS
				if( oData.contains( "quirks" ) )
				{
					Chip8::m_oCurrentQuirk.bShiftingFlag = oData[ "quirks" ].value( "shift",false );
					Chip8::m_oCurrentQuirk.bMemoryUnchanged = oData[ "quirks" ].value( "memoryLeaveIUnchanged",false );
					Chip8::m_oCurrentQuirk.bMemoryIncrementByX = oData[ "quirks" ].value( "memoryIncrementByX",false );
					Chip8::m_oCurrentQuirk.bVFResetFlag = oData["quirks"].value( "logic",false );
					Chip8::m_oCurrentQuirk.bDispWaitFlag = oData[ "quirks" ].value( "vblank", false );
					Chip8::m_oCurrentQuirk.bWrapFlag = oData[ "quirks" ].value( "wrap",false );
					Chip8::m_oCurrentQuirk.bQuirkJumpingFlag = oData[ "quirks" ].value( "jump",false );
				}
#endif
				return;
			}
		}
	}
	else
	{
		std::cerr << "ERROR::DATABASE::CANT_FIND_PLATFORMS_FILE : " << PLATFORMS_DEFAULT_FOLDER << std::endl;
		_ReturnAdditionnalInfoOnErrors( file );
	}
}

void Init_RomSettings::_ReturnAdditionnalInfoOnErrors( const std::ifstream& sfile )
{
	std::ios_base::iostate oState = sfile.rdstate();
	if( oState & std::ios_base::eofbit )
		std::cout << "End of file reached." << std::endl;
	else if( oState & std::ios_base::failbit )
		std::cout << "Non-fatal I/O error occurred." << std::endl;
	else if( oState & std::ios_base::badbit )
		std::cout << "Fatal I/O error occurred." << std::endl;
}