#include "Display.h"
#include "Chip8_Debugger.h"
#include <iostream>
#include "Chip8.h"
#include <thread>

// settings
const uint16_t WINDOW_WIDTH = 1920;
const uint16_t WINDOW_HEIGHT = 1080;

uint64_t Display::m_pPixels[ 64 ][ 2 ] = { 0 };

bool Display::m_bDirtyFrame = false;
Display* Display::m_pSingleton = nullptr;

unsigned int Display::m_iFBOTexture = 0;
unsigned int Display::m_iTexture = 0;

int Display::m_iValueMicroSRefresh = 16666;
std::chrono::microseconds Display::m_iCurrentTick = std::chrono::microseconds( Display::m_iValueMicroSRefresh );

uint8_t Display::m_iDisplayWidth = 0;//TODO : init to 0, so we could check and stop if no resolution is found
uint8_t Display::m_iDisplayHeight = 0;

std::string Display::m_sGameTitle = "";

#define WIDTH_DEFAULT_ON_ERROR 64
#define HEIGHT_DEFAULT_ON_ERROR 32

float vertices[] = {
	// positions		//Texture coords
	1.f, 1.0f, 0.0f,	0.0f, 0.0f,		// top right
	1.0f, -1.0f, 0.0f,	0.0f, 1.0f,		// bottom right
	-1.0f, -1.0f, 0.0f,	1.0f, 1.0f,		// bottom left 
	-1.0f, 1.0f, 0.0f,	1.0f, 0.0f,		// top left
};

unsigned int indices[] = {
	0, 1, 3,
	1, 2, 3
};

Display::Display() :
	m_pWindow( nullptr ),
	m_iVAO( 0 ),
	m_iVBO( 0 ),
	m_iEBO( 0 ),
	m_iFBO( 0 ),
	m_iLastTimeUpdate( std::chrono::steady_clock::now() ),
	m_oResolutionMode( ResolutionMode::LORES )
{
}

Display::~Display()
{
	m_pSingleton = nullptr;
}

static void glfw_error_callback( int error,const char* description )
{
	fprintf( stderr,"GLFW Error %d: %s\n",error,description );
}

int Display::Init( const KeyDisplayAccess& oKey,const Chip8* pCpu )
{
	glfwSetErrorCallback( glfw_error_callback );
	if( !glfwInit() )
		return -1;

	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR,3 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR,3 );
	glfwWindowHint( GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE );

	_CreateWindowChip();
	_InitRenderer();

	_InitPixelsData();
	_InitFramebuffer();

#ifdef DEBUG_INFO
	Chip8_Debugger::GetInstance()->Init( m_pWindow,pCpu );
#endif

	return 0;
}

int Display::_CreateWindowChip()
{
	// glfw window creation
	// --------------------
	m_pWindow = glfwCreateWindow( WINDOW_WIDTH,WINDOW_HEIGHT,"Chip8 Emulator",nullptr,nullptr );
	if( m_pWindow == nullptr )
	{
		std::cerr << "DISPLAY::FAILED_TO_CREATE_GLFW_WINDOW" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent( m_pWindow );
	//glfwSwapInterval( 1 );
	glfwSetFramebufferSizeCallback( m_pWindow,Display::framebuffer_size_callback );

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if( !gladLoadGLLoader( ( GLADloadproc )glfwGetProcAddress ) )
	{
		std::cerr << "DISPLAY::GLAD_FAILED_TO_INIT" << std::endl;
		return -1;
	}

	return 0;
}

void Display::_InitTexture()
{
#ifdef DEBUG_INFO
	glGenTextures( 1,&m_iFBOTexture );
	glBindTexture( GL_TEXTURE_2D,m_iFBOTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_RGB,WINDOW_WIDTH,WINDOW_HEIGHT,0,GL_RGB,GL_UNSIGNED_BYTE,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );

	glBindTexture( GL_TEXTURE_2D,0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,m_iFBOTexture,0 );
#endif
	//*-------------------------------------------------------------------------------------------------*//
	glGenTextures( 1,&m_iTexture );
	glBindTexture( GL_TEXTURE_2D,m_iTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_R32UI,2,m_iDisplayHeight,0,GL_RED_INTEGER,GL_UNSIGNED_INT,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );

	glBindTexture( GL_TEXTURE_2D,0 );
}

void Display::_InitFramebuffer()
{
#ifdef DEBUG_INFO
	glGenFramebuffers( 1,&m_iFBO );
	glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );
#endif

	_InitTexture();

#ifdef DEBUG_INFO
	glBindFramebuffer( GL_FRAMEBUFFER,0 );

	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		std::cerr << "DISPLAY::FRAMEBUFFER_NOT_COMPLETE" << std::endl;
#endif
}

