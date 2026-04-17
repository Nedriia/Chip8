#include "Display.h"
#include "Chip8_Debugger.h"
#include <iostream>

// settings
const uint16_t WINDOW_WIDTH = 1920;
const uint16_t WINDOW_HEIGHT = 1080;

uint64_t Display::m_pPixels[ 32 ] = { 0 };

bool Display::m_bDirtyFrame = false;
Display* Display::m_pSingleton = nullptr;

unsigned int Display::m_iFBOTexture = 0;

int Display::m_iValueMicroSRefresh = 16666;
std::chrono::microseconds Display::m_iCurrentTick = std::chrono::microseconds( Display::m_iValueMicroSRefresh );

uint8_t Display::m_iDisplayWidth = 64;//Keep as power of two or instruction in draw opcode might give you exotic result
uint8_t Display::m_iDisplayHeight = 32;

float vertices[] = {
	// positions		//Texture coords
	1.f, 1.0f, 0.0f,	1.0f, 0.0f,		// top right
	1.0f, -1.0f, 0.0f,	1.0f, 1.0f,		// bottom right
	-1.0f, -1.0f, 0.0f,	0.0f, 1.0f,		// bottom left 
	-1.0f, 1.0f, 0.0f,	0.0f, 0.0f,		// top left
};

unsigned int indices[] = {
	0, 1, 3,
	1, 2, 3
};

Display::Display() :
	m_pWindow( nullptr ),
	m_iTexture( 0 ),
	m_iVAO( 0 ),
	m_iVBO( 0 ),
	m_iEBO( 0 ),
	m_iFBO( 0 ),
	m_iLastTimeUpdate( std::chrono::steady_clock::now() )
{}

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

void Display::DrawPixelAtPos( const KeyDisplayAccess& oKey,uint8_t xPos,uint8_t yPos, uint8_t oValue,bool& bErased,bool bClipping )
{
	uint64_t iPreviousValue = m_pPixels[ yPos ];

	//Invert endianess ( only for 8 bit )
	oValue = ( ( oValue >> 1 ) & 0x55 ) | ( ( oValue << 1 ) & 0xAA );
	oValue = ( ( oValue >> 2 ) & 0x33 ) | ( ( oValue << 2 ) & 0xCC );
	oValue = ( oValue >> 4 ) | ( oValue << 4 );

	uint64_t iLine = static_cast< uint64_t >( oValue ) << xPos;

	bErased |= ( m_pPixels[ yPos ] & iLine );

	m_pPixels[ yPos ] ^= iLine;
	m_bDirtyFrame |= ( iPreviousValue != m_pPixels[ yPos ] );
}

void Display::Update( const std::chrono::steady_clock::time_point& time,const bool cpuPaused )
{
	// render loop
	// -----------
	std::chrono::microseconds elapsed = std::chrono::duration_cast< std::chrono::microseconds >( time - m_iLastTimeUpdate );
	std::chrono::microseconds startElapsed = elapsed;
	if( elapsed >= m_iCurrentTick )
	{
		int updateThisFrame = 0;
		const int maxUpdatePerFrame = 5; //could go up to 8 frame( 133 ms )
		if( elapsed >= ( m_iCurrentTick * maxUpdatePerFrame ) )// too much to catch up
		{
			m_iLastTimeUpdate = time;
			elapsed = std::chrono::microseconds( 0 );
			return;
		}

		while( elapsed >= m_iCurrentTick && updateThisFrame < maxUpdatePerFrame )
		{
			m_iLastTimeUpdate += m_iCurrentTick;
			++updateThisFrame;
			elapsed = std::chrono::duration_cast< std::chrono::microseconds >( time - m_iLastTimeUpdate );
		}

		if( m_bDirtyFrame )
		{
			glClearColor( 0.f,0.f,0.f,1.f );
			glClear( GL_COLOR_BUFFER_BIT );

#ifdef DEBUG_INFO
			glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );
#endif
			glBindTexture( GL_TEXTURE_2D,m_iTexture );
			glTexSubImage2D( GL_TEXTURE_2D,0,0,0,2,Display::GetHeight(),GL_RED_INTEGER,GL_UNSIGNED_INT,m_pPixels );

			m_sShaderProgram.Use();

#ifndef DEBUG_INFO
			glBindVertexArray( m_iVAO );
#endif // !DEBUG_INFO

			glDrawElements( GL_TRIANGLES,6,GL_UNSIGNED_INT,0 );

#ifdef DEBUG_INFO
			glBindFramebuffer( GL_FRAMEBUFFER,0 );
#else
			glfwSwapBuffers( m_pWindow );
#endif
			m_bDirtyFrame = false;
		}

#ifdef DEBUG_INFO
		Chip8_Debugger::GetInstance()->Update( startElapsed );
		Chip8_Debugger::GetInstance()->Render();
		glfwSwapBuffers( m_pWindow );
#else
		std::string sPerfDebug = std::format( "Chip8 Emulator : {} ms",std::chrono::duration<double,std::milli>( startElapsed ).count() );
		glfwSetWindowTitle( m_pWindow,sPerfDebug.c_str() );
#endif
	}
}