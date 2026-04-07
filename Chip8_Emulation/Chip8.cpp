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

int Chip8::m_iInstructionsPerFrame = 10;

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
	,m_oState( RunningState::Pause )
	,m_iLastTimeUpdate( std::chrono::steady_clock::now() )
	,m_sCurrentRomLoaded( nullptr )
	,m_iCycle( 0 )
	,m_iPreviousKeyPressed( 0xFF )
#ifdef QUIRK_DISPWAIT
	,m_iTimeLastFrame{}
#endif
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
	m_a0x8_Table[0x0] = { &Chip8::LD_VX_VY };
	m_a0x8_Table[0x1] = { &Chip8::OR };
	m_a0x8_Table[0x2] = { &Chip8::AND };
	m_a0x8_Table[0x3] = { &Chip8::XOR };
	m_a0x8_Table[0x4] = { &Chip8::ADD_VX_VY };
	m_a0x8_Table[0x5] = { &Chip8::SUB_VX_VY };
	m_a0x8_Table[0x6] = { &Chip8::SHR };
	m_a0x8_Table[0x7] = { &Chip8::SUBN_VX_VY };
	m_a0x8_Table[0xE] = { &Chip8::SHL };

	//0xE000
	m_a0xE_Table[ 0x9 ] = { &Chip8::SKP };
	m_a0xE_Table[ 0xA ] = { &Chip8::SKNP };

	//0xF000 - size 0xFF
	m_a0xF_Table[ 0x7 ]  = { &Chip8::LD_VX_DT };
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
		return;

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
		for( int i = 0; i < m_iInstructionsPerFrame; ++i )
		{
			uint16_t opcode = 0;

			_FetchOpcode( opcode );
			_DecodeExecute_Opcode( opcode );

			if( _IsEndReached( opcode ) || bForceNextStep )
				break;
		}

		_UpdateTimers();
		SoundManager::GetInstance()->Manage( ( uint8_t )m_iSound_timer );

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

void Chip8::_FetchOpcode( uint16_t& opcode )
{
	opcode = m_aMemory[ m_iPC ];
	uint8_t byte2 = m_aMemory[ m_iPC + 1 ];

	opcode = opcode << 8;
	opcode |= byte2;

	m_iPC += 2;
}

void Chip8::_DecodeExecute_Opcode( const uint16_t opcode )
{
	uint8_t iIndex = ( opcode & 0xF000 ) >> 12;
	uint8_t iRow = ( opcode & 0x000F );
	( this->*m_aMainTable[ iIndex ] )( opcode );
}

void Chip8::_UpdateTimers()
{
	if( m_iDelay_timer > 0 )
		--m_iDelay_timer;

	if( m_iSound_timer > 0 )
		--m_iSound_timer;
}

void Chip8::_AddOpcodeToHistory( const char* pOpcode )
{
#ifdef DEBUG_INFO
	//https://github.com/trapexit/chip-8_documentation
	if( m_aOpcodeHistory.size() >= 0x40 )
		m_aOpcodeHistory.pop_front();

	m_aOpcodeHistory.push_back( pOpcode );
	++m_iCycle;
#endif
}

bool Chip8::_IsEndReached( const uint16_t iOpcode )
{
	if( iOpcode == m_iLastOpcode && ( iOpcode & 0xF000 ) == 0x1000 )
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
		m_iLastOpcode = iOpcode;
		m_iCountBeforeStop = 0;
	}

	return false;
}

void Chip8::x0_Dispatch( const uint16_t opcode )
{
	uint8_t iLastNibble = opcode & 0x000F;
	CheckOpcodeAndExec( opcode, iLastNibble,m_a0x0_Table );
}

void Chip8::x8_Dispatch( const uint16_t opcode )
{
	uint8_t iLastNibble = opcode & 0x000F;
	CheckOpcodeAndExec( opcode,iLastNibble,m_a0x8_Table );
}

void Chip8::xE_Dispatch( const uint16_t opcode )
{
	uint8_t iThirdNibble = ( opcode & 0x00F0 ) >> 4;
	CheckOpcodeAndExec( opcode,iThirdNibble,m_a0xE_Table );
}

