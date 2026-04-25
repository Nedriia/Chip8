#pragma once

class Init_RomSettings
{

public:
	void LookForDatabaseInfos( const char* memblock,const size_t& size );

private:
	int _CalculateHash_RetrieveIndex( const char* memblock,const size_t& size );
	int _FindIndex();
	void _LoadProgramsSettings( const int iIndex );

};