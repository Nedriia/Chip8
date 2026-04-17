#include "Chip8.h"
#include <fstream>
#include <chrono>
#include <assert.h>
#include "Display.h"
#include <iostream>
#include "Input.h"
#include "SoundManager.h"
#include <string.h>

#define START_FONT_MEMORY_ADDRESS 0x050
#define START_ROM_MEMORY_ADDRESS 0x200
#define MEMORY_SIZE 4096
#define DEFAULT_PARENT_ROM_FOLDER "..\\Roms\\"
#define JMPCHECK_BEFORE_ENDING 4
#define USE_SWITCH_BRANCH 1

int Chip8::m_iInstructionsPerFrame = 5000000;
//int Chip8::m_iInstructionsPerFrame = 5;

Chip8* Chip8::m_pSingleton = nullptr;

void Chip8::SetRomToLoad( const KeyAccess& oKey,const std::string& sSrc )
{
	delete[] m_sCurrentRomLoaded;

	size_t iSize = sSrc.length() + 1;
	char* sDest = new char[ iSize ];
	strcpy_s( sDest,iSize,sSrc.c_str() );

	m_sCurrentRomLoaded = sDest;
}

Chip8::Chip8() :
	m_iLastOpcode( 0 )
	,m_iCountBeforeStop( 0 )
	,m_iCurrentOpcode( 0 )
#ifdef DEBUG_INFO
	,m_oState( RunningState::Pause )
#else
	,m_oState( RunningState::Running )
#endif
	,m_iLastTimeUpdate( std::chrono::steady_clock::now() )
	,m_sCurrentRomLoaded( nullptr )
	,m_iCycle( 0 )
	,m_iPreviousKeyPressed( 0xFF )
#ifdef QUIRK_DISPWAIT
	,m_iTimeLastFrame{}
#endif
	, m_pInputInstance( nullptr )
	,m_pSoundManagerInstance( nullptr )
{
	m_aMainTable[ 0x0 ] = { &Chip8::x0_Dispatch };
	m_aMainTable[ 0x1 ] = { &Chip8::JMP };
	m_aMainTable[ 0x2 ] = { &Chip8::CALL };
	m_aMainTable[ 0x3 ] = { &Chip8::SE_VX_NN };
	m_aMainTable[ 0x4 ] = { &Chip8::SNE_VX_NN };
	m_aMainTable[ 0x5 ] = { &Chip8::SE_VX_VY };
	m_aMainTable[ 0x6 ] = { &Chip8::LD_VX_NN };
	m_aMainTable[ 0x7 ] = { &Chip8::ADD_VX_NN };
	m_aMainTable[ 0x8 ] = { &Chip8::x8_Dispatch };
	m_aMainTable[ 0x9 ] = { &Chip8::SNE_VX_VY };
	m_aMainTable[ 0xA ] = { &Chip8::LD_I_NNN };
	m_aMainTable[ 0xB ] = { &Chip8::JMP_NNN };
	m_aMainTable[ 0xC ] = { &Chip8::RND };
	m_aMainTable[ 0xD ] = { &Chip8::DRAW };
	m_aMainTable[ 0xE ] = { &Chip8::xE_Dispatch };
	m_aMainTable[ 0xF ] = { &Chip8::xF_Dispatch };

	//0x0000
	m_a0x0_Table[ 0x0 ] = { &Chip8::CLS };
	m_a0x0_Table[ 0xE ] = { &Chip8::RET };

	//0x8000
	m_a0x8_Table[ 0x0 ] = { &Chip8::LD_VX_VY };
	m_a0x8_Table[ 0x1 ] = { &Chip8::OR };
	m_a0x8_Table[ 0x2 ] = { &Chip8::AND };
	m_a0x8_Table[ 0x3 ] = { &Chip8::XOR };
	m_a0x8_Table[ 0x4 ] = { &Chip8::ADD_VX_VY };
	m_a0x8_Table[ 0x5 ] = { &Chip8::SUB_VX_VY };
	m_a0x8_Table[ 0x6 ] = { &Chip8::SHR };
	m_a0x8_Table[ 0x7 ] = { &Chip8::SUBN_VX_VY };
	m_a0x8_Table[ 0xE ] = { &Chip8::SHL };

	//0xE000
	m_a0xE_Table[ 0x9 ] = { &Chip8::SKP };
	m_a0xE_Table[ 0xA ] = { &Chip8::SKNP };

	//0xF000 - size 0xFF
	m_a0xF_Table[ 0x7 ] = { &Chip8::LD_VX_DT };
	m_a0xF_Table[ 0x0A ] = { &Chip8::LD_VX_KEY };
	m_a0xF_Table[ 0x15 ] = { &Chip8::LD_DT_VX };
	m_a0xF_Table[ 0x18 ] = { &Chip8::LD_ST_VX };
	m_a0xF_Table[ 0x1E ] = { &Chip8::ADD_I_VX };
	m_a0xF_Table[ 0x29 ] = { &Chip8::LD_I_FONT };
	m_a0xF_Table[ 0x33 ] = { &Chip8::BCD };
	m_a0xF_Table[ 0x55 ] = { &Chip8::LD_I_VX };
	m_a0xF_Table[ 0x65 ] = { &Chip8::LD_VX_I };
}

