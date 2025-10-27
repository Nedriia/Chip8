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
	std::ifstream file( sPath + sROMToLoad, std::ios::binary | std::ios::in | std::ios::ate );

	if( file.is_open() )
	{
		std::streamsize size = file.tellg();
		if( size <= 0 || size > MEMORY_SIZE - START_ROM_MEMORY_ADDRESS )
		{
			throw std::runtime_error( "Size ROM invalid" );
			return;
		}

		char* memblock = new char[ size ];
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
			memory[ START_ROM_MEMORY_ADDRESS + i ] = static_cast< uint8_t >( memblock[ i ] );
		}

		delete[] memblock;
	}
}

void Chip8::EmulateCycle( bool bRefresh )
{
	if( delay_timer == 0 )
	{
		uint16_t opcode = 0;
		_FetchOpcode( opcode );
		_DecodeExecute_Opcode( opcode );
	}

	if( bRefresh )
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
	const char* OpcodeInstruct ="";

	uint16_t NNN	= opcode & 0x0FFF;
	uint8_t NN		= opcode & 0x00FF;
	uint8_t N		= opcode & 0x000F;
	uint8_t X		= ( opcode & 0x0F00 ) >> 8;
	uint8_t Y		= ( opcode & 0x00F0 ) >> 4;
	uint8_t& VF		= registers[15];

	uint16_t opcodeNibble = opcode & 0xF000;
	switch( opcodeNibble )
	{
	case 0x0000:
	{
		uint16_t check = opcode & 0x00FF;
		if( check == 0xE0 )
		{
			//Clears the screen
			Display::ClearScreen();
			OpcodeInstruct = "ONNN : CLS";
		}
		else if( check == 0xEE )
		{
			//Returns from a subroutine
			PC = stack[ SP ];
			stack[ SP ] = 0;
			if( SP != 0 )
				--SP;
			OpcodeInstruct = "0xEE : RET";
		}
		else
		{
			//Calls machine code routine (RCA 1802 for COSMAC VIP) at address NNN. Not necessary for most ROMs.
		}
	}
	break;
	case 0x1000:
	//Jumps to address NNN
	{
		PC = NNN;
		OpcodeInstruct = "0x1000 : JMP NNN";
	}
	break;
	case 0x2000:
	//Calls subroutine at NNN
	{
		if( stack[0] != 0 ) //could be simplier to use int and init the value to -1 but I want to keep 0 as start
			++SP;
		stack[ SP ] = PC;
		PC = NNN;
		OpcodeInstruct = "0x2000 : CALL NNN";
	}
	break;
	case 0x3000:
	//Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block)
	{
		if( registers[ X ] == NN )
			PC += 2;
		OpcodeInstruct = "0x3000 : SE VX, NN";
	}
	break;
	case 0x4000:
	//Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
	{
		if( registers[ X ] != NN )
			PC += 2;
		OpcodeInstruct = "0x4000 : SNE VX, NN";
	}
	break;
	case 0x5000:
	//Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
	{
		if( registers[ X ] == registers[ Y ] )
			PC += 2;
		OpcodeInstruct = "0x5000 : SE VX, VY";
	}
	break;
	case 0x6000:
	//Sets NN To VX
	{
		registers[ X ] = NN;
		OpcodeInstruct = "0x6000 : LD VX, NN";
	}
	break;
	case 0x7000:
	//Adds NN to VX (carry flag is not changed).
	{
		registers[ X ] += NN;
		OpcodeInstruct = "0x7000 : ADD VX, NN";
	}
	break;
	case 0x8000:
	{
		switch( opcode & 0x000F )
		{
		case 0:
		//Sets VX to the value of VY.
		{
			registers[ X ] = registers[ Y ];
			OpcodeInstruct = "0x8000 : LD VX, VY";
		}
		break;
		case 1:
		//Sets VX to VX or VY. (bitwise OR operation).
		{
			registers[ X ] |= registers[ Y ];
			OpcodeInstruct = "0x8001 : OR VX, VY";
		}
		break;
		case 2:
		//Sets VX to VX and VY. (bitwise AND operation).
		{
			registers[ X ] &= registers[ Y ];
			OpcodeInstruct = "0x8002 : AND VX, VY";
		}
		break;
		case 3:
		//Sets VX to VX xor VY.
		{
			registers[ X ] ^= registers[ Y ];
			OpcodeInstruct = "0x8003 : XOR VX, VY";
		}
		break;
		case 4:
		//Adds VY to VX.
		{
			uint16_t sum = registers[ X ] + registers[ Y ];
			registers[ X ] = sum & 0xFF;
			VF = ( sum > 0xFF ) ? 1 : 0;
			//VF is set to 1 when there's an overflow, and to 0 when there is not.
			OpcodeInstruct = "0x8004 : ADD VX, VY";
		}
		break;
		case 5:
		//VY is subtracted from VX.
		{
			uint16_t diff = registers[ X ] - registers[ Y ];
			registers[ X ] = diff & 0xFF;
			VF = 1;
			if( diff > 0xFF )
				VF = 0;
			// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
			OpcodeInstruct = "0x8005 : SUB VX, VY";
		}
		break;
		case 6:
		//If the least - significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
		{
			uint8_t LSB = ( registers[ X ] & 1 );
			registers[ X ] >>= 1; // OR registers[ X ] = registers[ Y ] >> 1 before 1990
			VF = LSB;
			OpcodeInstruct = "0x8006 : SHR VX, VY";
		}
		break;
		case 7:
		//Sets VX equals to VY minus VX.
		{
			uint16_t diff = registers[ Y ] - registers[ X ];
			registers[ X ] = diff & 0xFF;
			VF = 1;
			if( diff > 0xFF )
				VF = 0;
			// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX).
			OpcodeInstruct = "0x8007 : SUBN VX, VY";
		}
		break;
		case 0xE:
		// If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
		{
			uint8_t MSB = ( registers[ X ] & 0x80 ) == 0x80 ? 1 : 0;
			registers[ X ] <<= 1; // OR registers[ X ] = registers[ Y ] << 1 before 1990
			VF = MSB;
			OpcodeInstruct = "0x800E : SHL VX, VY";
		}
		break;
		default:
			break;
		}
	}
	break;
	case 0x9000:
	//Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block)
	{
		if( registers[ X ] != registers[ Y ] )
			PC += 2;
		OpcodeInstruct = "0x9000 : SNE VX, VY";
	}
	break;
	case 0xA000:
	//Sets I to the address NNN
	{
		I = NNN;
		OpcodeInstruct = "0xA000 : LD I, NNN";
	}
	break;
	case 0xB000:
	//Jumps to the address NNN plus V0
	{
		PC = NNN + registers[ 0 ];
		OpcodeInstruct = "0xB000 : JMP V0, NNN";
	}
	break;
	case 0xC000:
	//Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
	{
		std::uniform_int_distribution<std::mt19937::result_type> dist( 0, 255 );
		registers[ X ] = dist( rng ) & NN;
		OpcodeInstruct = "0xC000 : RND VX, NN";
	}
	break;
	case 0xD000:
	{
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
			Display::DrawPixelAtPos( registers[ X ], registers[ Y ] + iYOffset, memory[ I + iYOffset ], bErased );
			VF = bErased ? 1 : 0;
		}
		OpcodeInstruct = "0xD000 : DRW VX, VY, N";
		break;
	}
	case 0xE000:
	{
		uint16_t check = opcode & 0x00FF;
		if( check == 0x9E )
		{
			//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is pressed (usually the next instruction is a jump to skip a code block).
			OpcodeInstruct = "0xE09E : SKP VX";
		}
		else if( check == 0xA1 )
		{
			//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed (usually the next instruction is a jump to skip a code block).
			OpcodeInstruct = "0xE0A1 : SKNP VX";
		}
	}
	break;
	case 0xF000:
	{
		switch( opcode & 0x00FF )
		{
		case 7:
			//Sets VX to the value of the delay timer.
			registers[ X ] = delay_timer;
			OpcodeInstruct = "0xF007 : LD VX, DT";
			break;
		case 0x0A:
		{
			//A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event, delay and sound timers should continue processing)
			OpcodeInstruct = "0xF00A : LD VX, KEY";
		}
		break;
		case 0x15:
			//Sets the delay timer to VX
			delay_timer = registers[ X ];
			OpcodeInstruct = "0xF015 : LD DT, VX";
			break;
		case 0x18:
			//Sets the sound timer to VX
			sound_timer = registers[ X ];
			OpcodeInstruct = "0xF018 : LD ST, VX";
			break;
		case 0x1E:
			//Adds VX to I. VF is not affected
			I += registers[ X ];
			OpcodeInstruct = "0xF01E : ADD I, VXX";
			break;
		case 0x29:
		{
			//Sets I to the location of the sprite for the character in VX(only consider the lowest nibble). Characters 0-F (in hexadecimal) are represented by a 4x5 font.
			I = fontset[ registers[ X ] ];
			OpcodeInstruct = "0xF029 : LD I, FONT(VX)";
			//I = memory[ registers[ X ] ];
		}
		break;
		case 0x33:
		{
			//Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2
			memory[ I ] = registers[ X ] / 100;
			memory[ I + 1 ] = ( registers[ X ] / 10 ) % 10;
			memory[ I + 2 ] = ( registers[ X ] % 100 ) % 10;
			OpcodeInstruct = "0xF033 : BCD VX";
		}
		break;
		case 0x55:
		{
			//Stores from V0 to VX (including VX) in memory, starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified
			for( int i = 0; i <= X; ++i )
			{
				memory[ I + i ] = registers[ i ];
			}
			OpcodeInstruct = "0xF055 : LD [I], VX";
		}
		break;
		case 0x65:
		{
			//Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified
			for( int i = 0; i <= X; ++i )
			{
				registers[ i ] = memory[ I + i ];
			}
			OpcodeInstruct = "0xF065 : LD VX, [I]";
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

	if( OpcodeInstruct != "" )
		Chip8_Debugger::GetInstance()->AddEntryOpcode( OpcodeInstruct );
}

void Chip8::_UpdateTimers()
{
	if( delay_timer >= 0 )
		--delay_timer;

	if( sound_timer >= 0 )
		--sound_timer;
}