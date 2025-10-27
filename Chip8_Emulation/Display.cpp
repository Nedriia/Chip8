#include "Display.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Chip8_Debugger.h"
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// settings
const uint16_t WINDOW_WIDTH = 2000;
const uint16_t WINDOW_HEIGHT = 1000;

const uint8_t Display::CHIP8_DISPLAY_WIDTH = 64;
const uint8_t Display::CHIP8_DISPLAY_HEIGHT = 32;
uint8_t* Display::pixels = nullptr;

Display *Display::singleton = nullptr;

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
	window( nullptr ),
	texture( 0 ),
	VAO( 0 ),
	VBO( 0 ),
	EBO( 0 ),
	FBO( 0 )
{
}

Display::~Display()
{
	delete singleton;
}

static void glfw_error_callback( int error, const char* description )
{
	fprintf( stderr, "GLFW Error %d: %s\n", error, description );
}

int Display::Init()
{
	glfwSetErrorCallback( glfw_error_callback );
	if( !glfwInit() )
		return -1;

	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

	_CreateWindowChip();
	//_InitRenderer(); //No more need since we are using FBO and rendering in ImGui Image
	pixels = new uint8_t[ CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT ];
	_InitTexture();
	_InitFramebuffer();

	Chip8_Debugger::GetInstance()->Init(window);

	return 0;
}

int Display::_CreateWindowChip()
{
	// glfw window creation
	// --------------------
	window = glfwCreateWindow( WINDOW_WIDTH, WINDOW_HEIGHT, "Chip8 Emulator", nullptr, nullptr );
	if( window == nullptr )
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent( window );
	glfwSetFramebufferSizeCallback( window, Display::framebuffer_size_callback );

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if( !gladLoadGLLoader( ( GLADloadproc )glfwGetProcAddress ) )
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	return 0;
}

void Display::_InitTexture()
{
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	_InitPixelsData();
	glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, pixels );
}

void Display::_InitFramebuffer()
{
	glGenFramebuffers( 1, &FBO );
	glBindFramebuffer( GL_FRAMEBUFFER, FBO );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0 ); //bind texture to framebuffer

	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
}

void Display::_InitRenderer()
{
	glGenVertexArrays( 1, &VAO );
	glBindVertexArray( VAO ); //Need to be bind before, any vertex attributs call will be stored inside the VAO
	glGenBuffers( 1, &VBO );
	glGenBuffers( 1, &EBO );

	glBindBuffer( GL_ARRAY_BUFFER, VBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), ( void* )0 ); //Vertices
	glEnableVertexAttribArray( 0 );

	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), ( void* )( 3 * sizeof( float ) ) ); //UV coords
	glEnableVertexAttribArray( 1 );

	glBindVertexArray( VAO );

	shaderProgram = Shader( "vertexShader.glsl", "fragmentShader.glsl" );
}

void Display::_DestroyRenderer()
{
	glDeleteVertexArrays( 1, &VAO );
	glDeleteBuffers( 1, &VBO );
	glDeleteBuffers( 1, &EBO );
	glDeleteFramebuffers( 1, &FBO );
}

void Display::_InitPixelsData()
{
	for( int i = 0; i < CHIP8_DISPLAY_HEIGHT; ++i )
	{
		for( int j = 0; j < CHIP8_DISPLAY_WIDTH; ++j )
			*( pixels + i * CHIP8_DISPLAY_WIDTH + j ) = 0;
	}
}

void Display::_XORedPixelsData( int xPos, int yPos, uint8_t odata )
{
	*( pixels + yPos * CHIP8_DISPLAY_WIDTH + xPos ) ^= odata;
}

bool Display::_IsPixelErase( int xPos, int yPos )
{
	return *( pixels + yPos * CHIP8_DISPLAY_WIDTH + xPos ) == 255;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void Display::framebuffer_size_callback( GLFWwindow* window, int width, int height )
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport( 0, 0, width, height );
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Display::processInput( GLFWwindow* window )
{
	if( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS )
		glfwSetWindowShouldClose( window, true );
}

void Display::DestroyWindow()
{
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	delete[] pixels;
	glDeleteTextures( 1, &texture );
	_DestroyRenderer();
	shaderProgram.Delete();
	glfwTerminate();
}

void Display::ClearScreen()
{
	_InitPixelsData();
}

void Display::DrawPixelAtPos( uint8_t xPos, uint8_t yPos, uint8_t oValue, bool& bErased )
{
	uint8_t ByteMask = 0x80; //uint8_t mask 0x80 --> 10000000 // 01000000 // 00100000 ...
	uint8_t iPosLine = xPos;
	for( ByteMask; ByteMask > 0; ByteMask >>= 1, ++iPosLine )
	{
		iPosLine %= CHIP8_DISPLAY_WIDTH;
		yPos %= CHIP8_DISPLAY_HEIGHT;
		if( oValue & ByteMask )
		{
			bErased |= _IsPixelErase( iPosLine, yPos );
			_XORedPixelsData( iPosLine, yPos, 255 );
		}
	}
}

void Display::Update( bool& quit, bool bRefreshFrame )
{
	// render loop
	// -----------
	if( !glfwWindowShouldClose( window ) )
	{
		// input
		// -----
		processInput( window );

		if( bRefreshFrame )
		{
			Chip8_Debugger::GetInstance()->Update();

			glClearColor( 0.f, 0.f, 0.f, 1.f );
			glClear( GL_COLOR_BUFFER_BIT );

			//shaderProgram.Use(); //We render into ImGui Image no more need for now ( keeping commented in case we are doing a fullscreen )

			Chip8_Debugger::GetInstance()->Render();

			glfwSwapBuffers( window );
		}

		glfwPollEvents();
	}
	else
		quit = true;
}