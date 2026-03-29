#include "Chip8.h"
#include <fstream>
#include <chrono>
#include <assert.h>
#include "Display.h"
#include <iostream>
#include "Input.h"
#include "SoundManager.h"

#define START_FONT_MEMORY_ADDRESS 0x050
#define START_ROM_MEMORY_ADDRESS 0x200
#define MEMORY_SIZE 4096
#define DEFAULT_PARENT_ROM_FOLDER "..\\Roms\\"
#define INSTRUCTIONS_PER_SEC 600

Chip8* Chip8::m_pSingleton = nullptr;

Chip8::Chip8() :
	m_iLastOpcode( 0 )
	,m_iCountBeforeStop( 0 )
	,m_oState( RunningState::Pause )
	,m_iLastTimeUpdate( std::chrono::steady_clock::now() )
	,m_fAccumulator( 0.0f )
	,m_sCurrentRomLoaded( nullptr )
	,m_iCycle( 0 )
	,m_iOpcodesLastFrame( 0 )
	,m_iPreviousKeyPressed( 0xFF )
#ifdef QUIRK_DISPWAIT
	,m_iTimeLastFrame{}
#endif
{
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
	m_fAccumulator = 0.f;
	m_aOpcodeHistory.clear();
	m_iLastOpcode = 0;
	m_iCycle = 0;

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
			m_aMemory[ START_ROM_MEMORY_ADDRESS + i ] = static_cast< uint8_t >( memblock[ i ] );
		}

		delete[] memblock;

		m_sCurrentRomLoaded = sROMToLoad;
	}
	else
	{
		std::cout << "ERROR::CHIP8::LOADING::FILE_NOT_FOUND " << sROMToLoad << std::endl;
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
	if( elapsed >= refreshTick || bForceNextStep )
	{
		float msToSec = ( REFRESH_TICK_MicroSec / 1000000.0f );
		assert( msToSec != 0.f );

		float refreshPerFrame = INSTRUCTIONS_PER_SEC / ( 1 / msToSec );
		m_fAccumulator += refreshPerFrame - std::floor( refreshPerFrame );
		m_iOpcodesLastFrame = std::floor( refreshPerFrame ) + std::floor( m_fAccumulator );
		for( int i = 0; i < m_iOpcodesLastFrame; ++i )
		{
			uint16_t opcode = 0;

			_FetchOpcode( opcode );
			_DecodeExecute_Opcode( opcode );

			if( bForceNextStep )
			{
				m_fAccumulator = 0;
				break;
			}
		}

		m_iLastTimeUpdate += refreshTick;
		m_fAccumulator -= std::floor( m_fAccumulator );


		SoundManager::GetInstance()->Manage( ( uint8_t )m_iSound_timer );
	}
}

void Chip8::AskForState( const KeyAccess& key,RunningState oState ) const
{
	m_oState = oState;
}

