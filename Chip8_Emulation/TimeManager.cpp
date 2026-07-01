#include "TimeManager.h"
#include <thread>

using std::chrono::operator""ns;
using namespace std::chrono;

constexpr auto iEarlyWakeUp = 5555555ns; //Time to wake up early and busy wait the next frame
constexpr int  iMaxTickLimit = 5;

nanoseconds TimeManager::s_iAccumulator{0 };
nanoseconds TimeManager::s_iCurrentTick{ 16666666ns };
double TimeManager::s_iTimeLastFrame = 0;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
inline void cpu_relax() { _mm_pause(); }
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
inline void cpu_relax() { __asm__ __volatile__("yield"); }
#else
inline void cpu_relax() { /* rien, ou std::this_thread::yield() en dernier recours */ }
#endif

void TimeManager::HandleTime( const steady_clock::time_point& start )
{
	steady_clock::time_point iTimeAfterMainLoop = steady_clock::now();
	nanoseconds iElapsed { iTimeAfterMainLoop - start };

	nanoseconds iTimeToFill = ( s_iCurrentTick - iElapsed  );
	if ( iTimeToFill >= iEarlyWakeUp ) // Still some time available, check if we can sleep
	{
		nanoseconds iExtraTime = ( iTimeToFill - iEarlyWakeUp );
		if ( s_iAccumulator > nanoseconds::zero() )
		{
			if ( s_iAccumulator > s_iCurrentTick * iMaxTickLimit ) //Too much we skip
			{
				iExtraTime		= s_iAccumulator = nanoseconds::zero();
			}
			else
			{
				if ( iTimeToFill >= s_iAccumulator )
				{
					iExtraTime = iTimeToFill - s_iAccumulator; //iTimeToFill can absorb the whole accumulator the next frame
					s_iAccumulator = nanoseconds::zero();
				}
				else
				{
					s_iAccumulator -= iTimeToFill;
					iExtraTime = nanoseconds::zero();
				}
			}
		}

		std::this_thread::sleep_until( iTimeAfterMainLoop + iExtraTime );
	}

	while ( steady_clock::now() <= start + s_iCurrentTick )
	{
		cpu_relax();
	}; //Busy waiting

	s_iTimeLastFrame = 	duration<double, std::milli> ( steady_clock::now() - start ).count();
}

void TimeManager::SetRefreshTick( const double& iTick )
{
	s_iCurrentTick = duration_cast<nanoseconds>( duration<double>( iTick ) );
}