Chip8::~Chip8()
{
	m_pSingleton = nullptr;
}

void Chip8::Init( const KeyAccess& key,const char* sROMToLoad )
{
	unsigned timeSeed = std::chrono::steady_clock::now().time_since_epoch().count();
	m_iRng.seed( timeSeed );

	_LoadFont();
	_LoadROM( sROMToLoad );

	m_iPC = START_ROM_MEMORY_ADDRESS;
	m_iSP = 0;
}

void Chip8::_Reset()
{
	if( m_sCurrentRomLoaded == nullptr )
		return;

	m_iI.clear();
	m_iDelay_timer.clear();
	m_iSound_timer.clear();
	m_aOpcodeHistory.clear();
	m_iLastOpcode = 0;
	m_iCycle = 0;
	m_iCurrentOpcode = 0;

	Display::KeyDisplayAccess oKeyDisplay;
	Display::ClearScreen( oKeyDisplay );

	for( Data<uint8_t>* it = &m_aMemory[ 0 ]; it != &m_aMemory[ 0x1000 ]; ++it )
		it->clear();
	for( Data<uint8_t>* it = &m_aRegisters[ 0 ]; it != &m_aRegisters[ 0x10 ]; ++it )
		it->clear();
	for( Data<uint16_t>* it = &m_aStack[ 0 ]; it != &m_aStack[ 0x10 ]; ++it )
		it->clear();

	Chip8::KeyAccess oKey;
	Init( oKey,m_sCurrentRomLoaded );
}

void Chip8::_LoadFont()
{
	for( uint8_t i = 0; i < 80; ++i )
	{
		m_aMemory[ START_FONT_MEMORY_ADDRESS + i ] = m_aFontset[ i ];
	}
}

void Chip8::_LoadROM( const char* sROMToLoad )
{
	if( sROMToLoad == nullptr )
	{
		m_oState = RunningState::Pause;
		return;
	}

	std::string sPath = sROMToLoad;
	if( std::strchr( sROMToLoad,'\\' ) == nullptr )
		sPath = std::string( DEFAULT_PARENT_ROM_FOLDER ) + sROMToLoad;

	std::ifstream file( sPath,std::ios::binary | std::ios::in | std::ios::ate );
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
			m_aMemory[ START_ROM_MEMORY_ADDRESS + i ] = static_cast< uint8_t >( memblock[ i ] );
		}

		delete[] memblock;

		KeyAccess oKey;
		SetRomToLoad( oKey,sROMToLoad );
	}
	else
	{
		std::cerr << "ERROR::CHIP8::LOADING::FILE_NOT_FOUND " << sROMToLoad << std::endl;
	}
}

