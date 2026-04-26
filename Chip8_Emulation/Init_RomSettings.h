#pragma once
#include <fstream>
#include <vector>

class Init_RomSettings
{

public:
	void LookForDatabaseInfos( const char* memblock,const size_t& size );

private:
	int _CalculateHash_RetrieveIndex( const char* memblock,const size_t& size );
	int _FindIndex();
	void _LoadProgramsSettings( const int iIndex );
	void _LoadPlatformsSettings( const std::vector<std::string >& sPlatforms, const int iRomCustomTickrate );
	void _LoadPlatformsSpecs( const std::string& sPlatform, const int iRomCustomTickrate );
	void _ReturnAdditionnalInfoOnErrors( const std::ifstream& sfile );
};