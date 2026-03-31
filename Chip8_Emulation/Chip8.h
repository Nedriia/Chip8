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

#define QUIRKS
#if defined( QUIRKS )
	/*#define QUIRK_VFRESET
	#define QUIRK_MEMORY
	#define QUIRK_DISPWAIT*/
	#define QUIRK_CLIPPING
	/*#define QUIRK_SHIFTING
	#define QUIRK_JUMPING*/
#endif

template< typename T>
struct Data
{
public:
	Data() { oData = 0; m_bNewValue = false; m_bDirty = false; }

	Data& operator=( const Data& rhs ) { return Update( [ & ] { oData = rhs.oData; } ); }
	Data& operator=( const T& rhs ) { return Update( [ & ] { oData = rhs; } ); }
	Data& operator+=( const T& rhs ) { return Update( [ & ] { oData += rhs; } ); }
	Data& operator-=( const T& rhs ) { return Update( [ & ] { oData -= rhs; } ); }
	Data& operator--() { return Update( [ & ] { --oData; } ); }
	Data& operator++() { return Update( [ & ] { ++oData; } ); }
	Data& operator|=( const T& rhs ) { return Update( [ & ] { oData |= rhs; } ); }
	Data& operator&=( const T& rhs ) { return Update( [ & ] { oData &= rhs; } ); }
	Data& operator^=( const T& rhs ) { return Update( [ & ] { oData ^= rhs; } ); }
	Data& operator>>=( const T& rhs ) { return Update( [ & ] { oData >>= rhs; } ); }
	Data& operator<<=( const T& rhs ) { return Update( [ & ] { oData <<= rhs; } ); }

	operator T() const { return oData; }
	bool HasChanged() const { return m_bNewValue; }
	void SetDataAsDirty() const
	{
		if( !m_bDirty )
			m_bDirty = true;
		else
		{
			m_bDirty = false;
			m_bNewValue = false;
		}
	}
	bool IsNULL() const { return oData == 0; }
	void clear() { oData = 0; m_bNewValue = false; }

private:
	T oData;
	mutable bool m_bNewValue;
	mutable bool m_bDirty;
	template< typename F>
	Data& Update( F operation )
	{
		T previous = oData;
		operation();
		m_bNewValue = ( previous != oData );
		if( m_bNewValue )
			m_bDirty = false;
		return *this;
	}
};

class Chip8
{
public:

	class KeyAccess
	{
		friend class Chip8;
		friend class Chip8_Debugger;
		friend int main( int argc,char** argv );
		KeyAccess() {}
	};

	static Chip8* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new Chip8;
		return m_pSingleton;
	}

	void							Init( const KeyAccess& oKey,const char* sROMToLoad );
	void							EmulateCycle( const KeyAccess& oKey,const std::chrono::steady_clock::time_point& iTime );
	void							AskForState( const KeyAccess& oKey,RunningState oState ) const;
	void							DestroyCpu();

	const Data< uint16_t>*			GetStack() const { return m_aStack; }
	const Data< uint8_t>*			GetMemory() const { return m_aMemory; }
	const Data< uint8_t>*			GetRegisters() const { return m_aRegisters; }
	const std::deque<std::string>&	GetHistoryOpcode() const { return m_aOpcodeHistory; }

	const Data< uint16_t>&			GetI() const { return m_iI; }
	const Data< uint16_t>&			GetPC() const { return m_iPC; }

	const Data< uint8_t>&			GetSP() const { return m_iSP; }
	const Data< uint8_t>&			GetDelayTimer() const { return m_iDelay_timer; }
	const Data< uint8_t>&			GetSoundTimer() const { return m_iSound_timer; }
	int								GetCycleId() const { return m_iCycle; }

	bool							IsPause() const { return m_oState == RunningState::Pause; }
	bool							IsRunning() const { return m_oState == RunningState::Running; }

	static const int				GetInstructPerFrame() { return Chip8::GetInstance()->m_iInstructionsPerFrame; }
	static void						SetInstructionPerFrame( const int iNewValue ) { Chip8::GetInstance()->m_iInstructionsPerFrame = iNewValue; }

private:

	Chip8();
	~Chip8();

	void _Reset();
	void _LoadFont();
	void _LoadROM( const char* sROMToLoad );

	void _FetchOpcode( uint16_t& iOpcode );
	void _DecodeExecute_Opcode( const uint16_t iOpcode );
	void _UpdateTimers();
	void _AddOpcodeToHistory( const char* pOpcode );

	Data<uint8_t> m_aMemory[ 0x1000 ];
	Data<uint8_t> m_aRegisters[ 0x10 ];
	Data<uint16_t> m_iI; //Address register
	Data<uint16_t> m_aStack[ 0x10 ];
	Data<uint8_t> m_iSP; //Stack pointer
	Data<uint16_t> m_iPC; //Program counter

	uint16_t m_iLastOpcode;
	uint8_t m_iCountBeforeStop;

	Data<uint8_t> m_iDelay_timer;
	Data<uint8_t> m_iSound_timer;

	std::mt19937 m_iRng;

	uint8_t m_aFontset[ 0x50 ] =
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
	std::chrono::steady_clock::time_point		m_iLastTimeUpdate;
	const char*									m_sCurrentRomLoaded;
	int											m_iCycle;
	uint8_t										m_iPreviousKeyPressed;

#ifdef QUIRK_DISPWAIT
	std::chrono::steady_clock::time_point		m_iTimeLastFrame;
#endif

	static Chip8* m_pSingleton;

	static  int						m_iInstructionsPerFrame;
};