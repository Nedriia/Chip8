#pragma once
#include "string"
#include <sstream>
#include "HexEditor/src/HexEditor_ImGUI.h"

class Chip8;
class Chip8_Debugger
{
public:
	Chip8_Debugger();
	~Chip8_Debugger();

	void Init( GLFWwindow* mainWindow,const Chip8* pCPU );
	void Update( const double* time );
	void Render();
	void Destroy();

	static Chip8_Debugger* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new Chip8_Debugger;
		return m_pSingleton;
	}
	const Chip8* GetCPU() const {return m_pCPU; }
	std::unique_ptr<HexEditor_ImGUI>& GetHexEditor() const { return m_oHexEditor; }

private:
	template< typename T >
	void FormatDebugData( std::string sText,const char* sFormat, const T& oData, int& iIndexSelectable, int& iIndexPosition );

	static Chip8_Debugger*					m_pSingleton;
	static std::unique_ptr<HexEditor_ImGUI>	m_oHexEditor;

	GLFWwindow*							m_pWindow;
	const Chip8*						m_pCPU;
	int									m_iCycleIndex;

	int									m_iRegisterSelected;
	int									m_iMemorySelected;
	int									m_iStackSelected;

	bool								m_bFollowPc;
};