void Chip8::xF_Dispatch( const uint16_t opcode )
{
	uint16_t iLastNibbles = opcode & 0x00FF;
	CheckOpcodeAndExec( opcode,iLastNibbles,m_a0xF_Table );
}

template< typename T,size_t N >
void Chip8::CheckOpcodeAndExec( const uint16_t opcode,const T iNibble,const std::array<fct_opcode,N>& aTable )
{
	if( iNibble >= aTable.size() )
		std::cerr << "ERROR::TRY_TO_ACCESS_INDEX_BIGGER_THAN_TABLE_SIZE ( " << iNibble << " ) " << std::hex << opcode << std::endl;
	else if( aTable[ iNibble ] == nullptr )
		std::cerr << "ERROR::OPCODE_UNKNOWN_" << std::hex << opcode << std::endl;
	else
		( this->*aTable[ iNibble ] )( opcode );
}

void Chip8::CLS( uint16_t opcode )
{
	Display::KeyDisplayAccess oKeyDisplay;
	Display::ClearScreen( oKeyDisplay );
	
	_AddOpcodeToHistory( std::format( "{:04X}  : CLS",opcode ).c_str() );
}

void Chip8::RET( uint16_t opcode )
{
	//Returns from a subroutine
	m_iPC = m_aStack[ m_iSP ];
	m_aStack[ m_iSP ] = 0;
	if( m_iSP != 0 )
		--m_iSP;

	_AddOpcodeToHistory( std::format( "{:04X} : RET",opcode ).c_str() );
}

void Chip8::JMP( const uint16_t opcode )
{
	uint16_t NNN = opcode & 0x0FFF;

	//Jumps to address NNN
	m_iPC = NNN;

	_AddOpcodeToHistory( std::format( "{:04X}  : JMP",opcode ).c_str() );
}

void Chip8::JMP_NNN( const uint16_t opcode )
{
	std::string OpcodeInstruct;
	uint16_t NNN = opcode & 0x0FFF;
#ifdef QUIRK_JUMPING
	//Jumps to the address NNN plus V0
	OpcodeInstruct = std::format( "{:04X} : JMP V0, NNN",opcode );
	m_iPC = NNN + m_aRegisters[ 0 ];
#else // QUIRK_JUMPING
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	OpcodeInstruct = std::format( "{:04X} : JMP VX, NNN",opcode );
	m_iPC = NNN + m_aRegisters[ X ];
#endif
	_AddOpcodeToHistory( OpcodeInstruct.c_str() );
}

void Chip8::CALL( const uint16_t opcode )
{
	uint16_t NNN = opcode & 0x0FFF;

	if( m_aStack[ 0 ] != 0 ) //could be simplier to use int and init the value to -1 but I want to keep 0 as start
		++m_iSP;

	//Calls subroutine at NNN
	m_aStack[ m_iSP ] = m_iPC;
	m_iPC = NNN;

	_AddOpcodeToHistory( std::format( "{:04X} : CALL",opcode ).c_str() );
}

void Chip8::SNE_VX_NN( const uint16_t opcode )
{
	uint8_t NN = opcode & 0x00FF;
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
	if( m_aRegisters[ X ] != NN )
		m_iPC += 2;

	_AddOpcodeToHistory( std::format( "{:04X} : SNE VX, NN",opcode ).c_str() );
}

void Chip8::SNE_VX_VY( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;

	//Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block)
	if( m_aRegisters[ X ] != m_aRegisters[ Y ] )
		m_iPC += 2;

	_AddOpcodeToHistory( std::format( "{:04X} : SNE VX, VY",opcode ).c_str() );
}

void Chip8::SE_VX_NN( const uint16_t opcode )
{
	uint8_t NN = opcode & 0x00FF;
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block)
	if( m_aRegisters[ X ] == NN )
		m_iPC += 2;

	_AddOpcodeToHistory( std::format( "{:04X} : SNE VX, NN",opcode ).c_str() );
}

