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

void Chip8::EmulateCycle()
{
	uint16_t opcode = 0;
	FetchOpcode( opcode );
	DecodeOpcode( opcode );
	ExecuteOpcode();
}

void Chip8::FetchOpcode( uint16_t& opcode )
{
	opcode = memory[PC];
	uint8_t byte2 = memory[PC + 1];

	opcode = opcode << 8;
	opcode |= byte2;

	PC += 2;
}

void Chip8::DecodeOpcode( const uint16_t opcode)
{
	//set X,Y, N, NN, NNN
	uint16_t test = opcode & 0xF000;
	switch (test )
	{
		case 0x0000:
		{
			uint16_t check = opcode & 0x00FF;
			if( check == 0xE0 )
			{
				//Clears the screen
			}
			else if( check == 0xEE )
			{
				//Returns from a subroutine
			}
			else
			{
				//Calls machine code routine (RCA 1802 for COSMAC VIP) at address NNN. Not necessary for most ROMs.
			}
		}
		break;
		case 0x1000 :
			//Jumps to address NNN
			break;
		case 0x2000:
			//Calls subroutine at NNN
			break;
		case 0x3000:
			//Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block)
			break;
		case 0x4000:
			//Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
			break;
		case 0x5000:
			//Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
			break;
		case 0x6000:
			//Sets VX to NN
			break;
		case 0x7000:
			//Adds NN to VX (carry flag is not changed).
			break;
		case 0x8000:
		{
			switch( opcode & 0x000F )
			{
				case 0:
					//Sets VX to the value of VY.
					break;
				case 1:
					//Sets VX to VX or VY. (bitwise OR operation).
					break;
				case 2:
					//Sets VX to VX and VY. (bitwise AND operation).
					break;
				case 3:
					//Sets VX to VX xor VY.
					break;
				case 4:
					//Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.
					break;
				case 5:
					//VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
					break;
				case 6:
					//Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF
					break;
				case 7:
					//Sets VX to VY minus VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX).
					break;
				case 0xE:
					//Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset
					break;
			}
		}
		break;
		case 0x9000:
			//Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block)
			break;
		case 0xA000:
			//Sets I to the address NNN
			break;
		case 0xB000:
			//Jumps to the address NNN plus V0
			break;
		case 0xC000:
			//Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
			break;
		case 0xD000:
			/*Draws a sprite at coordinate(VX, VY) that has a width of 8 pixels and a height of N pixels.
			Each row of 8 pixels is read as bit - coded starting from memory location I; 
			I value does not change after the execution of this instruction.As described above, 
			VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, 
			and to 0 if that does not happen.*/
			break;
		case 0xE000:
		{
			uint16_t check = opcode & 0x00FF;
			if( check == 0x9E )
			{
				//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is pressed (usually the next instruction is a jump to skip a code block).
			}
			else if( check == 0xA1 )
			{
				//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed (usually the next instruction is a jump to skip a code block).
			}
		}
		break;
		case 0xF000:
		{
			std::cout << "F case " << std::endl;
		}
		break;

		default:
		break;
	}
}

void Chip8::ExecuteOpcode()
{
}
