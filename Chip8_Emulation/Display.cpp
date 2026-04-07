#include "Display.h"
#include "Chip8_Debugger.h"
#include <iostream>

// settings
const uint16_t WINDOW_WIDTH = 1920;
const uint16_t WINDOW_HEIGHT = 1080;

uint8_t* Display::m_pPixels = nullptr;
bool Display::m_bDirtyFrame = false;
Display* Display::m_pSingleton = nullptr;

unsigned int Display::m_iFBOTexture = 0;

int Display::m_iValueMicroSRefresh = 16666;
std::chrono::microseconds Display::m_iCurrentTick = std::chrono::microseconds( Display::m_iValueMicroSRefresh );

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
	m_iLastTimeUpdate( std::chrono::steady_clock::now() ),
	m_iDisplayWidth ( 64 ),//Keep as power of two or instruction in draw opcode might give you exotic result
	m_iDisplayHeight ( 32 )
{}

Display::~Display()
{
	delete[] m_pPixels;
	m_pPixels = nullptr;
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
	m_pPixels = new uint8_t[ m_iDisplayWidth * m_iDisplayHeight ];
	_InitPixelsData();
	_InitFramebuffer();

	Chip8_Debugger::GetInstance()->Init( m_pWindow,pCpu );

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
	glGenTextures( 1,&m_iFBOTexture );
	glBindTexture( GL_TEXTURE_2D,m_iFBOTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_RGB,WINDOW_WIDTH,WINDOW_HEIGHT,0,GL_RGB,GL_UNSIGNED_BYTE,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );

	glBindTexture( GL_TEXTURE_2D,0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,m_iFBOTexture,0 );
	//*-------------------------------------------------------------------------------------------------*//
	glGenTextures( 1,&m_iTexture );
	glBindTexture( GL_TEXTURE_2D,m_iTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_R8,m_iDisplayWidth,m_iDisplayHeight,0,GL_RED,GL_UNSIGNED_BYTE,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );

	glBindTexture( GL_TEXTURE_2D,0 );
}

void Display::_InitFramebuffer()
{
	glGenFramebuffers( 1,&m_iFBO );
	glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );

	_InitTexture();

	glBindFramebuffer( GL_FRAMEBUFFER,0 );

	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		std::cerr << "DISPLAY::FRAMEBUFFER_NOT_COMPLETE" << std::endl;
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
	for( int i = 0; i < Display::GetHeight(); ++i )
	{
		for( int j = 0; j < Display::GetWidth(); ++j )
			*( m_pPixels + i * Display::GetWidth() + j ) = 0;
	}
	m_bDirtyFrame = true;
}

void Display::_XORedPixelsData( int xPos,int yPos,uint8_t odata )
{
	uint8_t iPrevious = *( m_pPixels + yPos * Display::GetWidth() + xPos );
	uint8_t iCurrent = ( * ( m_pPixels + yPos * Display::GetWidth() + xPos ) ^= odata );
	m_bDirtyFrame |= ( iCurrent != iPrevious );
}

bool Display::_IsPixelErase( int xPos,int yPos )
{
	return *( m_pPixels + yPos * Display::GetWidth() + xPos ) == 255;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void Display::framebuffer_size_callback( GLFWwindow* m_pWindow,int width,int height )
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport( 0,0,width,height );

	glBindTexture( GL_TEXTURE_2D,m_iFBOTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,NULL );//need to reallocate texture

	glBindTexture( GL_TEXTURE_2D,0 );

	m_bDirtyFrame = true;
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

void Display::DrawPixelAtPos( const KeyDisplayAccess& oKey,uint8_t xPos,uint8_t yPos,uint8_t oValue,bool& bErased,bool bClipping )
{
	for( uint8_t ByteMask = 0x80; ByteMask > 0; ByteMask >>= 1,++xPos )//uint8_t mask 0x80 --> 10000000 // 01000000 // 00100000 ...
	{
		if( bClipping )
		{
			if( xPos >= Display::GetWidth() || yPos >= Display::GetHeight() )
				return;
		}
		else
		{
			xPos &= Display::GetWidth() - 1;
		}

		if( oValue & ByteMask )
		{
			bErased |= _IsPixelErase( xPos,yPos );
			_XORedPixelsData( xPos,yPos,255 );
		}
	}
}

void Display::Update( const std::chrono::steady_clock::time_point& time,bool cpuPaused )
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

		glClearColor( 0.f,0.f,0.f,1.f );
		glClear( GL_COLOR_BUFFER_BIT );

		if( m_bDirtyFrame )
		{
			glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );

			glBindTexture( GL_TEXTURE_2D,m_iTexture );
			glTexSubImage2D( GL_TEXTURE_2D,0,0,0,Display::GetWidth(),Display::GetHeight(),GL_RED,GL_UNSIGNED_BYTE,m_pPixels );

			m_sShaderProgram.Use();
			glDrawElements( GL_TRIANGLES,6,GL_UNSIGNED_INT,0 );

			glBindFramebuffer( GL_FRAMEBUFFER,0 );

			m_bDirtyFrame = false;
		}

#ifdef DEBUG_INFO
		Chip8_Debugger::GetInstance()->Update( startElapsed );
		Chip8_Debugger::GetInstance()->Render();
#endif

		glfwSwapBuffers( m_pWindow );
	}
}