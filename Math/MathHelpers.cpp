//--------------------------------------------------------------------------------------
// Math convenience functions
//--------------------------------------------------------------------------------------

#include "MathHelpers.h"

/*-----------------------------------------------------------------------------------------
	Float work
-----------------------------------------------------------------------------------------*/

// Test if a float value is approximately 0
// Epsilon value is the range around zero that is considered equal to zero
const float EPSILON32 = 0.5e-6f; // For 32-bit floats, requires zero to 6 decimal places
template<> bool IsZero<float>(const float x)
{
	return std::abs(x) < EPSILON32;
}

// Test if a double value is approximately 0
// Epsilon value is the range around zero that is considered equal to zero
const double EPSILON64 = 0.5e-15; // For 64-bit floats, requires zero to 15 decimal places
template<> bool IsZero<double>(const double x)
{
	return std::abs(x) < EPSILON64;
}


/*-----------------------------------------------------------------------------------------
	Random numbers
-----------------------------------------------------------------------------------------*/

// Return random integer from a to b (inclusive)
// Can only return up to RAND_MAX different values, spread evenly across the given range
// RAND_MAX is defined in stdlib.h and is compiler-specific (32767 on Visual Studio)
template<> int Random<int>(const int a, const int b)
{
	// Could just use a + rand() % (b-a), but using a more complex form to allow range
	// to exceed RAND_MAX and still return values spread across the range
	int t = (b - a + 1) * rand();
	return t == 0 ? a : a + (t - 1) / RAND_MAX;
}

// Return random float from a to b (inclusive)
// Can only return up to RAND_MAX different values, spread evenly across the given range
// RAND_MAX is defined in stdlib.h and is compiler-specific (32767 on Visual Studio)
template<> float Random<float>(const float a, const float b)
{
	return a + (b - a) * (static_cast<float>(rand()) / RAND_MAX);
}

// Return random double from a to b (inclusive)
// Can only return up to RAND_MAX different values, spread evenly across the given range
// RAND_MAX is defined in stdlib.h and is compiler-specific (32767 on Visual Studio)
template<> double Random<double>(const double a, const double b)
{
	return a + (b - a) * (static_cast<double>(rand()) / RAND_MAX);
}