void Chip8::SE_VX_VY( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;

	//Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
	if( m_aRegisters[ X ] == m_aRegisters[ Y ] )
		m_iPC += 2;

	_AddOpcodeToHistory( std::format( "{:04X} : SE VX, VY",opcode ).c_str() );
}

void Chip8::LD_VX_NN( const uint16_t opcode )
{
	uint8_t NN = opcode & 0x00FF;
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Sets NN To VX
	m_aRegisters[ X ] = NN;

	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, NN",opcode ).c_str() );
}

void Chip8::LD_VX_VY( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;

	//Sets VX to the value of VY.
	m_aRegisters[ X ] = m_aRegisters[ Y ];

	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, VY",opcode ).c_str() );
}

void Chip8::LD_I_NNN( const uint16_t opcode )
{
	uint16_t NNN = opcode & 0x0FFF;

	//Sets I to the address NNN
	m_iI = NNN;

	_AddOpcodeToHistory( std::format( "{:04X} : LD I, NNN",opcode ).c_str() );
}

void Chip8::LD_VX_DT( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Sets VX to the value of the delay timer.
	m_aRegisters[ X ] = m_iDelay_timer;

	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, DT",opcode ).c_str() );
}

void Chip8::LD_DT_VX( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Sets the delay timer to VX
	m_iDelay_timer = m_aRegisters[ X ];

	_AddOpcodeToHistory( std::format( "{:04X} : LD DT, VX",opcode ).c_str() );
}

void Chip8::LD_VX_KEY( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event, delay and sound timers should continue processing)
	if( Input::GetInstance() )
	{
		if( m_iPreviousKeyPressed != 0xFF && Input::GetInstance()->GetKeyState( m_iPreviousKeyPressed ) == 0 )//Previous Input has been released
		{
			m_aRegisters[ X ] = m_iPreviousKeyPressed;
			m_iPreviousKeyPressed = 0xFF;
		}
		else
		{
			m_iPreviousKeyPressed = Input::GetInstance()->IsAnyKeyPress();
			m_iPC -= 2;
		}
	}
	else
		m_iPC -= 2;

	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, KEY",opcode ).c_str() );
}

void Chip8::LD_ST_VX( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Sets the sound timer to VX
	m_iSound_timer = m_aRegisters[ X ];

	_AddOpcodeToHistory( std::format( "{:04X} : LD ST, VX",opcode ).c_str() );
}

void Chip8::LD_I_FONT( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Sets I to the location of the sprite for the character in VX(only consider the lowest nibble). Characters 0-F (in hexadecimal) are represented by a 4x5 font.
	m_iI = START_FONT_MEMORY_ADDRESS + ( m_aRegisters[ X ] * 5 );

	_AddOpcodeToHistory( std::format( "{:04X} : LD I, FONT( VX )",opcode ).c_str() );
}

void Chip8::LD_I_VX( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Stores from V0 to VX (including VX) in memory, starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified
	for( int i = 0; i <= X; ++i )
	{
#ifdef QUIRK_MEMORY
		m_aMemory[ m_iI ] = m_aRegisters[ i ];
		++m_iI;
#else
		m_aMemory[ m_iI + i ] = m_aRegisters[ i ];
#endif
	}

	_AddOpcodeToHistory( std::format( "{:04X} : LD [I], VX",opcode ).c_str() );
}

void Chip8::LD_VX_I( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified
	for( int i = 0; i <= X; ++i )
	{
#ifdef QUIRK_MEMORY
		m_aRegisters[ i ] = m_aMemory[ m_iI ];
		++m_iI;
#else
		m_aRegisters[ i ] = m_aMemory[ m_iI + i ];
#endif
	}

	_AddOpcodeToHistory( std::format( "{:04X} : LD VX, [I]",opcode ).c_str() );
}

void Chip8::ADD_VX_NN( const uint16_t opcode )
{
	uint8_t NN = opcode & 0x00FF;
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Adds NN to VX (carry flag is not changed).
	m_aRegisters[ X ] += NN;

	_AddOpcodeToHistory( std::format( "{:04X} : ADD VX, NN",opcode ).c_str() );
}

