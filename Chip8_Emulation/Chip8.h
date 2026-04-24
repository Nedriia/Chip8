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
		//#define QUIRK_DISPWAIT
		#define QUIRK_CLIPPING
		#define QUIRK_SHIFTING
		#define QUIRK_JUMPING
	#endif
#endif

template< typename T>
struct alignas ( 4 ) Data
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
#ifdef DEBUG_INFO
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
#endif
	bool IsNULL() const { return oData == 0; }
	void clear() { oData = 0; m_bNewValue = false; }

private:
	T oData;
	mutable bool m_bNewValue;
	mutable bool m_bDirty;
	template< typename F>
	Data& Update( F operation )
	{
#ifdef DEBUG_INFO
		T previous = oData;
#endif
		operation();
#ifdef DEBUG_INFO
		m_bNewValue = ( previous != oData );
		if( m_bNewValue )
			m_bDirty = false;
#endif
		return *this;
	}
};

class alignas( 16 ) Chip8
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

	const std::array<Data< uint8_t>,4096>*	GetMemory() const { return &m_aMemory; }

	const Data< uint16_t>*			GetStack() const { return m_aStack; }
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

	void _FetchDecode_Opcode();
	void _UpdateTimers();
#ifdef DEBUG_INFO
	void _AddOpcodeToHistory( const char* pOpcode );
#endif
	bool _IsEndReached();

	std::array< Data<uint8_t>, 4096 > m_aMemory;
	Data<uint8_t> m_aRegisters[ 16 ];
	Data<uint16_t> m_iI; //Address register
	Data<uint16_t> m_aStack[ 16 ];
	Data<uint8_t> m_iSP; //Stack pointer
	Data<uint16_t> m_iPC; //Program counter
	uint16_t m_iCurrentOpcode;

	uint16_t m_iLastOpcode;
	int		m_iCycle;
	mutable RunningState						m_oState;

	Data<uint8_t> m_iDelay_timer;
	Data<uint8_t> m_iSound_timer;

	uint8_t m_aFontset[ 80 ] =
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
	uint8_t m_iCountBeforeStop;
	uint8_t										m_iPreviousKeyPressed;

	std::deque<std::string>						m_aOpcodeHistory;

	std::chrono::steady_clock::time_point		m_iLastTimeUpdate;
	const char*									m_sCurrentRomLoaded;//Don't set that without SetRomToLoad function

#ifdef QUIRK_DISPWAIT
	std::chrono::steady_clock::time_point		m_iTimeLastFrame;
#endif

	static Chip8*								m_pSingleton;
	static  int									m_iInstructionsPerFrame;

	typedef void ( Chip8::*fct_opcode )();
	//Opcodes array
	std::array< fct_opcode, 16 > m_aMainTable;
	std::array< fct_opcode, 16 > m_a0x8_Table;
	std::array< fct_opcode, 16 > m_a0xF_Table;

	//Functions
	inline void x0_Dispatch();
	inline void x8_Dispatch();
	inline void xE_Dispatch();
	inline void xF_Dispatch();
	template< typename T, size_t N >
	inline void CheckOpcodeAndExec( const T iNibble, const std::array<fct_opcode,N>& aTable );

	//Opcodes
	inline void CLS();
	inline void RET();
	inline void CALL();

	inline void JMP();
	inline void JMP_NNN();

	inline void SNE_VX_NN();
	inline void SNE_VX_VY();

	inline void SE_VX_NN();
	inline void SE_VX_VY();

	inline void LD_VX_NN();
	inline void LD_VX_VY();
	inline void LD_I_NNN();
	inline void LD_VX_DT();
	inline void LD_DT_VX();
	inline void LD_VX_KEY();
	inline void LD_ST_VX();
	inline void LD_I_FONT();
	inline void LD_I_VX();
	inline void LD_VX_I();

	inline void ADD_VX_NN();
	inline void ADD_VX_VY();
	inline void ADD_I_VX();

	inline void SUB_VX_VY();
	inline void SUBN_VX_VY();

	inline void OR();
	inline void AND();
	inline void XOR();
	inline void SHR();
	inline void SHL();
	inline void RND();
	inline void DRAW();
	inline void BCD();

	inline void SKP();
	inline void SKNP();

	inline const uint8_t GetX();
	inline const uint8_t GetY();
	inline const uint16_t GetNNN();
	inline const uint8_t GetNN();
	inline const uint8_t GetN();

#ifdef DEBUG_INFO
	std::string m_sOpcodeInstruct;
#endif

	const Input* m_pInputInstance;
	SoundManager* m_pSoundManagerInstance;
	static uint8_t iHexToIndex[];

	struct alignas ( 16 ) DecodedOpcode
	{
		fct_opcode fct = nullptr;
		uint16_t NNN = 0;
		uint8_t X = 0;
		uint8_t Y = 0;
		uint8_t NN = 0;
		uint8_t N = 0;
	};
	std::array<DecodedOpcode, 4096 > m_aMirorMemory;
	DecodedOpcode*			   m_pCurrentOpcode; 

	std::mt19937 m_iRng;
};