void Chip8::EmulateCycle( const KeyAccess& key,const std::chrono::steady_clock::time_point& time )
{
	if( m_oState == RunningState::Reset )
	{
		_Reset();
		m_oState = RunningState::Pause;
	}

	if( m_oState == RunningState::Pause )
	{
		m_iLastTimeUpdate = time;
		return;
	}

	bool bForceNextStep = false;
	if( m_oState == RunningState::StepNextFrame )
	{
		m_oState = RunningState::Pause;
		bForceNextStep = true;
	}

	std::chrono::microseconds elapsed = std::chrono::duration_cast< std::chrono::microseconds >( time - m_iLastTimeUpdate );
	if( elapsed >= Display::GetRefreshTick() || bForceNextStep )
	{
		if( m_pInputInstance == nullptr )
			m_pInputInstance = Input::GetInstance();
		if( m_pSoundManagerInstance == nullptr )
			m_pSoundManagerInstance = SoundManager::GetInstance();

		for( int i = 0; i < m_iInstructionsPerFrame; ++i )
		{
			_FetchOpcode();
			_DecodeExecute_Opcode();

			if( _IsEndReached() || bForceNextStep )
				break;
		}

		_UpdateTimers();
		m_pSoundManagerInstance->Manage( ( uint8_t )m_iSound_timer );

		m_iLastTimeUpdate += Display::GetRefreshTick();
	}
}

void Chip8::AskForState( const KeyAccess& key,RunningState oState ) const
{
	m_oState = oState;
}

void Chip8::DestroyCpu()
{
	delete[] m_sCurrentRomLoaded;
	delete m_pSingleton;
}

void Chip8::_FetchOpcode()
{
	m_iCurrentOpcode = m_aMemory[ m_iPC ];
	uint8_t byte2 = m_aMemory[ m_iPC + 1 ];

	m_iCurrentOpcode <<= 8;
	m_iCurrentOpcode |= byte2;

	m_iPC += 2;
}

void Chip8::_DecodeExecute_Opcode()
{
	if( !USE_SWITCH_BRANCH )
	{
		uint8_t iIndex = ( m_iCurrentOpcode & 0xF000 ) >> 12;
		CheckOpcodeAndExec( iIndex,m_aMainTable );
	}
	else
	{
		uint16_t opcodeNibble = m_iCurrentOpcode & 0xF000;
		switch( opcodeNibble )
		{
			case 0x0000:
			{
				uint16_t check = m_iCurrentOpcode & 0x00FF;
				if( check == 0xE0 )
					CLS();
				else if( check == 0xEE )
					RET();
				else
					std::cerr << "ERROR::OPCODE_UNKNOWN_" << std::hex << m_iCurrentOpcode << std::endl;
			}
			break;
			case 0x1000:
				JMP();
			break;
			case 0x2000:
				CALL();
			break;
			case 0x3000:
				SE_VX_NN();
			break;
			case 0x4000:
				SNE_VX_NN();
			break;
			case 0x5000:
				SE_VX_VY();
			break;
			case 0x6000:
				LD_VX_NN();
			break;
			case 0x7000:
				ADD_VX_NN();
			break;
			case 0x8000:
			{
				switch( m_iCurrentOpcode & 0x000F )
				{
					case 0:
						LD_VX_VY();
					break;
					case 1:
						OR();
					break;
					case 2:
						AND();
					break;
					case 3:
						XOR();
					break;
					case 4:
						ADD_VX_VY();
					break;
					case 5:
						SUB_VX_VY();
					break;
					case 6:
						SHR();
					break;
					case 7:
						SUBN_VX_VY();
					break;
					case 0xE:
						SHL();
					break;
					default:
						std::cerr << "ERROR::OPCODE_UNKNOWN_" << std::hex << m_iCurrentOpcode << std::endl;
						break;
				}
			}
			break;
			case 0x9000:
				SNE_VX_VY();
			break;
			case 0xA000:
				LD_I_NNN();
			break;
			case 0xB000:
				JMP_NNN();
			break;
			case 0xC000:
				RND();
			break;
			case 0xD000:
				DRAW();
			break;
			case 0xE000:
			{
				uint16_t check = m_iCurrentOpcode & 0x00FF;
				if( check == 0x9E )
					SKP();
				else if( check == 0xA1 )
					SKNP();
				else
					std::cerr << "ERROR::OPCODE_UNKNOWN_" << std::hex << m_iCurrentOpcode << std::endl;
			}
			break;
			case 0xF000:
			{
				switch( m_iCurrentOpcode & 0x00FF )
				{
					case 7:
						LD_VX_DT();
					break;
					case 0x0A:
						LD_VX_KEY();
					break;
					case 0x15:
						LD_DT_VX();
					break;
					case 0x18:
						LD_ST_VX();
					break;
					case 0x1E:
						ADD_I_VX();
					break;
					case 0x29:
						LD_I_FONT();
					break;
					case 0x33:
						BCD();
					break;
					case 0x55:
						LD_I_VX();
					break;
					case 0x65:
						LD_VX_I();
					break;
					default:
						std::cerr << "ERROR::OPCODE_UNKNOWN_" << std::hex << m_iCurrentOpcode << std::endl;
						break;
				}
			}
			break;
			default:
				std::cerr << "ERROR::OPCODE_UNKNOWN_" << std::hex << m_iCurrentOpcode << std::endl;
			break;
		}
	}
}

