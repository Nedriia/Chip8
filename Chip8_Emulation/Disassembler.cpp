#include "Disassembler.h"
#include <iostream>
#include <fstream>
#include "Chip8.h"
#include <filesystem>
#include <unordered_set>

#define DEFAULT_DISSASEMBLY_FOLDER "..\\Dissasembly\\"

std::map< uint16_t, Disassembler::DisassembledLine > Disassembler::m_aDisassembly;
std::queue<uint16_t> Disassembler::m_aWorklist = {};

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

		std::unordered_set<uint16_t> aVisited;
		std::deque<uint16_t> aStack;

		m_aWorklist.push( iPC );

		while( m_aWorklist.empty() == false )
		{
			uint16_t iAdress = m_aWorklist.front();
			m_aWorklist.pop();

			if( aVisited.find( iAdress ) != aVisited.end() )
			{
				if( m_aWorklist.empty() )
				{
					if( aStack.empty() == false )
					{
						_AddToWorklist( aStack.back() );
						aStack.pop_back();
						continue;
					}
					else
						break;
				}
				else
					continue;
			}

			aVisited.insert( iAdress );

			uint16_t iTempAdress = m_aWorklist.size();
			uint16_t iCurrentOpcode = ( ( pCPU->GetMemoryAtAddr( iAdress )  << 8 ) | pCPU->GetMemoryAtAddr( iAdress + 1 ) );

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
						_WriteInstruction( "%04X		SCD %u",iAdress,iCurrentOpcode,file,N );
					else if( Y == 0x0D )
					{
						_WriteInstruction( "%04X		SCU %u		( XO_CHIP )",iAdress,iCurrentOpcode,file,N );
						pCPU->SetIfCurrentRomXoChip( true );
					}
					else if( check == 0xE0 )
						_WriteInstruction( "%04X		CLS",iAdress,iCurrentOpcode,file );
					else if( check == 0xEE )
					{
						_WriteInstruction( "%04X		RET",iAdress,iCurrentOpcode,file );
						if( aStack.empty() == false )
						{
							_AddToWorklist( aStack.back() );
							aStack.pop_back();
						}
					}
					else if( check == 0xFB )
						_WriteInstruction( "%04X		SCR",iAdress,iCurrentOpcode,file );
					else if( check == 0xFC )
						_WriteInstruction( "%04X		SCL",iAdress,iCurrentOpcode,file );
					else if( check == 0xFD )
					{
						_WriteInstruction( "%04X		EXIT",iAdress,iCurrentOpcode,file );
						return;
					}
					else if( check == 0xFE )
						_WriteInstruction( "%04X		LORES",iAdress,iCurrentOpcode,file );
					else if( check == 0xFF )
						_WriteInstruction( "%04X		HIRES",iAdress,iCurrentOpcode,file );
					else if( check == 0 )
						_WriteInstruction( "%04X		db %#04X",iAdress,iCurrentOpcode,file,NNN );
					else
						_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iAdress,iCurrentOpcode,file );
				}
				break;

				case 0x1000:
				{
					_WriteInstruction( "%04X		JMP %#04X",iAdress,iCurrentOpcode,file,NNN );
					_AddToWorklist( NNN );
					break;
				}
				case 0x2000:
				{
					_WriteInstruction( "%04X		CALL %#04X",iAdress,iCurrentOpcode,file,NNN );
					_AddToWorklist( NNN );
					aStack.push_back( iAdress + 2 );
					break;
				}
				case 0x3000:
				{
					_WriteInstruction( "%04X		SE V%u, %#04X",iAdress,iCurrentOpcode,file,X,NN );
					_SkipBlock( iAdress );
					break;
				}
				case 0x4000: 
				{
					_WriteInstruction( "%04X		SNE V%u, %#04X",iAdress,iCurrentOpcode,file,X,NN );
					_SkipBlock( iAdress );
					break;
				}
				case 0x5000:
				{
					switch( iCurrentOpcode & 0x000F )
					{
					case 0: 
					{
						_WriteInstruction( "%04X		SE V%u, V%u",iAdress,iCurrentOpcode,file,X,Y );
						_SkipBlock( iAdress );
						break;
					}
					case 2: _WriteInstruction( "%04X		SAVE V%u - V%u		( XO_CHIP )",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 3: _WriteInstruction( "%04X		LOAD V%u - V%u		( XO_CHIP )",iAdress,iCurrentOpcode,file,X,Y ); break;
					default:_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iAdress,iCurrentOpcode,file ); break;
					}
				}
				break;
				case 0x6000: _WriteInstruction( "%04X		LD V%u, %#04X",iAdress,iCurrentOpcode,file,X,NN ); break;
				case 0x7000: _WriteInstruction( "%04X		ADD V%u, %#04X",iAdress,iCurrentOpcode,file,X,NN ); break;
				case 0x8000:
				{
					switch( iCurrentOpcode & 0x000F )
					{
					case 0:		_WriteInstruction( "%04X		LD V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 1:		_WriteInstruction( "%04X		OR V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 2:		_WriteInstruction( "%04X		AND V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 3:		_WriteInstruction( "%04X		XOR V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 4:		_WriteInstruction( "%04X		ADD V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 5:		_WriteInstruction( "%04X		SUB V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 6:		_WriteInstruction( "%04X		SHR V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 7:		_WriteInstruction( "%04X		SUBN V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					case 0xE:	_WriteInstruction( "%04X		SHL V%u, V%u",iAdress,iCurrentOpcode,file,X,Y ); break;
					default:	_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iAdress,iCurrentOpcode,file ); break;
					}
				}
				break;
				case 0x9000:
				{
					_WriteInstruction( "%04X		SNE V%u, V%u",iAdress,iCurrentOpcode,file,X,Y );
					_SkipBlock( iAdress );
					break;
				}
				case 0xA000: _WriteInstruction( "%04X		LD I %#04X",iAdress,iCurrentOpcode,file,NNN ); break;
				case 0xB000:
				{
					_WriteInstruction( "%04X		JMP V0, %#04X",iAdress,iCurrentOpcode,file,NNN );
					_AddToWorklist( NNN ); //Can't precisely determine the jump ( NNN + V0 || NNN + VX )
					break;
				}
				case 0xC000: _WriteInstruction( "%04X		RND V0, %#04X",iAdress,iCurrentOpcode,file,NNN ); break;
				case 0xD000: _WriteInstruction( "%04X		DRW %u, %u, %u",iAdress,iCurrentOpcode,file,X,Y,N ); break;
				case 0xE000:
				{
					uint16_t check = iCurrentOpcode & 0x00FF;
					if( check == 0x9E )
					{
						_WriteInstruction( "%04X		SKP V%u",iAdress,iCurrentOpcode,file,X );
						_SkipBlock( iAdress );
					}
					else if( check == 0xA1 )
					{
						_WriteInstruction( "%04X		SKNP V%u",iAdress,iCurrentOpcode,file,X );
						_SkipBlock( iAdress );
					}
					else
						_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iAdress,iCurrentOpcode,file );
				}
				break;
				case 0xF000:
				{
					switch( iCurrentOpcode & 0xF0FF )
					{
					case 0xF000:
					{
						uint16_t iNextValue = ( *( pCPU->GetMemory()->begin() + iAdress + 2 ) << 8 ) | *( pCPU->GetMemory()->begin() + ( iAdress + 3 ) );
						_WriteInstruction( "%04X		LD I, NNNN		( XO_CHIP )",iAdress,iCurrentOpcode,file,iNextValue );
						_AddToWorklist( iAdress + 4 );
						pCPU->SetIfCurrentRomXoChip( true );
						break;
					}
					case 0xF001:
					{
						_WriteInstruction( "%04X		PLANE %u		( XO_CHIP )",iAdress,iCurrentOpcode,file,X );
						pCPU->SetIfCurrentRomXoChip( true );
						break;
					}
					case 0xF002:
					{
						_WriteInstruction( "%04X		AUDIO		( XO_CHIP )",iAdress,iCurrentOpcode,file );
						pCPU->SetIfCurrentRomXoChip( true );
						break;
					}
					case 0xF007: _WriteInstruction( "%04X		LD V%u, DT",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF00A: _WriteInstruction( "%04X		LD V%u, K",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF015: _WriteInstruction( "%04X		LD DT, V%u",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF018: _WriteInstruction( "%04X		LD ST, V%u",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF01E: _WriteInstruction( "%04X		ADD I, V%u",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF029: _WriteInstruction( "%04X		LD FONT, V%u",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF030: _WriteInstruction( "%04X		LD H_FONT, V%u",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF033: _WriteInstruction( "%04X		LD BCD, V%u",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF03A:
					{
						_WriteInstruction( "%04X		PITCH, V%u",iAdress,iCurrentOpcode,file,X );
						pCPU->SetIfCurrentRomXoChip( true );
						break;
					}
					case 0xF055: _WriteInstruction( "%04X		LD [I], V%u",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF065: _WriteInstruction( "%04X		LD V%u, [I]",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF075: _WriteInstruction( "%04X		LD RPL, V%u",iAdress,iCurrentOpcode,file,X ); break;
					case 0xF085: _WriteInstruction( "%04X		LD V%u, RPL",iAdress,iCurrentOpcode,file,X ); break;
						_WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iAdress,iCurrentOpcode,file );
					}
				}
				break;
				default: _WriteInstruction( "WARNING::UNKNOWN_OPCODE::%#04X",iAdress,iCurrentOpcode,file ); break;
			}

			if( iTempAdress == m_aWorklist.size() )
				_AddToWorklist( iAdress + 2 );

			if( m_aWorklist.empty() )
			{
				if( aStack.empty() == false )
				{
					_AddToWorklist( aStack.back() );
					aStack.pop_back();
				}
			}
		}
	}
}

inline void Disassembler::_WriteInstruction( std::string sText, const int iIndex,const uint16_t iOpcode,std::fstream& file,const uint16_t iAdress/* = 0*/,const uint8_t iX/* = 0*/,const uint8_t iY/* = 0*/,const uint8_t NN/* = 0*/ )
{
	DisassembledLine line;

	char buffer[ 64 ];
	snprintf( buffer,sizeof( buffer ),sText.c_str(),iOpcode,iAdress,iX,iY,NN );
	
	std::stringstream sstream;
	sstream << "0x" << std::hex << iIndex << ": ";

	if( !m_bCurrentRomDissasemblyExist )
		file << sstream.str() << ": " << buffer << "\n";

	line.m_iAddress = iIndex;
	line.m_sText = sstream.str() + buffer;
	m_aDisassembly.insert( { iIndex, line } );
}

void Disassembler::_AddToWorklist( const uint16_t iAddr )
{
	m_aWorklist.push( iAddr );
}

void Disassembler::_SkipBlock( const uint16_t iAdress )
{
	_AddToWorklist( iAdress + 4 );
	_AddToWorklist( iAdress + 2 );
}