void Chip8::ADD_VX_VY( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	//Adds VY to VX
	uint16_t sum = m_aRegisters[ X ] + m_aRegisters[ Y ];
	m_aRegisters[ X ] = sum & 0xFF;
	Data < uint8_t>& VF = m_aRegisters[ 15 ];
	VF = ( sum > 0xFF ) ? 1 : 0;
	//VF is set to 1 when there's an overflow, and to 0 when there is not.

	_AddOpcodeToHistory( std::format( "{:04X} : ADD VX, VY ( VF )",opcode ).c_str() );
}

void Chip8::ADD_I_VX( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Adds VX to I. VF is not affected
	m_iI += m_aRegisters[ X ];

	_AddOpcodeToHistory( std::format( "{:04X} : ADD I, VX",opcode ).c_str() );
}

void Chip8::SUB_VX_VY( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	Data < uint8_t>& VF = m_aRegisters[ 15 ];

	//VY is subtracted from VX.
	uint16_t diff = m_aRegisters[ X ] - m_aRegisters[ Y ];
	m_aRegisters[ X ] = diff & 0xFF;
	VF = 1;
	if( diff > 0xFF )
		VF = 0;
	// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)

	_AddOpcodeToHistory( std::format( "{:04X} : SUB VX, VY ( VF )",opcode ).c_str() );
}

void Chip8::SHR( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	Data < uint8_t>& VF = m_aRegisters[ 15 ];

	//If the least - significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
	uint8_t LSB = 0;

	std::string OpcodeInstruct;
#ifdef QUIRK_SHIFTING
	OpcodeInstruct = std::format( "{:04X} : SHR VX, VY",opcode );

	LSB = ( m_aRegisters[ Y ] & 0x01 );

	m_aRegisters[ X ] = m_aRegisters[ Y ] >> 1; //before 1990
#else // QUIRK_SHIFTING
	OpcodeInstruct = std::format( "{:04X} : SHR VX",opcode );

	LSB = ( m_aRegisters[ X ] & 0x01 );

	m_aRegisters[ X ] >>= 1;
#endif

	VF = LSB;

	_AddOpcodeToHistory( OpcodeInstruct.c_str() );
}

void Chip8::SUBN_VX_VY( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	Data < uint8_t>& VF = m_aRegisters[ 15 ];

	//Sets VX equals to VY minus VX.
	uint16_t diff = m_aRegisters[ Y ] - m_aRegisters[ X ];
	m_aRegisters[ X ] = diff & 0xFF;
	VF = 1;
	if( diff > 0xFF )
		VF = 0;
	// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX).

	_AddOpcodeToHistory( std::format( "{:04X} : SUBN VX, VY",opcode ).c_str() );
}

void Chip8::SHL( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	Data < uint8_t>& VF = m_aRegisters[ 15 ];

	uint8_t MSB = 0;

	std::string OpcodeInstruct;
#ifdef QUIRK_SHIFTING
	OpcodeInstruct = std::format( "{:04X} : SHL VX, VY",opcode );

	// If the most-significant bit of Vy is 1.
	MSB = ( m_aRegisters[ Y ] & 0x80 ) == 0x80 ? 1 : 0;

	m_aRegisters[ X ] = m_aRegisters[ Y ] << 1; //before 1990
#else // QUIRK_SHIFTING
	OpcodeInstruct = std::format( "{:04X} : SHL VX",opcode );

	// If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
	MSB = ( m_aRegisters[ X ] & 0x80 ) == 0x80 ? 1 : 0;

	m_aRegisters[ X ] <<= 1;
#endif

	VF = MSB;

	_AddOpcodeToHistory( OpcodeInstruct.c_str() );
}

void Chip8::RND( const uint16_t opcode )
{
	uint8_t NN = opcode & 0x00FF;
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
	std::uniform_int_distribution<std::mt19937::result_type> dist( 0,255 );
	m_aRegisters[ X ] = dist( m_iRng ) & NN;

	_AddOpcodeToHistory( std::format( "{:04X} : RND VX, NN",opcode ).c_str() );
}

