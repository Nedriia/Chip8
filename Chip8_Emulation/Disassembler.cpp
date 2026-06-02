#pragma once
#include "Disassembler.h"
#include <iostream>
#include <fstream>
#include "Chip8.h"
#include <filesystem>

#define DEFAULT_DISSASEMBLY_FOLDER "..\\Dissasembly\\"

std::vector< Disassembler::DisassembledLine > Disassembler::m_aDisassembly = {};
static bool m_bCurrentRomDissasemblyExist = false;

using namespace MemoryMap;
void Disassembler::Disassemble_ROM( const char* memblock, const char* sROMToLoad, const size_t size )
{
	m_aDisassembly.clear();

	std::filesystem::path romPath( sROMToLoad );
	std::filesystem::path outputPath = std::filesystem::path( DEFAULT_DISSASEMBLY_FOLDER ) / romPath.filename();
	outputPath.replace_extension( ".asm" );

	m_bCurrentRomDissasemblyExist = std::filesystem::exists( outputPath );

	std::fstream file;
	file.open( outputPath.string(),m_bCurrentRomDissasemblyExist ? std::ofstream::in : std::ofstream::out );

	if( file.is_open() )
	{
		uint16_t iPC = START_ROM_MEMORY_ADDRESS;
		Chip8* pCPU = Chip8::GetInstance();

		for( uint16_t iPC = 0; iPC < size; iPC += 2 )
		{
			int iIndex = START_ROM_MEMORY_ADDRESS + iPC;
			uint16_t iCurrentOpcode = ( *( pCPU->GetMemory()->begin() + iIndex ) << 8 ) | *( pCPU->GetMemory()->begin() + ( iIndex + 1 ) );
			
			uint8_t X = ( iCurrentOpcode & 0xF00 ) >> 8;
			uint8_t Y = ( iCurrentOpcode & 0xF0 ) >> 4;
			uint16_t NNN = iCurrentOpcode & 0xFFF;
			uint8_t NN = iCurrentOpcode & 0xFF;
			uint8_t N = iCurrentOpcode & 0xF;
			
			uint16_t opcodeNibble = iCurrentOpcode & 0xF000;
			switch( opcodeNibble )
			{
				case 0x0000:
				{
					uint16_t check = iCurrentOpcode & 0x00FF;
					if( Y == 0x0C )
						_WriteInstruction( "%04X		SCD %u",iIndex,iCurrentOpcode,file, N );
					else if( Y == 0x0D )
						_WriteInstruction( "%04X		SCU %u		( XO_CHIP )",iIndex,iCurrentOpcode,file,N );
					else if( check == 0xE0 )
						_WriteInstruction( "%04X		CLS",iIndex,iCurrentOpcode,file );
					else if( check == 0xEE )
						_WriteInstruction( "%04X		RET",iIndex,iCurrentOpcode,file );
					else if( check == 0xFB )
						_WriteInstruction( "%04X		SCR",iIndex,iCurrentOpcode,file );
					else if( check == 0xFC )
						_WriteInstruction( "%04X		SCL",iIndex,iCurrentOpcode,file );
					else if( check == 0xFD )
						_WriteInstruction( "%04X		EXIT",iIndex,iCurrentOpcode,file );
					else if( check == 0xFE )
						_WriteInstruction( "%04X		LORES",iIndex,iCurrentOpcode,file );
					else if( check == 0xFF )
						_WriteInstruction( "%04X		HIRES",iIndex,iCurrentOpcode,file );
					else if( check == 0 )
						_WriteInstruction( "%04X		db %#04X",iIndex,iCurrentOpcode,file, NNN );
					else
						_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iIndex,iCurrentOpcode,file );
				}
				break;
				
				case 0x1000: _WriteInstruction( "%04X		JMP %#04X",iIndex,iCurrentOpcode, file, NNN ); break;
				case 0x2000: _WriteInstruction( "%04X		CALL %#04X",iIndex,iCurrentOpcode,file, NNN ); break;
				case 0x3000: _WriteInstruction( "%04X		SE V%u, %#04X",iIndex,iCurrentOpcode,file, X, NN ); break;
				case 0x4000: _WriteInstruction( "%04X		SNE V%u, %#04X",iIndex,iCurrentOpcode,file, X, NN ); break;
				case 0x5000:
				{
					switch( iCurrentOpcode & 0x000F )
					{
						case 0: _WriteInstruction( "%04X		SE V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 2: _WriteInstruction( "%04X		SAVE V%u - V%u		( XO_CHIP )",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 3: _WriteInstruction( "%04X		LOAD V%u - V%u		( XO_CHIP )",iIndex,iCurrentOpcode,file,X,Y ); break;
						default:	_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iIndex,iCurrentOpcode,file ); break;
					}
				}
				break;
				case 0x6000: _WriteInstruction( "%04X		LD V%u, %#04X",iIndex,iCurrentOpcode,file,X, NN ); break;
				case 0x7000: _WriteInstruction( "%04X		ADD V%u, %#04X",iIndex,iCurrentOpcode,file,X,NN ); break;
				case 0x8000:
				{
					switch( iCurrentOpcode & 0x000F )
					{
						case 0:		_WriteInstruction( "%04X		LD V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 1:		_WriteInstruction( "%04X		OR V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 2:		_WriteInstruction( "%04X		AND V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 3:		_WriteInstruction( "%04X		XOR V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 4:		_WriteInstruction( "%04X		ADD V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 5:		_WriteInstruction( "%04X		SUB V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 6:		_WriteInstruction( "%04X		SHR V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 7:		_WriteInstruction( "%04X		SUBN V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						case 0xE:	_WriteInstruction( "%04X		SHL V%u, V%u",iIndex,iCurrentOpcode,file,X,Y ); break;
						default:	_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iIndex,iCurrentOpcode,file ); break;
					}
				}
				break;
				case 0x9000: _WriteInstruction( "%04X		SNE V%u, V%u",iIndex,iCurrentOpcode,file, X, Y ); break;
				case 0xA000: _WriteInstruction( "%04X		LD I %#04X",iIndex,iCurrentOpcode,file,NNN ); break;
				case 0xB000: _WriteInstruction( "%04X		JMP V0, %#04X",iIndex,iCurrentOpcode,file,NNN ); break;
				case 0xC000: _WriteInstruction( "%04X		RND V0, %#04X",iIndex,iCurrentOpcode,file,NNN ); break;
				case 0xD000: _WriteInstruction( "%04X		DRW %u, %u, %u",iIndex,iCurrentOpcode,file, X, Y, N ); break;
				case 0xE000:
				{
					uint16_t check = iCurrentOpcode & 0x00FF;
					if( check == 0x9E )
						_WriteInstruction( "%04X		SKP V%u",iIndex,iCurrentOpcode,file,X );
					else if( check == 0xA1 )
						_WriteInstruction( "%04X		SKNP V%u",iIndex,iCurrentOpcode,file,X );
					else
						_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iIndex,iCurrentOpcode,file );
				}
				break;
				case 0xF000:
				{
					switch( iCurrentOpcode & 0xF0FF )
					{
						case 0xF000:
						{
							iIndex += 2;
							uint16_t iNextValue = ( *( pCPU->GetMemory()->begin() + iIndex ) << 8 ) | *( pCPU->GetMemory()->begin() + ( iIndex + 1 ) );
							_WriteInstruction( "%04X		LD I, NNNN		( XO_CHIP )",iIndex,iCurrentOpcode,file,iNextValue );
							iPC += 2;
							break;
						}
						case 0xF001: _WriteInstruction( "%04X		PLANE %u		( XO_CHIP )",iIndex,iCurrentOpcode,file,X ); break;
						case 0xF002: _WriteInstruction( "%04X		AUDIO		( XO_CHIP )",iIndex,iCurrentOpcode,file ); break;
						case 0xF007: _WriteInstruction( "%04X		SKNP V%u, DT",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF00A: _WriteInstruction( "%04X		LD V%u, K",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF015: _WriteInstruction( "%04X		LD DT, V%u",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF018: _WriteInstruction( "%04X		LD ST, V%u",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF01E: _WriteInstruction( "%04X		ADD I, V%u",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF029: _WriteInstruction( "%04X		LD FONT, V%u",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF030: _WriteInstruction( "%04X		LD H_FONT, V%u",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF033: _WriteInstruction( "%04X		LD BCD, V%u",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF055: _WriteInstruction( "%04X		LD [I], V%u",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF065: _WriteInstruction( "%04X		LD V%u, [I]",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF075: _WriteInstruction( "%04X		LD RPL, V%u",iIndex,iCurrentOpcode,file, X ); break;
						case 0xF085: _WriteInstruction( "%04X		LD V%u, RPL",iIndex,iCurrentOpcode,file, X ); break;
						_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iIndex,iCurrentOpcode,file );
					}
				}
				break;
				default: _WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iIndex,iCurrentOpcode,file ); break;
			}
		}
	}
}

inline void Disassembler::_WriteInstruction( std::string sText, const int iIndex,const uint16_t iOpcode,std::fstream& file,const uint16_t iAdress/* = 0*/,const uint8_t iX/* = 0*/,const uint8_t iY/* = 0*/,const uint8_t NN/* = 0*/ )
{
	DisassembledLine line;

	char buffer[ 64 ];
	snprintf( buffer,sizeof( buffer ),sText.c_str(),iOpcode,iAdress,iX,iY,NN );

	if( !m_bCurrentRomDissasemblyExist )
		file << "0x" << std::hex << iIndex << ": " << buffer << "\n";

	line.m_iAddress = iIndex;
	line.m_sText = buffer;

	m_aDisassembly.push_back( line );
}