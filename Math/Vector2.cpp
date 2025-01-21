//--------------------------------------------------------------------------------------
// Vector2 class, encapsulating (x,y) coordinate and supporting functions
// Ssupports, float (Vector2), double (Vector2d) and int (Vector2i)
//--------------------------------------------------------------------------------------

#include "Vector2.h"
#include <cmath>


/*-----------------------------------------------------------------------------------------
    Member operators
-----------------------------------------------------------------------------------------*/

// Addition of another vector to this one, e.g. Position += Velocity
template <typename T> Vector2T<T>& Vector2T<T>::operator+= (const Vector2T<T>& v)
{
    x += v.x;
    y += v.y;
    return *this;
}

// Subtraction of another vector from this one, e.g. Velocity -= Gravity
template <typename T> Vector2T<T>& Vector2T<T>::operator-= (const Vector2T<T>& v)
{
    x -= v.x;
    y -= v.y;
    return *this;
}

// Negate this vector (e.g. Velocity = -Velocity)
template <typename T> Vector2T<T>& Vector2T<T>::operator- ()
{
    x = -x;
    y = -y;
    return *this;
}

// Plus sign in front of vector - called unary positive and usually does nothing. Included for completeness (e.g. Velocity = +Velocity)
template <typename T> Vector2T<T>& Vector2T<T>::operator+ ()
{
    return *this;
}

// Multiply vector by scalar (scales vector);
template <typename T> Vector2T<T>& Vector2T<T>::operator*= (FloatTypeFor<T> s)
{
    x = static_cast<T>(x * s);
    y = static_cast<T>(y * s);
    return *this;
}

// Divide vector by scalar (scales vector);
template <typename T> Vector2T<T>& Vector2T<T>::operator/= (FloatTypeFor<T> s)
{
    x = static_cast<T>(x / s);
    y = static_cast<T>(y / s);
    return *this;
}


/*-----------------------------------------------------------------------------------------
    Non-member operators
-----------------------------------------------------------------------------------------*/

// Vector-vector addition
template <typename T> Vector2T<T> operator+ (const Vector2T<T>& v, const Vector2T<T>& w)
{
    return { v.x + w.x, v.y + w.y };
}

// Vector-vector subtraction
template <typename T> Vector2T<T> operator- (const Vector2T<T>& v, const Vector2T<T>& w)
{
    return { v.x - w.x, v.y - w.y };
}

// Vector-scalar multiplication - use of extra template parameter means type of scalar doesn't have to match coordinate type
template <typename T> Vector2T<T> operator* (const Vector2T<T>& v, FloatTypeFor<T> s)
{
    return { static_cast<T>(v.x * s), static_cast<T>(v.y * s) };
}
template <typename T> Vector2T<T> operator* (FloatTypeFor<T> s, const Vector2T<T>& v)
{
    return { static_cast<T>(v.x * s), static_cast<T>(v.y * s) };
}

// Vector-scalar division - use of extra template parameter means type of scalar doesn't have to match coordinate type
template <typename T> Vector2T<T> operator/ (const Vector2T<T>& v, FloatTypeFor<T> s)
{
    return { static_cast<T>(v.x / s), static_cast<T>(v.y / s) };
}


/*-----------------------------------------------------------------------------------------
    Other member functions
-----------------------------------------------------------------------------------------*/

// Returns length of a vector
template <typename T> FloatTypeFor<T> Vector2T<T>::Length() const
{
    return static_cast<FloatTypeFor<T>>(std::sqrt(x * x + y * y));
}


/*-----------------------------------------------------------------------------------------
    Other non-member functions
-----------------------------------------------------------------------------------------*/

// Dot product of two given vectors (order not important) - non-member version
template <typename T> T Dot(const Vector2T<T>& v1, const Vector2T<T>& v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

// Return unit length vector in the same direction as given one
template <typename T> Vector2T<T> Normalise(const Vector2T<T>& v)
{
    T lengthSq = v.x*v.x + v.y*v.y;

    // Can't normalise zero length vector
    if (IsZero(lengthSq))
    {
        return { 0, 0 };
    }
    else
    {
        T invLength = InvSqrt(lengthSq);
        return { v.x * invLength, v.y * invLength };
    }
}


/*-----------------------------------------------------------------------------------------
    Template Instatiation
-----------------------------------------------------------------------------------------*/
// Instantiate this template class for specific numeric types. Prevents use of other types and
// allows us to put code in this cpp file (normally template code must be all in the header file)

template class Vector2T<float>;  // Vector2 or Vector2f
template class Vector2T<double>; // Vector2d
template class Vector2T<int>;    // Vector2i

// Also need to instantiate all the non-member template functions
template Vector2 operator+ (const Vector2& v, const Vector2& w);
template Vector2 operator- (const Vector2& v, const Vector2& w);
template Vector2 operator* (const Vector2& v, float s);
template Vector2 operator* (float s, const Vector2& v);
template Vector2 operator/ (const Vector2& v, float s);
template float   Dot(const Vector2& v1, const Vector2& v2);
template Vector2 Normalise(const Vector2& v);

template Vector2d operator+ (const Vector2d& v, const Vector2d& w);
template Vector2d operator- (const Vector2d& v, const Vector2d& w);
template Vector2d operator* (const Vector2d& v, double s);
template Vector2d operator* (double s, const Vector2d& v);
template Vector2d operator/ (const Vector2d& v, double s);
template double   Dot(const Vector2d& v1, const Vector2d& v2);
template Vector2d Normalise(const Vector2d& v);

template Vector2i operator+ (const Vector2i& v, const Vector2i& w);
template Vector2i operator- (const Vector2i& v, const Vector2i& w);
template Vector2i operator* (const Vector2i& v, float s);
template Vector2i operator* (float s, const Vector2i& v);
template Vector2i operator/ (const Vector2i& v, float s);
template int Dot(const Vector2i& v1, const Vector2i& v2);
// Normalise does not make sense for Vector2i (integer coordinates) so not included
