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
#include "Chip8.h"
#include <iomanip>
#include <format>
#include <chrono>
#include <iostream>

#define NULL_DATA_COLOR IM_COL32( 128,128,128,255 )
#define CHANGE_DATA_COLOR IM_COL32( 255,0,0,255 )
#define DEFAULT_DATA_COLOR IM_COL32( 255,255,255,180 )

Chip8_Debugger* Chip8_Debugger::singleton = nullptr;

Chip8_Debugger::Chip8_Debugger() :
	window( nullptr ),
	m_pCPU( nullptr )
{}

void Chip8_Debugger::Init( GLFWwindow* mainWindow,const Chip8* pCPU )
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

	if( pCPU != nullptr )
		m_pCPU = pCPU;
}

void Chip8_Debugger::Update( const std::chrono::microseconds& time )
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

	ImGuiIO& io = ImGui::GetIO();
	bool needsRedraw = ( io.MouseWheel != 0 )
		|| io.InputQueueCharacters.Size > 0
		|| ImGui::IsAnyItemActive();
	if( needsRedraw )
		Display::GetInstance()->SetFrameAsDirty();

	ImGui::SetNextWindowPos( ImVec2( 0,0 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 600,500 ),ImGuiCond_Once );
	if( ImGui::Begin( "CPU",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
	{
		if( m_pCPU != nullptr )
		{
			if( ImGui::BeginListBox( "#",ImVec2( -FLT_MIN,26 * ImGui::GetTextLineHeightWithSpacing() ) ) )
			{
				const Data<uint8_t>* it = m_pCPU->GetRegisters();
				if( it != nullptr )
				{
					for( int i = 0; i < 0x10; ++i )
					{
						ImGui::PushID( i );
						std::string sText= "V" + std::format( "{:X}:",i );
						FormatDebugData( sText,"%02X",it[i] );
						ImGui::PopID();
					}

					ImGui::NewLine();
					FormatDebugData( "I  :","%#06X",m_pCPU->GetI() );
					FormatDebugData( "SP :","%#06X",m_pCPU->GetSP() );
					FormatDebugData( "PC :","%#06X",m_pCPU->GetPC() );
					FormatDebugData( "DT :","%#06X",m_pCPU->GetDelayTimer() );
					FormatDebugData( "ST :","%#06X",m_pCPU->GetSoundTimer() );
				}
			}
			ImGui::EndListBox();
		}
	}
	ImGui::End();

	ImGui::SetNextWindowPos( ImVec2( 0,500 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 600,500 ),ImGuiCond_Once );
	if( ImGui::Begin( "Stack",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
	{
		if( m_pCPU != nullptr )
		{
			static int item_selected_idx = 0;
			if( ImGui::BeginListBox( "#",ImVec2( -FLT_MIN,26 * ImGui::GetTextLineHeightWithSpacing() ) ) )
			{
				const Data<uint16_t>* it = m_pCPU->GetStack();
				if( it != nullptr )
				{
					std::string sAdress;
					for( int i = 0; i < 0x10; ++i )
					{
						ImGui::PushID( i );
						FormatDebugData( "","%#06X",it[i]);
						ImGui::PopID();
					}
				}
			}
			ImGui::EndListBox();
		}
	}
	ImGui::End();

	{
		ImGui::SetNextWindowPos( ImVec2( 600,0 ),ImGuiCond_Always,ImVec2( 0,0 ) );
		ImGui::SetNextWindowSize( ImVec2( 900,600 ),ImGuiCond_Once );
		if( ImGui::Begin( "#ID",nullptr,ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar ) )
		{
			glBindTexture( GL_TEXTURE_2D,Display::GetInstance()->GetTexture() );
			glTexSubImage2D( GL_TEXTURE_2D,0,0,0,Display::CHIP8_DISPLAY_WIDTH,Display::CHIP8_DISPLAY_HEIGHT,GL_RED,GL_UNSIGNED_BYTE,Display::GetInstance()->GetPixels() );
			ImVec2 avail = ImVec2( ImGui::GetContentRegionAvail().x,ImGui::GetContentRegionAvail().y - 14 );
			ImGui::Image( ( ImTextureID )( intptr_t )Display::GetInstance()->GetTexture(),avail );
		}

		float width = ImGui::GetContentRegionAvail().x;
		const char* titleLeft = "Chip-8 Display";

		std::string textPerfDebug = std::format( "{} ms",std::chrono::duration<double,std::milli>( time ).count() );
		ImGui::TextColored( ImVec4( 0.7f,0.7f,0.7f,1.0f ),"%s",titleLeft );

		ImGui::SameLine( width - ImGui::CalcTextSize( textPerfDebug.c_str() ).x );
		ImGui::TextColored( ImVec4( 0.5f,0.5f,0.5f,1.0f ),"%s",textPerfDebug.c_str() );

		ImGui::End();
	}

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
		if( m_pCPU != nullptr )
		{
			static int item_selected_idx = 0;
			if( ImGui::BeginListBox( "#",ImVec2( -FLT_MIN,20 * ImGui::GetTextLineHeightWithSpacing() ) ) )
			{
				const Data<uint8_t>* it = m_pCPU->GetMemory();
				if( it != nullptr )
				{
					std::string sAdress;
					for( int i = 0; i < 0x1000; i += 0x10 )
					{
						sAdress.clear();

						ImGui::PushID( i );

						ImGui::Text( std::format( "{:#06X}:",i ).c_str() );
						ImGui::SameLine();

						bool is_selected = ( item_selected_idx == i );
						for( int n = i; n < i + 0x10; ++n )
						{
							ImGui::PushID( n );
							if( n == i + 8 )
							{
								ImGui::Text( "-" );
								ImGui::SameLine();
							}
							FormatDebugData( "","%02X",it[ n ] );
							ImGui::SameLine();
							ImGui::PopID();
						}
						ImGui::PopID();
						ImGui::NewLine();
					}
				}
			}
			ImGui::EndListBox();
		}
	}
	ImGui::End();

	ImGui::SetNextWindowPos( ImVec2( 1500,0 ),ImGuiCond_Always,ImVec2( 0,0 ) );
	ImGui::SetNextWindowSize( ImVec2( 500,1000 ),ImGuiCond_Once );
	if( ImGui::Begin( "Opcode Decode",nullptr,ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
	{
		if( m_pCPU != nullptr )
		{
			Chip8::KeyAccess oKey;
			Display::KeyDisplayAccess oKeyDisplay;

			static int item_selected_idx = 0;
			bool bPause = m_pCPU->IsPause();

			if( ImGui::Checkbox( "Pause",&bPause ) )
				m_pCPU->AskForState( oKey,bPause ? RunningState::Pause : RunningState::Running );
			ImGui::SameLine();

			static int iIndex = 0;
			if( ImGui::Button( "Step Next Frame" ) )
			{
				m_pCPU->AskForState( oKey,RunningState::StepNextFrame );
			}
			ImGui::SameLine();

			if( ImGui::Button( "Reset" ) )
			{
				Display::ClearScreen( oKeyDisplay );
				m_pCPU->AskForState( oKey,RunningState::Reset );
			}

			if( ImGui::BeginListBox( "#",ImVec2( -FLT_MIN,54 * ImGui::GetTextLineHeightWithSpacing() ) ) )
			{
				for( int n = 0; n < m_pCPU->GetHistoryOpcode().size(); ++n )
				{
					ImGui::PushID( n );
					bool is_selected = ( item_selected_idx == n );
					if( ImGui::Selectable( m_pCPU->GetHistoryOpcode()[ n ].c_str(),is_selected ) )
						item_selected_idx = n;

					ImGui::PopID();
				}

				if( ( m_pCPU->GetHistoryOpcode().size() > 0 && m_pCPU->IsRunning() ) || ( iIndex != m_pCPU->GetCycleId() ) )
				{
					ImGui::SetScrollHereY( 0.5f );

					item_selected_idx = m_pCPU->GetHistoryOpcode().size() - 1;
					iIndex = m_pCPU->GetCycleId();
					Display::GetInstance()->SetFrameAsDirty();
				}
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

template< typename T >
void Chip8_Debugger::FormatDebugData( std::string sText,const char* sFormat,const T& oData )
{
	static_assert( std::is_same_v<T,Data<uint8_t>> || std::is_same_v<T,Data<uint16_t>>,"Function is being call with an unsupported type" );

	if( !sText.empty() )
	{
		ImGui::Text( sText.c_str() );
		ImGui::SameLine();
	}
	ImGui::PushStyleColor( ImGuiCol_Text,oData.IsNULL() ? NULL_DATA_COLOR : oData.HasChanged() ? CHANGE_DATA_COLOR : DEFAULT_DATA_COLOR );

	char buffer[ 64 ];
	if constexpr( std::is_same_v<T,Data<uint8_t>> )
	{
		uint8_t val = ( uint8_t )oData;
		snprintf( buffer,sizeof( buffer ),sFormat,val );
	}
	else if constexpr( std::is_same_v<T,Data<uint16_t>> )
	{
		uint16_t val = ( uint16_t )oData;
		snprintf( buffer,sizeof( buffer ),sFormat,val );
	}

	oData.ClearFlag();
	ImGui::TextUnformatted( buffer );
	ImGui::PopStyleColor();
}