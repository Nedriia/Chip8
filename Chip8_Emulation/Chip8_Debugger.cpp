// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#include "Chip8_Debugger.h"
#include "Display.h"

Chip8_Debugger* Chip8_Debugger::singleton = nullptr;

Chip8_Debugger::Chip8_Debugger() :
	window( nullptr ),
	m_bFollowPc( true )
{
}

void Chip8_Debugger::Init( GLFWwindow* mainWindow )
{
	ImVec4 clear_color = ImVec4( 0.45f,0.55f,0.60f,1.00f );
	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor( glfwGetPrimaryMonitor() ); // Valid on GLFW 3.3+ only

	window = mainWindow;
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes( main_scale );        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL( mainWindow,true );
#ifdef __EMSCRIPTEN__
	ImGui_ImplGlfw_InstallEmscriptenCallbacks( mainWindow,"#canvas" );
#endif
	ImGui_ImplOpenGL3_Init( "#version 330" );
}

void Chip8_Debugger::Update()
{
	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	glfwPollEvents();
	if( glfwGetWindowAttrib( window,GLFW_ICONIFIED ) != 0 )
	{
		ImGui_ImplGlfw_Sleep( 10 );
		return;
	}

	// Start the Dear ImGui frame
	glBindFramebuffer( GL_FRAMEBUFFER,Display::GetInstance()->GetFBO() );

	// render
	// ------
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos( ImVec2( 0,0 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 600,500 ),ImGuiCond_Once );
	if( ImGui::Begin( "CPU",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
	{
	}
	ImGui::End();

	ImGui::SetNextWindowPos( ImVec2( 0,500 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 600,500 ),ImGuiCond_Once );
	if( ImGui::Begin( "Stack",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
	{
	}
	ImGui::End();

	ImGui::SetNextWindowPos( ImVec2( 600,0 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 900,600 ),ImGuiCond_Once );
	if( ImGui::Begin( "Chip-8 Display",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar ) )
	{
		glBindTexture( GL_TEXTURE_2D,Display::GetInstance()->GetTexture() );
		glTexSubImage2D( GL_TEXTURE_2D,0,0,0,Display::CHIP8_DISPLAY_WIDTH,Display::CHIP8_DISPLAY_HEIGHT,GL_RED,GL_UNSIGNED_BYTE,Display::GetInstance()->GetPixels() );
		ImVec2 avail = ImGui::GetContentRegionAvail();
		ImGui::Image( ( ImTextureID )( intptr_t )Display::GetInstance()->GetTexture(),avail );
	}
	ImGui::End();

	ImGui::SetNextWindowPos( ImVec2( 600,600 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 300,400 ),ImGuiCond_Once );
	if( ImGui::Begin( "Inputs",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar ) )
	{
	}
	ImGui::End();

	ImGui::SetNextWindowPos( ImVec2( 900,600 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 600,400 ),ImGuiCond_Once );
	if( ImGui::Begin( "Memory",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar ) )
	{
	}
	ImGui::End();

	ImGui::SetNextWindowPos( ImVec2( 1500,0 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 500,1000 ),ImGuiCond_Once );
	if( ImGui::Begin( "Opcode live view",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
	{
		static int item_selected_idx = 0;
		ImGui::Checkbox( "Follow PC : ",&m_bFollowPc );
		if( ImGui::BeginListBox( "#",ImVec2( -FLT_MIN,54 * ImGui::GetTextLineHeightWithSpacing() ) ) )
		{
			for( int n = 0; n < opcodeHistory.size(); ++n )
			{
				ImGui::PushID( n );
				bool is_selected = ( item_selected_idx == n );
				if( ImGui::Selectable( opcodeHistory[ n ].c_str(),is_selected ) )
					item_selected_idx = n;

				if( m_bFollowPc && n == opcodeHistory.size() - 1 )
				{
					ImGui::SetScrollHereY( 1.0f );
					item_selected_idx = opcodeHistory.size() - 1;
				}
				ImGui::PopID();
			}
			ImGui::EndListBox();
		}
	}
	ImGui::End();

	glBindFramebuffer( GL_FRAMEBUFFER,0 );
}

void Chip8_Debugger::Render()
{
	int display_w,display_h;
	glfwGetFramebufferSize( window,&display_w,&display_h );
	glViewport( 0,0,display_w,display_h );

	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}

void Chip8_Debugger::AddEntryOpcode( const char* pOpcode )
{
	if( opcodeHistory.size() >= 0x200 )
		opcodeHistory.pop_front();

	opcodeHistory.push_back( pOpcode );
}