void Chip8::DestroyCpu()
{
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
	std::string OpcodeInstruct;

	uint16_t NNN = opcode & 0x0FFF;
	uint8_t NN = opcode & 0x00FF;
	uint8_t N = opcode & 0x000F;
	uint8_t X = ( opcode & 0x0F00 ) >> 8;
	uint8_t Y = ( opcode & 0x00F0 ) >> 4;
	Data < uint8_t>& VF = m_aRegisters[ 15 ];

	uint16_t opcodeNibble = opcode & 0xF000;
	switch( opcodeNibble )
	{
	case 0x0000:
	{
		uint16_t check = opcode & 0x00FF;
		if( check == 0xE0 )
		{
			OpcodeInstruct = std::format( "{:04X}  : CLS",opcode );

			Display::KeyDisplayAccess oKeyDisplay;
			Display::ClearScreen( oKeyDisplay );
		}
		else if( check == 0xEE )
		{
			OpcodeInstruct = std::format( "{:04X} : RET",opcode );

			//Returns from a subroutine
			m_iPC = m_aStack[ m_iSP ];
			m_aStack[ m_iSP ] = 0;
			if( m_iSP != 0 )
				--m_iSP;
		}
		else
		{
			//Calls machine code routine (RCA 1802 for COSMAC VIP) at address NNN. Not necessary for most ROMs.
		}
	}
	break;
	case 0x1000:
	{
		OpcodeInstruct = std::format( "{:04X} : JMP",opcode );

		//Jumps to address NNN
		m_iPC = NNN;
	}
	break;
	case 0x2000:
	{
		if( m_aStack[ 0 ] != 0 ) //could be simplier to use int and init the value to -1 but I want to keep 0 as start
			++m_iSP;

		OpcodeInstruct = std::format( "{:04X} : CALL",opcode );

		//Calls subroutine at NNN
		m_aStack[ m_iSP ] = m_iPC;
		m_iPC = NNN;
	}
	break;
	case 0x3000:
	{
		OpcodeInstruct = std::format( "{:04X} : SE VX, NN",opcode );

		//Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block)
		if( m_aRegisters[ X ] == NN )
			m_iPC += 2;
	}
	break;
	case 0x4000:
	{
		OpcodeInstruct = std::format( "{:04X} : SNE VX, NN",opcode );

		//Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
		if( m_aRegisters[ X ] != NN )
			m_iPC += 2;
	}
	break;
	case 0x5000:
	{
		OpcodeInstruct = std::format( "{:04X} : SE VX, VY",opcode );

		//Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
		if( m_aRegisters[ X ] == m_aRegisters[ Y ] )
			m_iPC += 2;
	}
	break;
	case 0x6000:
	{
		OpcodeInstruct = std::format( "{:04X} : LD VX, NN",opcode );

		//Sets NN To VX
		m_aRegisters[ X ] = NN;
	}
	break;
	case 0x7000:
	{
		OpcodeInstruct = std::format( "{:04X} : ADD VX, NN",opcode );

		//Adds NN to VX (carry flag is not changed).
		m_aRegisters[ X ] += NN;
	}
	break;
	case 0x8000:
	{
		switch( opcode & 0x000F )
		{
		case 0:
		{
			OpcodeInstruct = std::format( "{:04X} : LD VX, VY",opcode );

			//Sets VX to the value of VY.
			m_aRegisters[ X ] = m_aRegisters[ Y ];
		}
		break;
		case 1:
		{
			OpcodeInstruct = std::format( "{:04X} : OR VX, VY",opcode );

			//Sets VX to VX or VY. (bitwise OR operation)
			m_aRegisters[ X ] |= m_aRegisters[ Y ];
#ifdef QUIRK_VFRESET
			VF = 0;
#endif
		}
		break;
		case 2:
		{
			OpcodeInstruct = std::format( "{:04X} : AND VX, VY",opcode );

			//Sets VX to VX and VY. (bitwise AND operation)
			m_aRegisters[ X ] &= m_aRegisters[ Y ];
#ifdef QUIRK_VFRESET
			VF = 0;
#endif
		}
		break;
		case 3:
		{
			OpcodeInstruct = std::format( "{:04X} : XOR VX, VY",opcode );

			//Sets VX to VX xor VY
			m_aRegisters[ X ] ^= m_aRegisters[ Y ];
#ifdef QUIRK_VFRESET
			VF = 0;
#endif
		}
		break;
		case 4:
		{
			OpcodeInstruct = std::format( "{:04X} : ADD VX, VY ( VF )",opcode );

			//Adds VY to VX
			uint16_t sum = m_aRegisters[ X ] + m_aRegisters[ Y ];
			m_aRegisters[ X ] = sum & 0xFF;
			VF = ( sum > 0xFF ) ? 1 : 0;
			//VF is set to 1 when there's an overflow, and to 0 when there is not.
		}
		break;
		case 5:
		{
			OpcodeInstruct = std::format( "{:04X} : SUB VX, VY ( VF )",opcode );

			//VY is subtracted from VX.
			uint16_t diff = m_aRegisters[ X ] - m_aRegisters[ Y ];
			m_aRegisters[ X ] = diff & 0xFF;
			VF = 1;
			if( diff > 0xFF )
				VF = 0;
			// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
		}
		break;
		case 6:
		{
			//If the least - significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
			uint8_t LSB = 0;

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
		}
		break;
		case 7:
		{
			OpcodeInstruct = std::format( "{:04X} : SUBN VX, VY",opcode );

			//Sets VX equals to VY minus VX.
			uint16_t diff = m_aRegisters[ Y ] - m_aRegisters[ X ];
			m_aRegisters[ X ] = diff & 0xFF;
			VF = 1;
			if( diff > 0xFF )
				VF = 0;
			// VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX).
		}
		break;
		case 0xE:
		{
			uint8_t MSB = 0;

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
		}
		break;
		default:
			break;
		}
	}
	break;
	case 0x9000:
	{
		OpcodeInstruct = std::format( "{:04X} : SNE VX, VY",opcode );

		//Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block)
		if( m_aRegisters[ X ] != m_aRegisters[ Y ] )
			m_iPC += 2;
	}
	break;
	case 0xA000:
	{
		OpcodeInstruct = std::format( "{:04X} : LD I, NNN",opcode );

		//Sets I to the address NNN
		m_iI = NNN;
	}
	break;
	case 0xB000:
	{
#ifdef QUIRK_JUMPING
		//Jumps to the address NNN plus V0
		OpcodeInstruct = std::format( "{:04X} : JMP V0, NNN",opcode );
		m_iPC = NNN + m_aRegisters[ 0 ];
#else // QUIRK_JUMPING
		OpcodeInstruct = std::format( "{:04X} : JMP VX, NNN",opcode );
		m_iPC = NNN + m_aRegisters[ X ];
#endif
	}
	break;
	case 0xC000:
	{
		OpcodeInstruct = std::format( "{:04X} : RND VX, NN",opcode );

		//Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
		std::uniform_int_distribution<std::mt19937::result_type> dist( 0,255 );
		m_aRegisters[ X ] = dist( m_iRng ) & NN;
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

#ifdef QUIRK_DISPWAIT
		OpcodeInstruct = std::format( "{:04X} : DRW VX, VY VBlank",opcode );

		//VBlank, waiting for next frame
		if( m_iTimeLastFrame.time_since_epoch().count() == 0 )
		{
			m_iTimeLastFrame = m_iLastTimeUpdateTimers;
			m_iPC -= 2;
			break;
		}
		else if( m_iLastTimeUpdateTimers >= ( m_iTimeLastFrame + refreshTick ) )
		{
			m_iTimeLastFrame = {};
		}
		else
		{
			m_iPC -= 2;
			break;
		}
#else
		OpcodeInstruct = std::format( "{:04X} : DRW VX, VY",opcode );
#endif

		Display::KeyDisplayAccess oKeyDisplay;
		uint8_t xPos = m_aRegisters[ X ] & ( Display::CHIP8_DISPLAY_WIDTH - 1 );
		uint8_t yPos = m_aRegisters[ Y ] & ( Display::CHIP8_DISPLAY_HEIGHT - 1 );

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

		break;
	}
	case 0xE000:
	{
		uint16_t check = opcode & 0x00FF;
		if( check == 0x9E )
		{
			OpcodeInstruct = std::format( "{:04X} : SKP",opcode );

			//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is pressed (usually the next instruction is a jump to skip a code block).
			if( Input::GetInstance()->GetKeyState( m_aRegisters[ X ] ) )
				m_iPC += 2;
		}
		else if( check == 0xA1 )
		{
			OpcodeInstruct = std::format( "{:04X} : SKNP",opcode );

			//Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed (usually the next instruction is a jump to skip a code block).
			if( !Input::GetInstance()->GetKeyState( m_aRegisters[ X ] ) )
				m_iPC += 2;
		}
	}
	break;
	case 0xF000:
	{
		switch( opcode & 0x00FF )
		{
		case 7:
			OpcodeInstruct = std::format( "{:04X} : LD VX, DT",opcode );

			//Sets VX to the value of the delay timer.
			m_aRegisters[ X ] = m_iDelay_timer;
			break;
		case 0x0A:
		{
			OpcodeInstruct = std::format( "{:04X} : LD VX, KEY",opcode );

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
		}
		break;
		case 0x15:
			OpcodeInstruct = std::format( "{:04X} : LD DT, VX",opcode );

			//Sets the delay timer to VX
			m_iDelay_timer = m_aRegisters[ X ];
			break;
		case 0x18:
			OpcodeInstruct = std::format( "{:04X} : LD ST, VX",opcode );

			//Sets the sound timer to VX
			m_iSound_timer = m_aRegisters[ X ];
			break;
		case 0x1E:
			OpcodeInstruct = std::format( "{:04X} : ADD I, VX",opcode );

			//Adds VX to I. VF is not affected
			m_iI += m_aRegisters[ X ];
			break;
		case 0x29:
		{
			OpcodeInstruct = std::format( "{:04X} : LD I, FONT( VX )",opcode );

			//Sets I to the location of the sprite for the character in VX(only consider the lowest nibble). Characters 0-F (in hexadecimal) are represented by a 4x5 font.
			m_iI = START_FONT_MEMORY_ADDRESS + ( m_aRegisters[ X ] * 5 );
		}
		break;
		case 0x33:
		{
			OpcodeInstruct = std::format( "{:04X} : BCD VX",opcode );

			//Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2
			m_aMemory[ m_iI ] = m_aRegisters[ X ] / 100;
			m_aMemory[ m_iI + 1 ] = ( m_aRegisters[ X ] / 10 ) % 10;
			m_aMemory[ m_iI + 2 ] = ( m_aRegisters[ X ] % 100 ) % 10;
		}
		break;
		case 0x55:
		{
			OpcodeInstruct = std::format( "{:04X} : LD [I], VX",opcode );

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
		}
		break;
		case 0x65:
		{
			OpcodeInstruct = std::format( "{:04X} : LD VX, [I]",opcode );

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

	if( OpcodeInstruct.empty() )
		std::cout << "Opcode Unknown " << std::format( "{:04X}",opcode ) << std::endl;

	_AddOpcodeToHistory( OpcodeInstruct.c_str() );

	if( opcode == m_iLastOpcode && opcodeNibble == 0x1000 )//JMP instruction
	{
		++m_iCountBeforeStop;
		if( m_iCountBeforeStop >= 2 )
		{
			m_oState = RunningState::Pause;
			return;
		}
	}
	else
	{
		m_iLastOpcode = opcode;
		m_iCountBeforeStop = 0;
	}
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
	//https://github.com/trapexit/chip-8_documentation
	if( m_aOpcodeHistory.size() >= 0x40 )
		m_aOpcodeHistory.pop_front();

	m_aOpcodeHistory.push_back( pOpcode );
	++m_iCycle;
}