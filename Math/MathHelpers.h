//--------------------------------------------------------------------------------------
// Math convenience functions
//--------------------------------------------------------------------------------------

#ifndef _MATH_HELPERS_H_DEFINED_
#define _MATH_HELPERS_H_DEFINED_

#include <cmath>
#include <numbers>
#include <type_traits>

// Special type to handle situations where floating point numbers are exceptionally required in integer-based template classes
// For example a Vector2i (2D vector with int coords) needs a length function that returns a float not an int, so this type is used
// Specifically: 
//     FloatTypeFor<int>    is float     (also <char>, <short>, <long int> and any other type etc.)
//     FloatTypeFor<float>  is float
//     FloatTypeFor<double> is double
template <typename T> using FloatTypeFor = std::conditional_t<std::is_floating_point_v<T>, T, float>;



// Test if a floating point value is approximately 0
template <typename T> bool IsZero(const T x);


// 1 / Sqrt. Used often (e.g. normalising) and can be optimised, so it gets its own function
// Supports int, float and double. Use of conditional_t (advanced topic) ensures int version returns a float result (float/double return same type)
template <typename T, typename U = std::conditional_t<std::is_integral_v<T>, float, T>> constexpr U InvSqrt(const T x)
{
	return 1 / std::sqrt(x);
}


// Pass an angle in degrees, returns the angle in radians
// Supports int, float and double. Use of conditional_t (advanced topic) ensures int version returns a float result (float/double return same type)
template <typename T, typename U = std::conditional_t<std::is_integral_v<T>, float, T>> constexpr U ToRadians(T d)
{
	return static_cast<U>(d) * std::numbers::pi_v<U> / 180;
}

// Pass an angle in radians, returns the angle in degrees
// Supports int, float and double. Use of conditional_t (advanced topic) ensures int version returns a float result (float/double return same type)
template <typename T, typename U = std::conditional_t<std::is_integral_v<T>, float, T>> constexpr U ToDegrees(T r)
{
	return static_cast<U>(r) * 180 / std::numbers::pi_v<U>;
}


// Return random number from a to b (inclusive) - will return int, float or double random number matching the type of the parameters a & b
template <typename T> T Random(const T a, const T b);


#endif // _MATH_HELPERS_H_DEFINED_