void Display::_InitRenderer()
{
	glGenVertexArrays( 1,&m_iVAO );
	glBindVertexArray( m_iVAO ); //Need to be bind before, any vertex attributs call will be stored inside the VAO
	glGenBuffers( 1,&m_iVBO );
	glGenBuffers( 1,&m_iEBO );

	glBindBuffer( GL_ARRAY_BUFFER,m_iVBO );
	glBufferData( GL_ARRAY_BUFFER,sizeof( vertices ),vertices,GL_STATIC_DRAW );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,m_iEBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof( indices ),indices,GL_STATIC_DRAW );

	glVertexAttribPointer( 0,3,GL_FLOAT,GL_FALSE,5 * sizeof( float ),( void* )0 ); //Vertices
	glEnableVertexAttribArray( 0 );

	glVertexAttribPointer( 1,2,GL_FLOAT,GL_FALSE,5 * sizeof( float ),( void* )( 3 * sizeof( float ) ) ); //UV coords
	glEnableVertexAttribArray( 1 );

	glBindVertexArray( m_iVAO );

	m_sShaderProgram = Shader( "vertexShader.glsl","fragmentShader.glsl" );
}

void Display::_DestroyRenderer()
{
	glDeleteVertexArrays( 1,&m_iVAO );
	glDeleteBuffers( 1,&m_iVBO );
	glDeleteBuffers( 1,&m_iEBO );
	glDeleteFramebuffers( 1,&m_iFBO );
}

void Display::_InitPixelsData()
{
	memset( m_pPixels,0,sizeof( m_pPixels ) );
	m_bDirtyFrame = true;
}

