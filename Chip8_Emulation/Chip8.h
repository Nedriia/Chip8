#pragma once

#include <stdint.h>
#include <random>
#include <deque>

class Chip8
{
public:
	Chip8();
	void Init( const char* sROMToLoad );
	void LoadFont();
	void LoadROM( const char* sROMToLoad );

	void EmulateCycle();

	const uint16_t* GetStack() const { return stack; }
	const uint8_t* GetMemory() const { return memory; }
	const uint8_t* GetRegisters() const { return registers; }
	const std::deque<std::string>& GetHistoryOpcode() const { return m_aOpcodeHistory; }

	uint16_t	GetI() const { return I;}
	uint16_t	GetPC() const { return PC;}

	uint8_t		GetSP() const { return SP; }
	uint8_t		GetDelayTimer() const { return delay_timer;}
	uint8_t		GetSoundTimer() const { return sound_timer;}

private:
	void _FetchOpcode( uint16_t& opcode );
	void _DecodeExecute_Opcode( const uint16_t opcode );
	void _UpdateTimers();
	void _AddOpcodeToHistory( const char* pOpcode );

	uint8_t memory[ 0x1000 ] = { 0 };
	uint8_t registers[ 0x10 ] = { 0 };
	uint16_t I; //Address register
	uint16_t stack[ 0x10 ] = { 0 };
	uint8_t SP; //Stack pointer
	uint16_t PC; //Program counter

	uint8_t delay_timer;
	uint8_t sound_timer;

	std::mt19937 rng;

	uint8_t fontset[ 0x50 ] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	std::deque<std::string> m_aOpcodeHistory;

};