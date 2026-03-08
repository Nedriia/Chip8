#include "Chip8.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include "Display.h"
#include "Chip8_Debugger.h"

#define START_FONT_MEMORY_ADDRESS 0x050
#define START_ROM_MEMORY_ADDRESS 0x200
#define MEMORY_SIZE 4096
#define DEFAULT_PARENT_ROM_FOLDER "..\\"

Chip8::Chip8() :
	I( 0 ),
	SP( 0 ),
	PC( 0 ),
	delay_timer( 0 ),
	sound_timer( 0 )
{
}

void Chip8::Init( const char* sROMToLoad )
{
	unsigned timeSeed = std::chrono::steady_clock::now().time_since_epoch().count();
	rng.seed( timeSeed );

	LoadFont();
	LoadROM( sROMToLoad );

	PC = START_ROM_MEMORY_ADDRESS;
	SP = 0;
}

void Chip8::LoadFont()
{
	for( uint8_t i = 0; i < 80; ++i )
	{
		memory[ START_FONT_MEMORY_ADDRESS + i ] = fontset[ i ];
	}
}

void Chip8::LoadROM( const char* sROMToLoad )
{
	//TODO : Add Check here if command not empty
	std::string sPath = DEFAULT_PARENT_ROM_FOLDER;
	std::ifstream file( sPath + sROMToLoad,std::ios::binary | std::ios::in | std::ios::ate );

	if( file.is_open() )
	{
		std::streamsize size = file.tellg();
		if( size <= 0 || size > MEMORY_SIZE - START_ROM_MEMORY_ADDRESS )
		{
			throw std::runtime_error( "Size ROM invalid" );
			return;
		}

		char* memblock = new char[ size ];
		file.seekg( 0,std::ios::beg );
		file.read( memblock,size );
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
			memory[ START_ROM_MEMORY_ADDRESS + i ] = static_cast< uint8_t >( memblock[ i ] );
		}

		delete[] memblock;
	}
}

void Chip8::EmulateCycle()
{
	if( Chip8_Debugger::GetInstance() && Chip8_Debugger::GetInstance()->IsEmulationPaused() )
		return;

	if( delay_timer == 0 )
	{
		uint16_t opcode = 0;
		_FetchOpcode( opcode );
		_DecodeExecute_Opcode( opcode );
	}

	_UpdateTimers();
}

void Chip8::_FetchOpcode( uint16_t& opcode )
{
	opcode = memory[ PC ];
	uint8_t byte2 = memory[ PC + 1 ];

	opcode = opcode << 8;
	opcode |= byte2;

	PC += 2;
}