void Display::AssignDisplaySettings( bool bDefaultRes /*= false*/, const std::vector<std::string >& sColors /*= {}*/ )
{
	int iIndex = 0;

	int vertexBgColorLocation = glGetUniformLocation( m_sShaderProgram.ID,"bgColor" );
	int vertexFontColorLocation = glGetUniformLocation( m_sShaderProgram.ID,"fontColor" );

	glUseProgram( m_sShaderProgram.ID );

	if( sColors.empty() )
	{
		glUniform3f( vertexBgColorLocation,0.67f,0.27f,0.0f );
		glUniform3f( vertexFontColorLocation,1.0f,0.67f,0.0f );

		if( bDefaultRes )
			SetResolution( WIDTH_DEFAULT_ON_ERROR, HEIGHT_DEFAULT_ON_ERROR );

		return;
	}

	for( std::string sColor : sColors )
	{
		if( sColor[ 0 ] == '#' )
			sColor.erase( 0,1 );

		unsigned long iColor = std::stoul( sColor,nullptr,16 );
		float vColor[ 3 ] = { ( ( iColor >> 16 ) & 0xFF ) / 255.0f, ( ( iColor >> 8 ) & 0xFF ) / 255.0f, ( iColor & 0xFF ) / 255.0f };

		switch( iIndex )
		{
		case 0:
			glUniform3f( vertexBgColorLocation,vColor[ 0 ],vColor[ 1 ],vColor[ 2 ] );
			break;
		case 1:
			glUniform3f( vertexFontColorLocation,vColor[ 0 ],vColor[ 1 ],vColor[ 2 ] );
			break;
		default:
			std::cerr << "Case Color not handle";
			break;
		}
		++iIndex;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void Display::framebuffer_size_callback( GLFWwindow* m_pWindow,int width,int height )
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport( 0,0,width,height );

#ifdef DEBUG_INFO
	glBindTexture( GL_TEXTURE_2D,m_iFBOTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,NULL );//need to reallocate texture

	glBindTexture( GL_TEXTURE_2D,0 );

	glBindTexture( GL_TEXTURE_2D,m_iTexture );

	m_bDirtyFrame = true;
#endif
}

void Display::DestroyWindow( const KeyDisplayAccess& oKey )
{
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glDeleteTextures( 1,&m_iTexture );
	glDeleteTextures( 1,&m_iFBOTexture );
	_DestroyRenderer();

	m_iTexture = 0;
	m_iFBOTexture = 0;
	m_iVAO = 0;
	m_iVBO = 0;
	m_iEBO = 0;
	m_iFBO = 0;

	m_sShaderProgram.Delete();

	glfwDestroyWindow( m_pWindow );
	glfwTerminate();

	m_pWindow = nullptr;
	delete m_pSingleton;
}

void Display::ClearScreen( const KeyDisplayAccess& oKey )
{
	_InitPixelsData();
}

void Display::DrawPixelAtPos( const KeyDisplayAccess& oKey,uint8_t xStartingPos,uint8_t yStartingPos,uint8_t N,uint8_t& iVFFlag,bool bWrapping )
{
	Chip8* pInstance = Chip8::GetInstance();

	uint8_t iCurrentX = xStartingPos & ( m_iDisplayWidth - 1 );
	uint8_t iCurrentY = yStartingPos & ( m_iDisplayHeight - 1 );

	int iSpriteSize = 8;
	if( N == 0 )
	{
		uint8_t iIndex = m_iDisplayWidth == 64 ? 1 : 0;
		uint8_t iIndex2 = iIndex == 0 ? 1 : 0;

		iSpriteSize = 16;
		for( uint8_t iYOffset = 0; iYOffset < 16; ++iYOffset,++iCurrentY )
		{
			if( !bWrapping )
			{
				if( iCurrentY >= m_iDisplayHeight )
					return;
			}
			else if( bWrapping )
				iCurrentY &= ( m_iDisplayHeight - 1 );

			uint16_t iMemoryValue = *( pInstance->GetMemory()->begin() + pInstance->GetI() + ( iYOffset * 2 ) ) << 8 |
									*( pInstance->GetMemory()->begin() + pInstance->GetI() + ( iYOffset * 2 ) + 1 );

			uint64_t iLine = static_cast< uint64_t >( iMemoryValue ) << 48;

			uint64_t iPreviousValue = m_pPixels[ iCurrentY ][ iIndex ];
			uint64_t iPreviousValue2 = m_pPixels[ iCurrentY ][ iIndex2 ];

			//Check in wich block we are, or if both
			if( iCurrentX < 64 && iCurrentX + iSpriteSize >= 64 )
			{
				if( m_iDisplayWidth > 64 )
				{
					int xShift = iCurrentX + iSpriteSize;
					int iAns = xShift - 64;

					uint64_t iLine2 = iLine;
					iLine <<= ( iSpriteSize - iAns );
					iLine2 >>= iCurrentX;

					iVFFlag |= ( m_pPixels[ iCurrentY ][ iIndex ] & iLine ) || ( m_pPixels[ iCurrentY ][ iIndex2 ] & iLine2 ) ? 1 : 0;

					m_pPixels[ iCurrentY ][ iIndex ] ^= iLine;
					m_pPixels[ iCurrentY ][ iIndex2 ] ^= iLine2;
				}
				else
				{
					if( bWrapping )
						iLine = iLine >> iCurrentX | iLine << ( m_iDisplayWidth - iCurrentX );
					else
						iLine >>= iCurrentX;

					m_pPixels[ iCurrentY ][ iIndex2 ] ^= iLine;
				}

				m_bDirtyFrame |= ( iPreviousValue != m_pPixels[ iCurrentY ][ iIndex ] ) || ( iPreviousValue2 != m_pPixels[ iCurrentY ][ iIndex2 ] );
			}
			else if( iCurrentX < 64 )
			{
				iLine >>= iCurrentX;

				iVFFlag |= ( m_pPixels[ iCurrentY ][ iIndex2 ] & iLine ) ? 1 : 0;
				m_pPixels[ iCurrentY ][ iIndex2 ] ^= iLine;

				m_bDirtyFrame |= ( iPreviousValue2 != m_pPixels[ iCurrentY ][ iIndex2 ] );
			}
			else if( iCurrentX >= 64 )
			{
				iLine >>= ( iCurrentX - 64 );

				iVFFlag |= ( m_pPixels[ iCurrentY ][ iIndex ] & iLine ) ? 1 : 0;
				m_pPixels[ iCurrentY ][ iIndex ] ^= iLine;

				if( bWrapping )
				{
					int xShift = iCurrentX + iSpriteSize;
					if( xShift >= m_iDisplayWidth )
					{
						int iAns = xShift - 64;

						uint64_t iLine2 = static_cast< uint64_t >( iMemoryValue ) << 48;
						iLine2 <<= ( iSpriteSize - iAns );

						iVFFlag |= ( m_pPixels[ iCurrentY ][ iIndex2 ] & iLine2 ) ? 1 : 0;
						m_pPixels[ iCurrentY ][ iIndex2 ] ^= iLine2;
					}
				}

				m_bDirtyFrame |= ( iPreviousValue != m_pPixels[ iCurrentY ][ iIndex ] ) || ( iPreviousValue2 != m_pPixels[ iCurrentY ][ iIndex2 ] );
			}
		}
		return;
	}

	for( uint8_t iYOffset = 0; iYOffset < N; ++iYOffset, ++iCurrentY )
	{
		if( !bWrapping )
		{
			if( iCurrentY >= m_iDisplayHeight )
				return;
		}
		else if( bWrapping )
			iCurrentY &= ( m_iDisplayHeight - 1 );

		uint16_t iMemoryValue = 0;
		uint64_t iLine = 0;

		uint16_t iMemoryOffset = ( pInstance->GetI() + iYOffset );

#ifdef OVERFLOW_CONTROL
		iMemoryOffset &= 0xFFF;
#endif	
		
		if( GetResolutionMode() != ResolutionMode::HIRES )
		{
			uint64_t iPreviousValue = m_pPixels[ iCurrentY ][ 0 ];

			iMemoryValue = ( uint8_t& )*( pInstance->GetMemory()->begin() + iMemoryOffset );
			iLine = static_cast<uint64_t>( iMemoryValue ) << 56; //Store as big endian in ram

			if( iCurrentX != 0 )
			{
				if( bWrapping )
					iLine = iLine >> iCurrentX | iLine << ( m_iDisplayWidth - iCurrentX );
				else
					iLine >>= iCurrentX;
			}

			iVFFlag |= ( m_pPixels[ iCurrentY ][ 0 ] & iLine ) ? 1 : 0;
			m_pPixels[ iCurrentY ][ 0 ] ^= iLine;
 			m_bDirtyFrame |= ( iPreviousValue != m_pPixels[ iCurrentY ][ 0 ] );
		}
		else
		{
			iMemoryValue = ( uint8_t& )*( pInstance->GetMemory()->begin() + iMemoryOffset );

			iLine = static_cast< uint64_t >( iMemoryValue ) << 56; //Store as big endian in ram

			uint64_t iPreviousValue = m_pPixels[ iCurrentY ][ 0 ];
			uint64_t iPreviousValue2 = m_pPixels[ iCurrentY ][ 1 ];

			//Check in wich block we are, or if both
			if( iCurrentX < 64 && iCurrentX + iSpriteSize >= 64 )
			{
				int xShift = iCurrentX + iSpriteSize;
				int iAns = xShift - 64;

				uint64_t iLine2 = iLine;
				iLine  <<= ( iSpriteSize - iAns );
				iLine2 >>= iCurrentX;

				iVFFlag |= ( m_pPixels[ iCurrentY ][ 0 ] & iLine ) || ( m_pPixels[ iCurrentY ][ 1 ] & iLine2 ) ? 1 : 0;

				m_pPixels[ iCurrentY ][ 0 ] ^= iLine;
				m_pPixels[ iCurrentY ][ 1 ] ^= iLine2;

				m_bDirtyFrame |= ( iPreviousValue != m_pPixels[ iCurrentY ][ 0 ] ) || ( iPreviousValue2 != m_pPixels[ iCurrentY ][ 1 ] );
			}
			else if( iCurrentX < 64 )
			{
				iLine >>= iCurrentX;

				iVFFlag |= ( m_pPixels[ iCurrentY ][ 1 ] & iLine ) ? 1 : 0;
				m_pPixels[ iCurrentY ][ 1 ] ^= iLine;

				m_bDirtyFrame |= iPreviousValue2 != m_pPixels[ iCurrentY ][ 1 ];
			}
			else if( iCurrentX >= 64 )
			{
				iLine >>= ( iCurrentX - 64 );

				iVFFlag |= ( m_pPixels[ iCurrentY ][ 0 ] & iLine ) ? 1 : 0;
				m_pPixels[ iCurrentY ][ 0 ] ^= iLine;

				if( bWrapping )
				{
					int xShift = iCurrentX + iSpriteSize;
					if( xShift >= m_iDisplayWidth )
					{
						int iAns = xShift - 64;
						uint64_t iOverflow = static_cast< uint64_t >( iMemoryValue ) << 56;
						iOverflow <<= ( iSpriteSize - iAns );
						iVFFlag |= ( m_pPixels[ iCurrentY ][ 1 ] & iOverflow ) ? 1 : 0;
						m_pPixels[ iCurrentY ][ 1 ] ^= iOverflow;
					}
				}

				m_bDirtyFrame |= ( iPreviousValue != m_pPixels[ iCurrentY ][ 0 ] ) || ( iPreviousValue2 != m_pPixels[ iCurrentY ][ 1 ] );
			}
		}
	}
}

void Display::ScrollVertical( const KeyDisplayAccess& oKey,uint8_t N, bool bDown )
{
	if( bDown )
	{
		N = Chip8::m_oCurrentQuirk.bLegacySrolling ? N / 2 : N;
		for( int k = m_iDisplayHeight - 1; k >= 0; --k )
		{
			int iIndex = k - N;
			if( iIndex < 0 )
			{
				m_pPixels[ k ][ 0 ] = 0; //Clip
				if( GetResolutionMode() == ResolutionMode::HIRES )
					m_pPixels[ k ][ 1 ] = 0;
			}
			else
			{
				m_pPixels[ k ][ 0 ] = m_pPixels[ iIndex ][ 0 ];
				if( GetResolutionMode() == ResolutionMode::HIRES )
					m_pPixels[ k ][ 1 ] = m_pPixels[ iIndex ][ 1 ];
			}
		}
	}
	else
	{
		for( int k = 0; k < m_iDisplayHeight; ++k )
		{
			int iIndex = k + N;
			if( iIndex < m_iDisplayHeight )
			{
				m_pPixels[ k ][ 0 ] = m_pPixels[ iIndex ][ 0 ];
				if( GetResolutionMode() == ResolutionMode::HIRES )
					m_pPixels[ k ][ 1 ] = m_pPixels[ iIndex ][ 1 ];
			}
			else
			{
				m_pPixels[ k ][ 0 ] = 0; //Clip
				if( GetResolutionMode() == ResolutionMode::HIRES )
					m_pPixels[ k ][ 1 ] = 0;
			}
		}
	}
}

void Display::ScrollHorizontal( const KeyDisplayAccess& oKey,bool bLeft )
{
	uint8_t iScrollValue = Chip8::m_oCurrentQuirk.bLegacySrolling ? 2 : 4;
	for( int k = 0; k < m_iDisplayHeight; ++k )
	{
		if( GetResolutionMode() == ResolutionMode::LORES )
			bLeft ? m_pPixels[ k ][ 0 ] <<= iScrollValue : m_pPixels[ k ][ 0 ] >>= iScrollValue;
		else
		{
			//Check if we push outside the block
			if( bLeft )
			{
				uint8_t iBlockErase = ( m_pPixels[ k ][ 0 ] >> 60 ) & 0xF;

				m_pPixels[ k ][ 0 ] <<= iScrollValue;
				m_pPixels[ k ][ 1 ] = ( m_pPixels[ k ][ 1 ] << iScrollValue ) | iBlockErase;
			}
			else
			{
				uint64_t iBlockErase = m_pPixels[ k ][ 1 ] & 0xF;

				m_pPixels[ k ][ 1 ] >>= iScrollValue;
				m_pPixels[ k ][ 0 ] = ( m_pPixels[ k ][ 0 ] >> iScrollValue ) | ( iBlockErase << 60 );
			}
		}
	}
}

void Display::Update( const std::chrono::steady_clock::time_point& time,const bool cpuPaused )
{
	// render loop
	// -----------
	std::chrono::microseconds elapsed = std::chrono::duration_cast< std::chrono::microseconds >( time - m_iLastTimeUpdate );
	std::chrono::microseconds startElapsed = elapsed;
	if( elapsed >= m_iCurrentTick )
	{
#ifdef DEBUG_INFO
		if( elapsed >= ( m_iCurrentTick * 5 ) )// too much to catch up ( 83 ms )
		{
			m_iLastTimeUpdate = time;
			elapsed = std::chrono::microseconds( 0 );
			return;
		}

		while( elapsed >= m_iCurrentTick )
		{
#endif
			m_iLastTimeUpdate += m_iCurrentTick;

#ifdef DEBUG_INFO
			elapsed = std::chrono::duration_cast< std::chrono::microseconds >( time - m_iLastTimeUpdate );
		}
#endif

		if( m_bDirtyFrame )
		{
			glClearColor( 0.f,0.f,0.f,1.f );
			glClear( GL_COLOR_BUFFER_BIT );

#ifdef DEBUG_INFO
			glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );
#endif
			glTexSubImage2D( GL_TEXTURE_2D,0,0,0,4,Display::GetHeight(),GL_RED_INTEGER,GL_UNSIGNED_INT,m_pPixels );

			m_sShaderProgram.Use();

			glDrawElements( GL_TRIANGLES,6,GL_UNSIGNED_INT,0 );

#ifdef DEBUG_INFO
			glBindFramebuffer( GL_FRAMEBUFFER,0 );
#endif
			m_bDirtyFrame = false;
		}

#ifdef DEBUG_INFO
		Chip8_Debugger::GetInstance()->Update( startElapsed );
		Chip8_Debugger::GetInstance()->Render();
#endif
		std::string sPerfDebug = std::format( "{} : {} ms ",m_sGameTitle,std::chrono::duration<double,std::milli>( startElapsed ).count() );
		glfwSetWindowTitle( m_pWindow,sPerfDebug.c_str() );

		glfwSwapBuffers( m_pWindow );
	}
	else
	{
		std::this_thread::sleep_for( m_iCurrentTick - elapsed );
	}
}

