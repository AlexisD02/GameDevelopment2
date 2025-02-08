//--------------------------------------------------------------------------------------
// Vector3 class, encapsulating (x,y,z) coordinates and supporting functions
// Supports float (Vector3), double (Vector3d) and int (Vector3i)
//--------------------------------------------------------------------------------------

#include "Vector3.h"
#include <cmath>


/*-----------------------------------------------------------------------------------------
    Operators
-----------------------------------------------------------------------------------------*/

// Addition of another vector to this one, e.g. Position += Velocity
template <typename T> Vector3T<T>& Vector3T<T>::operator+= (const Vector3T<T>& v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

// Subtraction of another vector from this one, e.g. Velocity -= Gravity
template <typename T> Vector3T<T>& Vector3T<T>::operator-= (const Vector3T<T>& v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

// Negate this vector (e.g. Velocity = -Velocity)
template <typename T> Vector3T<T>& Vector3T<T>::operator- ()
{
    x = -x;
    y = -y;
    z = -z;
    return *this;
}

// Plus sign in front of vector - called unary positive and usually does nothing. Included for completeness (e.g. Velocity = +Velocity)
template <typename T> Vector3T<T>& Vector3T<T>::operator+ ()
{
    return *this;
}


// Multiply vector by scalar (scales vector) - use of extra template parameter means type of scalar doesn't have to match coordinate type
template <typename T> Vector3T<T>& Vector3T<T>::operator*= (FloatTypeFor<T> s)
{
    x = static_cast<T>(x * s);
    y = static_cast<T>(y * s);
    z = static_cast<T>(z * s);
    return *this;
}
// Divide vector by scalar (scales vector) - use of extra template parameter means type of scalar doesn't have to match coordinate type
template <typename T> Vector3T<T>& Vector3T<T>::operator/= (FloatTypeFor<T> s)
{
    x = static_cast<T>(x / s);
    y = static_cast<T>(y / s);
    z = static_cast<T>(z / s);
    return *this;
}


/*-----------------------------------------------------------------------------------------
    Non-member operators
-----------------------------------------------------------------------------------------*/

// Vector-vector addition
template <typename T> Vector3T<T> operator+ (const Vector3T<T>& v, const Vector3T<T>& w)
{
    return { v.x + w.x, v.y + w.y, v.z + w.z };
}

// Vector-vector subtraction
template <typename T> Vector3T<T> operator- (const Vector3T<T>& v, const Vector3T<T>& w)
{
    return { v.x - w.x, v.y - w.y, v.z - w.z };
}

// Vector-scalar multiplication - use of extra template parameter means type of scalar doesn't have to match coordinate type
template <typename T> Vector3T<T> operator* (const Vector3T<T>& v, FloatTypeFor<T> s)
{
    return { static_cast<T>(v.x * s), static_cast<T>(v.y * s), static_cast<T>(v.z * s) };
}
template <typename T> Vector3T<T> operator* (FloatTypeFor<T> s, const Vector3T<T>& v)
{
    return { static_cast<T>(v.x * s), static_cast<T>(v.y * s), static_cast<T>(v.z * s) };
}

// Vector-scalar division - use of extra template parameter means type of scalar doesn't have to match coordinate type
template <typename T> Vector3T<T> operator/ (const Vector3T<T>& v, FloatTypeFor<T> s)
{
    return { static_cast<T>(v.x / s), static_cast<T>(v.y / s), static_cast<T>(v.z / s) };
}


/*-----------------------------------------------------------------------------------------
    Other member functions
-----------------------------------------------------------------------------------------*/

// Returns length of a vector
template <typename T> FloatTypeFor<T> Vector3T<T>::Length() const
{
    return static_cast<FloatTypeFor<T>>(std::sqrt(x * x + y * y + z * z));
}



/*-----------------------------------------------------------------------------------------
    Other non-member functions
-----------------------------------------------------------------------------------------*/

// Dot product of two given vectors (order not important) - non-member version
template <typename T> T Dot(const Vector3T<T>& v1, const Vector3T<T>& v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// Cross product of two given vectors (order is important) - non-member version
template <typename T> Vector3T<T> Cross(const Vector3T<T>& v1, const Vector3T<T>& v2)
{
    return { v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x };
}

// Return unit length vector in the same direction as given one
template <typename T> Vector3T<T> Normalise(const Vector3T<T>& v)
{
    T lengthSq = v.x*v.x + v.y*v.y + v.z*v.z;

    // Can't normalise zero length vector
    if (IsZero(lengthSq))
    {
        return { 0, 0, 0 };
    }
    else
    {
        T invLength = InvSqrt(lengthSq);
        return { v.x * invLength, v.y * invLength, v.z * invLength };
    }
}

// Performs linear interpolation between two values.
template<typename T> Vector3T<T> Lerp(const Vector3T<T>& a, const Vector3T<T>& b, float t)
{
    return a + (b - a) * t;
}

// Returns a normalized vector from the given offset if the distance is above a small threshold.
template<typename T> Vector3T<T> OffsetNorm(const Vector3T<T>& offset, float dist)
{
    if (dist < 0.0001f)
    {
        return { 0, 0, 0 };
    }
    return offset / dist;
}


/*-----------------------------------------------------------------------------------------
    Template Instatiation
-----------------------------------------------------------------------------------------*/
// Instantiate this template class for specific numeric types. Prevents use of other types and
// allows us to put code in this cpp file (normally template code must be all in the header file)

template class Vector3T<float>;  // Vector3 or Vector3f
template class Vector3T<double>; // Vector3d
template class Vector3T<int>;    // Vector3i

// Also need to instantiate all the non-member template functions
template Vector3 operator+ (const Vector3& v, const Vector3& w);
template Vector3 operator- (const Vector3& v, const Vector3& w);
template Vector3 operator* (const Vector3& v, float s);
template Vector3 operator* (float s, const Vector3& v);
template Vector3 operator/ (const Vector3& v, float s);
template float   Dot  (const Vector3& v1, const Vector3& v2);
template Vector3 Cross(const Vector3& v1, const Vector3& v2);
template Vector3 Normalise(const Vector3& v);
template Vector3 Lerp(const Vector3& a, const Vector3& b, float t);
template Vector3 OffsetNorm(const Vector3& offset, float dist);

template Vector3d operator+ (const Vector3d& v, const Vector3d& w);
template Vector3d operator- (const Vector3d& v, const Vector3d& w);
template Vector3d operator* (const Vector3d& v, double s);
template Vector3d operator* (double s, const Vector3d& v);
template Vector3d operator/ (const Vector3d& v, double s);
template double   Dot  (const Vector3d& v1, const Vector3d& v2);
template Vector3d Cross(const Vector3d& v1, const Vector3d& v2);
template Vector3d Normalise(const Vector3d& v);
template Vector3d Lerp(const Vector3d& a, const Vector3d& b, float t);
template Vector3d OffsetNorm(const Vector3d& offset, float dist);

template Vector3i operator+ (const Vector3i& v, const Vector3i& w);
template Vector3i operator- (const Vector3i& v, const Vector3i& w);
template Vector3i operator* (const Vector3i& v, float s);
template Vector3i operator* (float s, const Vector3i& v);
template Vector3i operator/ (const Vector3i& v, float s);
template int      Dot(const Vector3i& v1, const Vector3i& v2);
template Vector3i Cross(const Vector3i& v1, const Vector3i& v2);
// Normalise does not make sense for Vector3i (integer coordinates) so not included
