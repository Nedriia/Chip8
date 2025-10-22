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

constexpr uint8_t CHIP8_DISPLAY_WIDTH = 64;
constexpr uint8_t CHIP8_DISPLAY_HEIGHT = 32;

static uint8_t pixels[ CHIP8_DISPLAY_HEIGHT ][ CHIP8_DISPLAY_WIDTH ] = { 0 }; //Watch out, first element in 2D are lines

#define FPS_LOCK 60.0f

static Chip8_Debugger oDebugger;

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
	FBO( 0 ),
	lastTime( 0.0 )
{
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
	//_InitRenderer();
	_InitTexture();
	_InitFramebuffer();

	oDebugger.Init( window );

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

	memset( pixels, 0, CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT );
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

void Display::UpdateTexture()
{
	
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

void Display::ClearScreen()
{
	memset( pixels, 0, sizeof( pixels ) );
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
			bErased |= pixels[ yPos ][ iPosLine ] == 255;
			pixels[ yPos ][ iPosLine ] ^= 255;
		}
	}
}

void Display::RenderInTexture()
{
	glBindTexture( GL_TEXTURE_2D, texture );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, pixels );
	ImGui::Image( ( ImTextureID )( intptr_t )texture, ImVec2( 900, 600 ) );
}

void Display::Update( bool& quit )
{
	// render loop
	// -----------
	if( !glfwWindowShouldClose( window ) )
	{
		// input
		// -----
		processInput( window );

		/*double now = glfwGetTime();
		if( ( now - lastTime ) > double( 1 / FPS_LOCK ) )
		{*/
			glBindFramebuffer( GL_FRAMEBUFFER, FBO );
			
			// render
			// ------
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::SetNextWindowPos( ImVec2( 0, 0 ), ImGuiCond_Always, ImVec2( 0, 0 ) );
			ImGui::SetNextWindowSize( ImVec2( 600, 1000 ), ImGuiCond_Once );
			if( ImGui::Begin( "CPU", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
			{
			}
			ImGui::End();

			ImGui::SetNextWindowPos( ImVec2( 600, 0 ), ImGuiCond_Always, ImVec2( 0, 0 ) );
			ImGui::SetNextWindowSize( ImVec2( 900, 600 ), ImGuiCond_Once );
			if( ImGui::Begin( "Chip-8 Display", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar ) )
			{
				RenderInTexture();
			}
			ImGui::End();

			ImGui::SetNextWindowPos( ImVec2( 600, 600 ), ImGuiCond_Always, ImVec2( 0, 0 ) );
			ImGui::SetNextWindowSize( ImVec2( 300, 400 ), ImGuiCond_Once );
			if( ImGui::Begin( "Inputs", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar ) )
			{
			}
			ImGui::End();

			ImGui::SetNextWindowPos( ImVec2( 900, 600 ), ImGuiCond_Always, ImVec2( 0, 0 ) );
			ImGui::SetNextWindowSize( ImVec2( 600, 400 ), ImGuiCond_Once );
			if( ImGui::Begin( "Memory", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar ) )
			{
			}
			ImGui::End();

			ImGui::SetNextWindowPos( ImVec2( 1500, 0 ), ImGuiCond_Always, ImVec2( 0, 0 ) );
			ImGui::SetNextWindowSize( ImVec2( 500, 1000 ), ImGuiCond_Once );
			if( ImGui::Begin( "Opcode live view", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
			{
			}
			ImGui::End();
			
			glBindFramebuffer( GL_FRAMEBUFFER, 0 );

			glClearColor( 0.f, 0.f, 0.f, 1.f );
			glClear( GL_COLOR_BUFFER_BIT );

			//shaderProgram.Use();

			oDebugger.Render();

			glfwSwapBuffers( window );
			//lastTime = now;
		//}

		glfwPollEvents();
	}
	else
		quit = true;
}