#pragma once
#include "string"
#include <vector>
#include <queue>
#include <map>
#include <cstdint>

class Chip8;
class Disassembler
{
public:
	Disassembler(){};

	static void Disassemble_ROM( const char* memblock, const char* sROMToLoad,const size_t size );

private:
	static void _WriteInstruction( std::string sText,const int iIndex,const uint16_t iOpcode,std::fstream& file,const uint16_t iAdress = 0,const uint8_t iX = 0,const uint8_t iY = 0,const uint8_t NN = 0 );

	struct DisassembledLine
	{
		uint16_t	m_iAddress;
		std::string m_sText;
	};

	static void		_AddToWorklist( const uint16_t iAddr );
	static void		_SkipBlock( const uint16_t iAdress );

	static std::map< uint16_t, DisassembledLine > m_aDisassembly;
	static std::queue<uint16_t> m_aWorklist;

public:
	static const std::map< uint16_t, DisassembledLine >& GetDisassemblyInstructions() { return m_aDisassembly; }

};