void Display::SetResolution( const int iWidth,const int iHeight )
{
	if( m_iDisplayWidth == iWidth && m_iDisplayHeight == iHeight )
		return;

	m_iDisplayWidth = iWidth;
	m_iDisplayHeight = iHeight;
	int iWithLocation = glGetUniformLocation( m_sShaderProgram.ID,"Width" );
	int iHeightLocation = glGetUniformLocation( m_sShaderProgram.ID,"Height" );

	glUseProgram( m_sShaderProgram.ID );

	glUniform1i( iWithLocation,m_iDisplayWidth );
	glUniform1i( iHeightLocation,m_iDisplayHeight );
	glUniform1i( iHeightLocation,m_iDisplayHeight );

	//Update texture with new res
	glBindTexture( GL_TEXTURE_2D,m_iTexture );
	glTexImage2D( GL_TEXTURE_2D,0,GL_R32UI,4,iHeight,0,GL_RED_INTEGER,GL_UNSIGNED_INT,NULL );//Beware to Display::Update glTexSubImage2D call
	m_bDirtyFrame = true;

#ifdef DEBUG_INFO
	std::cout << std::format( "DISPLAY::CHANGE_CURRENT_RESOLUTION_{}x{}",m_iDisplayWidth,m_iDisplayHeight ) << std::endl;
#endif
}

void Display::SetResolutionMode( const ResolutionMode oResolutionMode )
{
	//Keep res as power of two or instruction in draw opcode might give you exotic result
	if( m_oResolutionMode != oResolutionMode )
		m_oResolutionMode = oResolutionMode;

	switch( m_oResolutionMode )
	{
	case ResolutionMode::HIRES:
		SetResolution( 128,64 );
		break;
	case ResolutionMode::LORES:
		SetResolution( 64,32 );
		break;
	default:
		std::cerr << "Should not pass here with that value" << std::endl;
		break;
	}
}