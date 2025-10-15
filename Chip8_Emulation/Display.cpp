#include "Display.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// settings
const uint16_t WINDOW_WIDTH = 800;
const uint16_t WINDOW_HEIGHT = 600;

constexpr uint8_t CHIP8_DISPLAY_WIDTH = 64;
constexpr uint8_t CHIP8_DISPLAY_HEIGHT = 32;

static uint8_t pixels[ CHIP8_DISPLAY_WIDTH ][ CHIP8_DISPLAY_HEIGHT ] = { 0 };

float vertices[] = {
	// positions		//Texture coords
	1.f, 1.0f, 0.0f,	1.0f, 0.0f,    // top right
	1.0f, -1.0f, 0.0f,	1.0f, 1.0f,   // bottom right
	-1.0f, -1.0f, 0.0f,	0.0f, 1.0f,   // bottom left 
	-1.0f, 1.0f, 0.0f,	0.0f, 0.0f,    // top left
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
	EBO( 0 )
{
}

int Display::Init()
{
	// glfw: initialize and configure
		// ------------------------------
	glfwInit();
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

#ifdef __APPLE__
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
#endif

	_CreateWindowChip();
	_InitRenderer();
	_InitTexture();
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
	unsigned int texture;
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	memset( pixels, 0, CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, pixels );
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
	glDeleteTextures( 1, &texture );
	_DestroyRenderer();
	shaderProgram.Delete();
	glfwTerminate();
}

void Display::Update( bool& quit )
{
	// render loop
		// -----------
	while( !glfwWindowShouldClose( window ) )
	{
		// input
		// -----
		processInput( window );

		// render
		// ------
		glClearColor( 0.f, 0.f, 0.f, 0.f );
		glClear( GL_COLOR_BUFFER_BIT );

		//Draw
		shaderProgram.Use();
		glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers( window );
		glfwPollEvents();
	}

	quit = true;
}