#ifndef CHIP8_EMULATION_TIMEMANAGER_H
#define CHIP8_EMULATION_TIMEMANAGER_H
#include <chrono>

typedef std::chrono::nanoseconds nanoseconds;
class TimeManager
{
public:

	TimeManager(){};
	~TimeManager(){};

	static void HandleTime( const std::chrono::steady_clock::time_point& start );

	static const double* GetTimeLastFrame() { return &s_iTimeLastFrame; }
	static nanoseconds* GetRefreshTick() { return &s_iCurrentTick; }

	static void SetRefreshTick( const double& iTick );

private:
	static nanoseconds  s_iAccumulator;
	static nanoseconds	s_iCurrentTick;
	static double		s_iTimeLastFrame;
};


#endif //CHIP8_EMULATION_TIMEMANAGER_H