void Chip8::DRAW( const uint16_t opcode )
{
	std::string OpcodeInstruct;
#ifdef QUIRK_DISPWAIT
	OpcodeInstruct = std::format( "{:04X} : DRW VX, VY VBlank",opcode );

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
	OpcodeInstruct = std::format( "{:04X} : DRW VX, VY",opcode );
#endif

	uint8_t N = opcode & 0x000F;
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	Data < uint8_t>& VF = m_aRegisters[ 15 ];
	/*Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
	The interpreter reads N bytes from memory, starting at the address stored in I.
	These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). Sprites are XORed onto the existing screen.
	If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0.
	If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen
	I value does not change after the execution of this instruction*/
	bool bErased = false;
	uint8_t iYOffset = 0;

	Display::KeyDisplayAccess oKeyDisplay;
	uint8_t xPos = m_aRegisters[ X ] & ( Display::GetWidth() - 1 );
	uint8_t yPos = m_aRegisters[ Y ] & ( Display::GetHeight() - 1 );

	VF = 0;
	for( ; iYOffset < N; ++iYOffset )
	{
#ifdef QUIRK_CLIPPING
		Display::DrawPixelAtPos( oKeyDisplay,xPos,yPos + iYOffset,m_aMemory[ m_iI + iYOffset ],bErased,true );
#else
		Display::DrawPixelAtPos( oKeyDisplay,xPos,yPos + iYOffset,m_aMemory[ m_iI + iYOffset ],bErased,false );
#endif
		if( bErased )
			VF = 1;
	}

	_AddOpcodeToHistory( OpcodeInstruct.c_str() );
}

void Chip8::SKP( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is pressed (usually the next instruction is a jump to skip a code block).
	if( Input::GetInstance()->GetKeyState( m_aRegisters[ X ] ) )
		m_iPC += 2;

	_AddOpcodeToHistory( std::format( "{:04X} : SKP",opcode ).c_str() );
}

void Chip8::SKNP( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed (usually the next instruction is a jump to skip a code block).
	if( !Input::GetInstance()->GetKeyState( m_aRegisters[ X ] ) )
		m_iPC += 2;

	_AddOpcodeToHistory( std::format( "{:04X} : SKNP",opcode ).c_str() );
}

void Chip8::BCD( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;

	//Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2
	m_aMemory[ m_iI ] = m_aRegisters[ X ] / 100;
	m_aMemory[ m_iI + 1 ] = ( m_aRegisters[ X ] / 10 ) % 10;
	m_aMemory[ m_iI + 2 ] = ( m_aRegisters[ X ] % 100 ) % 10;

	_AddOpcodeToHistory( std::format( "{:04X} : BCD",opcode ).c_str() );
}

void Chip8::OR( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;

	//Sets VX to VX or VY. (bitwise OR operation)
	m_aRegisters[ X ] |= m_aRegisters[ Y ];
#ifdef QUIRK_VFRESET
	Data < uint8_t>& VF = m_aRegisters[ 15 ];
	VF = 0;
#endif

	_AddOpcodeToHistory( std::format( "{:04X} : OR VX, VY",opcode ).c_str() );
}

void Chip8::AND( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	//Sets VX to VX and VY. (bitwise AND operation)
	m_aRegisters[ X ] &= m_aRegisters[ Y ];
#ifdef QUIRK_VFRESET
	Data < uint8_t>& VF = m_aRegisters[ 15 ];
	VF = 0;
#endif

	_AddOpcodeToHistory( std::format( "{:04X} : AND VX, VY",opcode ).c_str() );
}

void Chip8::XOR( const uint16_t opcode )
{
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	//Sets VX to VX xor VY
	m_aRegisters[ X ] ^= m_aRegisters[ Y ];
#ifdef QUIRK_VFRESET
	Data < uint8_t>& VF = m_aRegisters[ 15 ];
	VF = 0;
#endif

	_AddOpcodeToHistory( std::format( "{:04X} : XOR VX, VY",opcode ).c_str() );
}