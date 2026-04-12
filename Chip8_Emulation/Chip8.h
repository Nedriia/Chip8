#pragma once

#include <random>
#include <deque>
#include <chrono>
#include <array>
#include "SoundManager.h"
#include "Input.h"
#include "Display.h"

enum class RunningState
{
	Running,
	Pause,
	StepNextFrame,
	Reset
};

#ifndef QUIRKS
//#define QUIRKS
	#ifdef QUIRKS
		#define QUIRK_VFRESET
		#define QUIRK_MEMORY
		#define QUIRK_DISPWAIT
		#define QUIRK_CLIPPING
		#define QUIRK_SHIFTING
		#define QUIRK_JUMPING
	#endif
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

	static const int				GetInstructPerFrame() { return m_iInstructionsPerFrame; }
	static void						SetInstructionPerFrame( const int iNewValue ) { m_iInstructionsPerFrame = iNewValue; }

	const char*						GetCurrentRomLoaded() const { return m_sCurrentRomLoaded; }
	void							SetRomToLoad( const KeyAccess& oKey, const std::string& sSrc );

private:

	Chip8();
	~Chip8();

	void _Reset();
	void _LoadFont();
	void _LoadROM( const char* sROMToLoad );

	void _FetchOpcode();
	void _DecodeExecute_Opcode();
	void _UpdateTimers();
#ifdef DEBUG_INFO
	void _AddOpcodeToHistory( const char* pOpcode );
#endif
	bool _IsEndReached();

	Data<uint8_t> m_aMemory[ 0x1000 ];
	Data<uint8_t> m_aRegisters[ 0x10 ];
	Data<uint16_t> m_iI; //Address register
	Data<uint16_t> m_aStack[ 0x10 ];
	Data<uint8_t> m_iSP; //Stack pointer
	Data<uint16_t> m_iPC; //Program counter
	uint16_t m_iCurrentOpcode;

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
	const char*									m_sCurrentRomLoaded;//Don't set that without SetRomToLoad function
	int											m_iCycle;
	uint8_t										m_iPreviousKeyPressed;

#ifdef QUIRK_DISPWAIT
	std::chrono::steady_clock::time_point		m_iTimeLastFrame;
#endif

	static Chip8*								m_pSingleton;
	static  int									m_iInstructionsPerFrame;

	typedef void ( Chip8::*fct_opcode )();
	//Opcodes array
	std::array< fct_opcode, 0x10 > m_aMainTable;
	std::array< fct_opcode, 0x10 > m_a0x0_Table;
	std::array< fct_opcode, 0x10 > m_a0x8_Table;
	std::array< fct_opcode, 0x10 > m_a0xE_Table;
	std::array< fct_opcode, 0xFF > m_a0xF_Table;
	//Functions
	void x0_Dispatch();
	void x8_Dispatch();
	void xE_Dispatch();
	void xF_Dispatch();
	template< typename T, size_t N >
	void CheckOpcodeAndExec( const T iNibble, const std::array<fct_opcode,N>& aTable );

	//Opcodes
	void CLS();
	void RET();
	void CALL();

	void JMP();
	void JMP_NNN();

	void SNE_VX_NN();
	void SNE_VX_VY();

	void SE_VX_NN();
	void SE_VX_VY();

	void LD_VX_NN();
	void LD_VX_VY();
	void LD_I_NNN();
	void LD_VX_DT();
	void LD_DT_VX();
	void LD_VX_KEY();
	void LD_ST_VX();
	void LD_I_FONT();
	void LD_I_VX();
	void LD_VX_I();

	void ADD_VX_NN();
	void ADD_VX_VY();
	void ADD_I_VX();

	void SUB_VX_VY();
	void SUBN_VX_VY();

	void OR();
	void AND();
	void XOR();
	void SHR();
	void SHL();
	void RND();
	void DRAW();
	void BCD();

	void SKP();
	void SKNP();

#ifdef DEBUG_INFO
	std::string m_sOpcodeInstruct;
#endif

	const Input* m_pInputInstance;
	SoundManager* m_pSoundManagerInstance;
};