//--------------------------------------------------------------------------------------
// Timer class - works like a stopwatch
//--------------------------------------------------------------------------------------
// Uses new std::chrono classes

#ifndef _TIMER_H_INCLUDED_
#define _TIMER_H_INCLUDED_

#include <chrono>

class Timer
{
public:
	/*-----------------------------------------------------------------------------------------
		Constructor
	-----------------------------------------------------------------------------------------*/

	Timer();

	
	/*-----------------------------------------------------------------------------------------
		Timer control
	-----------------------------------------------------------------------------------------*/

	// Start the timer running
	void Start();

	// Stop the timer running
	void Stop();

	// Reset the timer to zero
	void Reset();


	/*-----------------------------------------------------------------------------------------
		Timing
	-----------------------------------------------------------------------------------------*/

	// Get time passed (seconds) since since timer was started or last reset
	float GetTime();

	// Get time passed (seconds) since last call to this function. If this is the first call, then
	// the time since timer was started or the last reset is returned
	float GetLapTime();


	/*-----------------------------------------------------------------------------------------
		Private member data
	-----------------------------------------------------------------------------------------*/
private:
	// Is the timer running
	bool mRunning;

	using Clock = std::chrono::steady_clock;
	Clock mClock;
	Clock::time_point mStart;
	Clock::time_point mLap;
	Clock::time_point mStop;
};


#endif //_TIMER_H_INCLUDED_
