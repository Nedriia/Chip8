#pragma once

class Display
{

public:
	Display();
	int Init();
	int CreateWindowChip();
	void DestroyWindow();
	void Update( bool& quit );

private:
};