void Chip8::_UpdateTimers()
{
	if( m_iDelay_timer > 0 )
		--m_iDelay_timer;

	if( m_iSound_timer > 0 )
		--m_iSound_timer;
}

#ifdef DEBUG_INFO
void Chip8::_AddOpcodeToHistory( const char* pOpcode )
{
	//https://github.com/trapexit/chip-8_documentation
	if( m_aOpcodeHistory.size() >= 0x40 )
		m_aOpcodeHistory.pop_front();

	m_aOpcodeHistory.push_back( pOpcode );
	++m_iCycle;
}
#endif

bool Chip8::_IsEndReached()
{
	if( m_iCurrentOpcode == m_iLastOpcode && ( m_iCurrentOpcode & 0xF000 ) == 0x1000 )
	{
		++m_iCountBeforeStop;
		if( m_iCountBeforeStop >= JMPCHECK_BEFORE_ENDING )
		{
			m_oState = RunningState::Pause;
			return true;
		}
	}
	else
	{
		m_iLastOpcode = m_iCurrentOpcode;
		m_iCountBeforeStop = 0;
	}

	return false;
}

inline void Chip8::x0_Dispatch()
{
	uint8_t iLastNibble = m_iCurrentOpcode & 0x000F;
	CheckOpcodeAndExec( iLastNibble,m_a0x0_Table );
}

inline void Chip8::x8_Dispatch()
{
	uint8_t iLastNibble = m_iCurrentOpcode & 0x000F;
	CheckOpcodeAndExec( iLastNibble,m_a0x8_Table );
}

inline void Chip8::xE_Dispatch()
{
	uint8_t iThirdNibble = ( m_iCurrentOpcode & 0x00F0 ) >> 4;
	CheckOpcodeAndExec( iThirdNibble,m_a0xE_Table );
}

inline void Chip8::xF_Dispatch()
{
	uint16_t iLastNibbles = m_iCurrentOpcode & 0x00FF;
	CheckOpcodeAndExec( iLastNibbles,m_a0xF_Table );
}

template< typename T,size_t N >
inline void Chip8::CheckOpcodeAndExec( const T iNibble,const std::array<fct_opcode,N>& aTable )
{
	if( aTable[ iNibble ] == nullptr || iNibble < 0 || iNibble >= aTable.size() )
		std::cerr << "ERROR::OPCODE_UNKNOWN_" << std::hex << m_iCurrentOpcode << std::endl;
	else
		( this->*aTable[ iNibble ] )();
}