void Chip8::_DecodeExecute_Opcode( const uint16_t opcode )
{
	std::string OpcodeInstruct;

	uint16_t NNN = opcode & 0x0FFF;
	uint8_t NN = opcode & 0x00FF;
	uint8_t N = opcode & 0x000F;
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	uint8_t& VF = registers[ 15 ];

	uint16_t opcodeNibble = opcode & 0xF000;
	switch( opcodeNibble )
	{
	case 0x0000:
	{
		uint16_t check = opcode & 0x00FF;
		if( check == 0xE0 )
		{
			OpcodeInstruct = "00E0 : CLS";

			//Clear the screen
			Display::ClearScreen();
		}
		else if( check == 0xEE )
		{
			OpcodeInstruct = "00EE : RET";

			//Returns from a subroutine
			PC = stack[ SP ];
			stack[ SP ] = 0;
			if( SP != 0 )
				--SP;
		}
		else
		{
			//Calls machine code routine (RCA 1802 for COSMAC VIP) at address NNN. Not necessary for most ROMs.
		}
	}
	break;
	case 0x1000:
	{
		OpcodeInstruct = std::format( "1000 : JMP {:#06X} )",NNN );

		//Jumps to address NNN
		PC = NNN;
	}
	break;
	case 0x2000:
	{
		if( stack[ 0 ] != 0 ) //could be simplier to use int and init the value to -1 but I want to keep 0 as start
			++SP;

		OpcodeInstruct = std::format( "2000 : CALL {:#06X}",NNN );

		//Calls subroutine at NNN
		stack[ SP ] = PC;
		PC = NNN;
	}
	break;
	case 0x3000:
	{
		OpcodeInstruct = std::format( "3000 : SE VX, NN {}", NN );

		//Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block)
		if( registers[ X ] == NN )
			PC += 2;
	}
	break;
	case 0x4000:
	{
		OpcodeInstruct = std::format( "4000 : SNE VX, NN {:#02X}",NN );

		//Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
		if( registers[ X ] != NN )
			PC += 2;
	}
	break;
	case 0x5000:
	{
		OpcodeInstruct = "5000 : SE VX, VY";

		//Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
		if( registers[ X ] == registers[ Y ] )
			PC += 2;
	}
	break;
	case 0x6000:
	{
		OpcodeInstruct = std::format( "6000 : LD VX, NN {:#02X}", NN );

		//Sets NN To VX
		registers[ X ] = NN;
	}
	break;
	case 0x7000:
	{
		OpcodeInstruct = std::format( "7000 : ADD VX, NN {:#02X}",NN );

		//Adds NN to VX (carry flag is not changed).
		registers[ X ] += NN;
	}
	break;
	case 0x8000:
	{
		switch( opcode & 0x000F )
		{
		case 0:
		{
			OpcodeInstruct = "8000 : LD VX, VY";

			//Sets VX to the value of VY.
			registers[ X ] = registers[ Y ];
		}
		break;
		case 1:
		{
			OpcodeInstruct = "8001 : OR VX, VY";

			//Sets VX to VX or VY. (bitwise OR operation)
			registers[ X ] |= registers[ Y ];
		}
		break;
		case 2:
		{
			OpcodeInstruct = "8002 : AND VX, VY";

			//Sets VX to VX and VY. (bitwise AND operation)
			registers[ X ] &= registers[ Y ];
		}
		break;
		case 3:
		{
			OpcodeInstruct = "8003 : XOR VX, VY";

			//Sets VX to VX xor VY
			registers[ X ] ^= registers[ Y ];
		}
		break;
		case 4:
		{
			OpcodeInstruct = "8004 : ADD VX, VY ( VF )";

			//Adds VY to VX
			uint16_t sum = registers[ X ] + registers[ Y ];
			registers[ X ] = sum & 0xFF;
			VF = ( sum > 0xFF ) ? 1 : 0;
			//VF is set to 1 when there's an overflow, and to 0 when there is not.
		}
		break;
		case 5:
		{
			OpcodeInstruct = "0x8005 : SUB VX, VY ( VF )";

			//VY is subtracted from VX.
			uint16_t diff = registers[ X ] - registers[ Y ];
			registers[ X ] = diff & 0xFF;
			VF = 1;
			if( diff > 0xFF )
				VF = 0;
			// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
		}
		break;
		case 6:
		{
			OpcodeInstruct = "8006 : SHR VX, VY";

			//If the least - significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
			uint8_t LSB = ( registers[ X ] & 1 );
			registers[ X ] >>= 1; // OR registers[ X ] = registers[ Y ] >> 1 before 1990
			VF = LSB;
		}
		break;
		case 7:
		{
			OpcodeInstruct = "8007 : SUBN VX, VY";

			//Sets VX equals to VY minus VX.
			uint16_t diff = registers[ Y ] - registers[ X ];
			registers[ X ] = diff & 0xFF;
			VF = 1;
			if( diff > 0xFF )
				VF = 0;
			// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX).
		}
		break;
		case 0xE:
		{
			OpcodeInstruct = "800E : SHL VX";

			// If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
			uint8_t MSB = ( registers[ X ] & 0x80 ) == 0x80 ? 1 : 0;
			registers[ X ] <<= 1; // OR registers[ X ] = registers[ Y ] << 1 before 1990
			VF = MSB;
		}
		break;
		default:
			break;
		}
	}
	break;
	case 0x9000:
	{
		OpcodeInstruct = "9000 : SNE VX, VY";

		//Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block)
		if( registers[ X ] != registers[ Y ] )
			PC += 2;
	}
	break;
	case 0xA000:
	{
		OpcodeInstruct = std::format( "A000 : LD I, NNN {:#06X}",NNN );

		//Sets I to the address NNN
		I = NNN;
	}
	break;
	case 0xB000:
	{
		OpcodeInstruct = std::format( "B000 : JMP V0, NNN {:#06X}",NNN );

		//Jumps to the address NNN plus V0
		PC = NNN + registers[ 0 ];
	}
	break;
	case 0xC000:
	{
		OpcodeInstruct = "C000 : RND VX, NN";

		//Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
		std::uniform_int_distribution<std::mt19937::result_type> dist( 0,255 );
		registers[ X ] = dist( rng ) & NN;
	}
	break;
	case 0xD000:
	{
		OpcodeInstruct = std::format( "D000 : DRW N {} VX, VY",N );

		/*Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
		The interpreter reads N bytes from memory, starting at the address stored in I.
		These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). Sprites are XORed onto the existing screen.
		If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0.
		If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen
		I value does not change after the execution of this instruction*/
		bool bErased = false;
		uint8_t iYOffset = 0;
		for( ; iYOffset < N; ++iYOffset )
		{
			Display::DrawPixelAtPos( registers[ X ],registers[ Y ] + iYOffset,memory[ I + iYOffset ],bErased );
			VF = bErased ? 1 : 0;
		}

		break;
	}
	case 0xE000:
	{
		uint16_t check = opcode & 0x00FF;
		if( check == 0x9E )
		{
			OpcodeInstruct = "E09E : SKP";

			//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is pressed (usually the next instruction is a jump to skip a code block).
		}
		else if( check == 0xA1 )
		{
			OpcodeInstruct = "E0A1 : SKNP";

			//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed (usually the next instruction is a jump to skip a code block).
		}
	}
	break;
	case 0xF000:
	{
		switch( opcode & 0x00FF )
		{
		case 7:
			OpcodeInstruct = "F007 : LD VX, DT";

			//Sets VX to the value of the delay timer.
			registers[ X ] = delay_timer;
			break;
		case 0x0A:
		{
			OpcodeInstruct = "F00A : LD VX, KEY";

			//A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event, delay and sound timers should continue processing)
		}
		break;
		case 0x15:
			OpcodeInstruct = "F015 : LD DT, VX";

			//Sets the delay timer to VX
			delay_timer = registers[ X ];
			break;
		case 0x18:
			OpcodeInstruct = "F018 : LD ST, VX";

			//Sets the sound timer to VX
			sound_timer = registers[ X ];
			break;
		case 0x1E:
			OpcodeInstruct = "F01E : ADD I, VX";

			//Adds VX to I. VF is not affected
			I += registers[ X ];
			break;
		case 0x29:
		{
			OpcodeInstruct = "F029 : LD I, FONT( VX )";

			//Sets I to the location of the sprite for the character in VX(only consider the lowest nibble). Characters 0-F (in hexadecimal) are represented by a 4x5 font.
			I = fontset[ registers[ X ] ];
		}
		break;
		case 0x33:
		{
			OpcodeInstruct = "F033 : BCD VX";

			//Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2
			memory[ I ] = registers[ X ] / 100;
			memory[ I + 1 ] = ( registers[ X ] / 10 ) % 10;
			memory[ I + 2 ] = ( registers[ X ] % 100 ) % 10;
		}
		break;
		case 0x55:
		{
			OpcodeInstruct = "F055 : LD [I], VX";

			//Stores from V0 to VX (including VX) in memory, starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified
			for( int i = 0; i <= X; ++i )
			{
				memory[ I + i ] = registers[ i ];
			}
		}
		break;
		case 0x65:
		{
			OpcodeInstruct = "F065 : LD VX, [I]";

			//Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified
			for( int i = 0; i <= X; ++i )
			{
				registers[ i ] = memory[ I + i ];
			}
		}
		break;
		default:
			break;
		}
	}
	break;

	default:
		break;
	}

	if( !OpcodeInstruct.empty() )
		_AddOpcodeToHistory( OpcodeInstruct.c_str() );
}

void Chip8::_UpdateTimers()
{
	if( delay_timer >= 0 )
		--delay_timer;

	if( sound_timer >= 0 )
		--sound_timer;
}

void Chip8::_AddOpcodeToHistory( const char* pOpcode )
{
	//https://github.com/trapexit/chip-8_documentation
	if( m_aOpcodeHistory.size() >= 0x200 )
		m_aOpcodeHistory.pop_front();

	m_aOpcodeHistory.push_back( pOpcode );
}