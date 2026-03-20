#pragma once

#include <random>
#include <deque>
#include <chrono>

enum class RunningState
{
	Running,
	Pause,
	StepNextFrame,
	Reset
};

template< typename T>
struct Data
{
public:
	Data() { data = 0; bNewValue = false; }
	Data& operator=( const T& rhs )
	{
		bNewValue = ( data != rhs );
		data = rhs;
		return *this;
	}

	Data& operator+=( const T& rhs ){ return Update( [ & ] { data += rhs; } ); }
	Data& operator--(){ return Update( [ & ] { --data; } ); }
	Data& operator++(){ return Update( [ & ] { ++data; } ); }
	Data& operator|=( const T& rhs ){ return Update( [ & ] { data |= rhs; } ); }
	Data& operator&=( const T& rhs ){ return Update( [ & ] { data &= rhs; } ); }
	Data& operator^=( const T& rhs ){ return Update( [ & ] { data ^= rhs; } ); }
	Data& operator>>=( const T& rhs ) { return Update( [ & ] { data >>= rhs; } ); }
	Data& operator<<=( const T& rhs ) { return Update( [ & ] { data <<= rhs; } ); }

	operator T() const { return data; }
	bool HasChanged() const { return bNewValue; }
	bool IsNULL() const { return data == 0; }
	void clear() { data = 0; bNewValue = false; }

private:
	T data;
	bool bNewValue;
	template< typename F>
	Data& Update( F operation )
	{
		T previous = data;
		operation();
		bNewValue = ( previous != data );
		return *this;
	}
};

class Chip8
{
public:
	Chip8();

	class KeyAccess
	{
		friend class Chip8;
		friend class Chip8_Debugger;
		friend int main( int argc,char** argv );
		KeyAccess(){}
	};

	void Init( KeyAccess key, const char* sROMToLoad );
	void EmulateCycle( KeyAccess key, const std::chrono::steady_clock::time_point& time );
	void AskForState( KeyAccess key,RunningState oState ) const;

	const Data< uint16_t>* GetStack() const { return stack; }
	const Data< uint8_t>* GetMemory() const { return memory; }
	const Data< uint8_t>* GetRegisters() const { return registers; }
	const std::deque<std::string>& GetHistoryOpcode() const { return m_aOpcodeHistory; }

	const Data< uint16_t> GetI() const { return I; }
	const Data< uint16_t> GetPC() const { return PC; }

	const Data< uint8_t> GetSP() const { return SP; }
	const Data< uint8_t> GetDelayTimer() const { return delay_timer; }
	const Data< uint8_t> GetSoundTimer() const { return sound_timer; }

	bool		IsPause() const { return m_oState == RunningState::Pause; }
	bool		IsRunning() const { return m_oState == RunningState::Running; }

private:
	void _Reset( );
	void _LoadFont( );
	void _LoadROM( const char* sROMToLoad );

	void _FetchOpcode( uint16_t& opcode );
	void _DecodeExecute_Opcode( const uint16_t opcode );
	void _UpdateTimers();
	void _AddOpcodeToHistory( const char* pOpcode );

	Data<uint8_t> memory[ 0x1000 ];
	Data<uint8_t> registers[ 0x10 ];
	Data<uint16_t> I; //Address register
	Data<uint16_t> stack[ 0x10 ];
	Data<uint8_t> SP; //Stack pointer
	Data<uint16_t> PC; //Program counter

	uint16_t m_iLastOpcode;
	uint8_t countBeforeStop;

	Data<uint8_t> delay_timer;
	Data<uint8_t> sound_timer;

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

	std::deque<std::string>						m_aOpcodeHistory;

	mutable RunningState						m_oState;
	std::chrono::steady_clock::time_point		lastTimeUpdate;
	std::chrono::steady_clock::time_point		lastTimeUpdateTimers;
	float										accumulator;
	const char*									m_sCurrentRomLoaded;
};