inline void Chip8::CLS()
{
	Display::KeyDisplayAccess oKeyDisplay;
	Display::ClearScreen( oKeyDisplay );

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X}  : CLS",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::RET()
{
	//Returns from a subroutine
	m_iPC = m_aStack[ m_iSP ];
	m_aStack[ m_iSP ] = 0;
	if( m_iSP != 0 )
		--m_iSP;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : RET",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::JMP()
{
	//Jumps to address NNN
	m_iPC = GetNNN();

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X}  : JMP",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::JMP_NNN()
{
#ifdef QUIRK_JUMPING
	//Jumps to the address NNN plus V0
	#ifdef DEBUF_INFO
		m_sOpcodeInstruct = std::format( "{:04X} : JMP V0, NNN",m_iCurrentOpcode );
	#endif //DEBUG_INFO
	m_iPC = GetNNN() + m_aRegisters[ 0 ];
#else // QUIRK_JUMPING

	#ifdef DEBUG_INFO
		m_sOpcodeInstruct = std::format( "{:04X} : JMP VX",m_iCurrentOpcode );
	#endif //DEBUG_INFO
	m_iPC = GetNNN() + m_aRegisters[ GetX() ];
#endif //QUIRK_JUMPING

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( m_sOpcodeInstruct.c_str() );
#endif
}

inline void Chip8::CALL()
{
	if( m_aStack[ 0 ] != 0 ) //could be simplier to use int and init the value to -1 but I want to keep 0 as start
		++m_iSP;

	//Calls subroutine at NNN
	m_aStack[ m_iSP ] = m_iPC;
	m_iPC = GetNNN();

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : CALL",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::SNE_VX_NN()
{
	//Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
	if( m_aRegisters[ GetX() ] != GetNN() )
		m_iPC += 2;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : SNE VX, NN",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::SNE_VX_VY()
{
	//Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block)
	if( m_aRegisters[ GetX() ] != m_aRegisters[ GetY() ] )
		m_iPC += 2;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : SNE VX, VY",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::SE_VX_NN()
{
	//Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block)
	if( m_aRegisters[ GetX() ] == GetNN() )
		m_iPC += 2;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : SNE VX, NN",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::SE_VX_VY()
{
	//Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
	if( m_aRegisters[ GetX() ] == m_aRegisters[ GetY() ] )
		m_iPC += 2;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : SE VX, VY",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_VX_NN()
{
	//Sets NN To VX
	m_aRegisters[ GetX() ] = GetNN();

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, NN",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_VX_VY()
{
	//Sets VX to the value of VY.
	m_aRegisters[ GetX() ] = m_aRegisters[ GetY() ];

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, VY",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_I_NNN()
{
	//Sets I to the address NNN
	m_iI = GetNNN();

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD I, NNN",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_VX_DT()
{
	//Sets VX to the value of the delay timer.
	m_aRegisters[ GetX() ] = m_iDelay_timer;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, DT",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_DT_VX()
{
	//Sets the delay timer to VX
	m_iDelay_timer = m_aRegisters[ GetX() ];

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD DT, VX",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_VX_KEY()
{
	//A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event, delay and sound timers should continue processing)
	if( m_pInputInstance )
	{
		if( m_iPreviousKeyPressed != 0xFF && m_pInputInstance->GetKeyState( m_iPreviousKeyPressed ) == 0 )//Previous Input has been released
		{
			m_aRegisters[ GetX() ] = m_iPreviousKeyPressed;
			m_iPreviousKeyPressed = 0xFF;
		}
		else
		{
			m_iPreviousKeyPressed = m_pInputInstance->IsAnyKeyPress();
			m_iPC -= 2;
		}
	}
	else
		m_iPC -= 2;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, KEY",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_ST_VX()
{
	//Sets the sound timer to VX
	m_iSound_timer = m_aRegisters[ GetX() ];

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD ST, VX",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_I_FONT()
{
	//Sets I to the location of the sprite for the character in VX(only consider the lowest nibble). Characters 0-F (in hexadecimal) are represented by a 4x5 font.
	m_iI = START_FONT_MEMORY_ADDRESS + ( m_aRegisters[ GetX() ] * 5 );

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD I, FONT( VX )",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_I_VX()
{
	//Stores from V0 to VX (including VX) in memory, starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified
	for( int i = 0; i <= GetX(); ++i )
	{
#ifdef QUIRK_MEMORY
		m_aMemory[ m_iI ] = m_aRegisters[ i ];
		++m_iI;
#else
		m_aMemory[ m_iI + i ] = m_aRegisters[ i ];
#endif
	}

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD [I], VX",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::LD_VX_I()
{
	//Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified
	for( int i = 0; i <= GetX(); ++i )
	{
#ifdef QUIRK_MEMORY
		m_aRegisters[ i ] = m_aMemory[ m_iI ];
		++m_iI;
#else
		m_aRegisters[ i ] = m_aMemory[ m_iI + i ];
#endif
	}

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, [I]",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::ADD_VX_NN()
{
	//Adds NN to VX (carry flag is not changed).
	m_aRegisters[ GetX() ] += GetNN();

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : ADD VX, NN",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::ADD_VX_VY()
{
	uint8_t X = GetX();
	//Adds VY to VX
	uint16_t sum = m_aRegisters[ X ] + m_aRegisters[ GetY() ];
	m_aRegisters[ X ] = sum & 0xFF;
	m_aRegisters[ 15 ] = ( sum > 0xFF ) ? 1 : 0;
	//VF is set to 1 when there's an overflow, and to 0 when there is not.

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : ADD VX, VY ( VF )",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::ADD_I_VX()
{
	//Adds VX to I. VF is not affected
	m_iI += m_aRegisters[ GetX() ];

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : ADD I, VX",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::SUB_VX_VY()
{
	uint8_t X = GetX();

	//VY is subtracted from VX.
	uint16_t diff = m_aRegisters[ X ] - m_aRegisters[ GetY() ];
	m_aRegisters[ X ] = diff & 0xFF;
	m_aRegisters[ 15 ] = 1;
	if( diff > 0xFF )
		m_aRegisters[ 15 ] = 0;
	// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : SUB VX, VY ( VF )",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::SHR()
{
	uint8_t X = GetX();
	uint8_t Y = GetY();

	//If the least - significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
	uint8_t LSB = 0;

#ifdef QUIRK_SHIFTING
	#ifdef DEBUF_INFO
		m_sOpcodeInstruct = std::format( "{:04X} : SHR VX, VY",m_iCurrentOpcode );
	#endif //DEBUF_INFO

	LSB = ( m_aRegisters[ Y ] & 0x01 );

	m_aRegisters[ X ] = m_aRegisters[ Y ] >> 1; //before 1990
#else // QUIRK_SHIFTING
	#ifdef DEBUG_INFO
		m_sOpcodeInstruct = std::format( "{:04X} : SHR VX",m_iCurrentOpcode );
	#endif

	LSB = ( m_aRegisters[ X ] & 0x01 );

	m_aRegisters[ X ] >>= 1;
#endif

	m_aRegisters[ 15 ] = LSB;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( m_sOpcodeInstruct.c_str() );
#endif
}

inline void Chip8::SUBN_VX_VY()
{
	uint8_t X = GetX();

	//Sets VX equals to VY minus VX.
	uint16_t diff = m_aRegisters[ GetY() ] - m_aRegisters[X];
	m_aRegisters[ X ] = diff & 0xFF;
	m_aRegisters[ 15 ] = 1;
	if( diff > 0xFF )
		m_aRegisters[ 15 ] = 0;
	// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX).

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : SUBN VX, VY",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::SHL()
{
	uint8_t MSB = 0;
#ifdef QUIRK_SHIFTING
	#ifdef DEBUG_INFO//DEBUG_INFO
		m_sOpcodeInstruct = std::format( "{:04X} : SHL VX, VY",m_iCurrentOpcode );
	#endif //DEBUG_INFO

	uint8_t Y = GetY();
	// If the most-significant bit of Vy is 1.
	MSB = ( m_aRegisters[ Y ] & 0x80 ) == 0x80 ? 1 : 0;

	m_aRegisters[ GetX() ] = m_aRegisters[Y] << 1; //before 1990
#else // QUIRK_SHIFTING
	uint8_t X = GetX();
	#ifdef DEBUG_INFO//DEBUG_INFO
		m_sOpcodeInstruct = std::format( "{:04X} : SHL VX",m_iCurrentOpcode );
	#endif //DEBUG_INFO

	// If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
	MSB = ( m_aRegisters[ X ] & 0x80 ) == 0x80 ? 1 : 0;

	m_aRegisters[ X ] <<= 1;
#endif
	m_aRegisters[ 15 ] = MSB;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( m_sOpcodeInstruct.c_str() );
#endif
}

inline void Chip8::RND()
{
	//Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
	std::uniform_int_distribution<std::mt19937::result_type> dist( 0,255 );
	m_aRegisters[ GetX() ] = dist(m_iRng) & GetNN();

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : RND VX, NN",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::DRAW()
{
#ifdef QUIRK_DISPWAIT
	#ifdef DEBUG_INFO
		m_sOpcodeInstruct = std::format( "{:04X} : DRW VX, VY VBlank",m_iCurrentOpcode );
	#endif //DEBUG_INFO

	//VBlank, waiting for next frame
	if( m_iTimeLastFrame.time_since_epoch().count() == 0 )
	{
		m_iTimeLastFrame = m_iLastTimeUpdate;
		m_iPC -= 2;
		return;
	}
	if( m_iLastTimeUpdate >= ( m_iTimeLastFrame + Display::GetRefreshTick() ) )
	{
		m_iTimeLastFrame = {};
	}
	else
	{
		m_iPC -= 2;
		return;
	}
#else
	#ifdef DEBUG_INFO
		m_sOpcodeInstruct = std::format( "{:04X} : DRW VX, VY",m_iCurrentOpcode );
	#endif //DEBUG_INFO
#endif //QUIRK_DISPWAIT

	/*Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
	The interpreter reads N bytes from memory, starting at the address stored in I.
	These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). Sprites are XORed onto the existing screen.
	If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0.
	If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen
	I value does not change after the execution of this instruction*/
	bool bErased = false;
	uint8_t iYOffset = 0;

	Display::KeyDisplayAccess oKeyDisplay;
	uint8_t xPos = m_aRegisters[ GetX() ] & ( Display::GetWidth() - 1 );
	uint8_t yPos = m_aRegisters[ GetY() ] & ( Display::GetHeight() - 1 );

	m_aRegisters[ 15 ] = 0;
	for( ; iYOffset < GetN(); ++iYOffset )
	{
#ifdef QUIRK_CLIPPING
		Display::DrawPixelAtPos( oKeyDisplay,xPos,yPos + iYOffset,m_aMemory[ m_iI + iYOffset ],bErased,true );
#else
		Display::DrawPixelAtPos( oKeyDisplay,xPos,yPos + iYOffset,m_aMemory[ m_iI + iYOffset ],bErased,false );
#endif
		if( bErased )
			m_aRegisters[ 15 ] = 1;
	}

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( m_sOpcodeInstruct.c_str() );
#endif
}

inline void Chip8::SKP()
{
	//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is pressed (usually the next instruction is a jump to skip a code block).
	if( m_pInputInstance->GetKeyState( m_aRegisters[ GetX() ]) )
		m_iPC += 2;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : SKP",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::SKNP()
{
	//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed (usually the next instruction is a jump to skip a code block).
	if( !m_pInputInstance->GetKeyState( m_aRegisters[ GetX() ]) )
		m_iPC += 2;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : SKNP",m_iCurrentOpcode ).c_str() );
#endif
}

inline const uint8_t Chip8::GetX()
{
	return ( m_iCurrentOpcode & 0x0F00 ) >> 8;
}

inline const uint8_t Chip8::GetY()
{
	return ( m_iCurrentOpcode & 0x00F0 ) >> 4;
}

inline const uint16_t Chip8::GetNNN()
{
	return m_iCurrentOpcode & 0x0FFF;
}

inline const uint8_t Chip8::GetNN()
{
	return m_iCurrentOpcode & 0x00FF;
}

inline const uint8_t Chip8::GetN()
{
	return  m_iCurrentOpcode & 0x000F;
}

inline void Chip8::BCD()
{
	uint8_t X = GetX();

	//Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2
	m_aMemory[ m_iI ] = m_aRegisters[ X ] / 100;
	m_aMemory[ m_iI + 1 ] = ( m_aRegisters[ X ] / 10 ) % 10;
	m_aMemory[ m_iI + 2 ] = ( m_aRegisters[ X ] % 100 ) % 10;

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : BCD",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::OR()
{
	//Sets VX to VX or VY. (bitwise OR operation)
	m_aRegisters[ GetX()] |= m_aRegisters[ GetY() ];
#ifdef QUIRK_VFRESET
	m_aRegisters[ 15 ] = 0;
#endif

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : OR VX, VY",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::AND()
{
	//Sets VX to VX and VY. (bitwise AND operation)
	m_aRegisters[ GetX() ] &= m_aRegisters[ GetY() ];
#ifdef QUIRK_VFRESET
	m_aRegisters[ 15 ] = 0;
#endif

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : AND VX, VY",m_iCurrentOpcode ).c_str() );
#endif
}

inline void Chip8::XOR()
{
	//Sets VX to VX xor VY
	m_aRegisters[ GetX() ] ^= m_aRegisters[ GetY() ];
#ifdef QUIRK_VFRESET
	m_aRegisters[ 15 ] = 0;
#endif

#ifdef DEBUG_INFO
	_AddOpcodeToHistory( std::format( "{:04X} : XOR VX, VY",m_iCurrentOpcode ).c_str() );
#endif
}