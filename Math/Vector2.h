//--------------------------------------------------------------------------------------
// Vector2 class, encapsulating (x,y) coordinate and supporting functions
// Supports, float (Vector2), double (Vector2d) and int (Vector2i)
//--------------------------------------------------------------------------------------
// Template classes usually put all code in header file. However, here we use explicit template instantiation at the
// end of the .cpp file (see the comment there), which allows us to put the code in the cpp file and just the class
// interface here, just like an ordinary non-templated class

#ifndef _VECTOR2_H_DEFINED_
#define _VECTOR2_H_DEFINED_

#include "MathHelpers.h"


// Template class to support float, double or int coordinates. Do not use this typename, use the simpler types below
template <typename T> class Vector2T;

// Define convenient names for vectors of different types to avoid the messy angle bracket syntax
using Vector2i = Vector2T<int>;    // 2D vector with int coordinates
using Vector2f = Vector2T<float>;  // 2D vector with float coords
using Vector2d = Vector2T<double>; // 2D vector with double coords
using Vector2 = Vector2f; // Simple name for 2D vector with float coords


// Full declaration
template <typename T> class Vector2T
{
// Allow public access for such a simple, well-defined class
public:
    // Vector components
    T x;
    T y;

    /*-----------------------------------------------------------------------------------------
        Constructors
    -----------------------------------------------------------------------------------------*/

    // Default constructor - leaves values uninitialised (for performance)
    #pragma warning(suppress: 26495) // disable warning about constructor leaving things uninitialised ("suppress" affects next line only)
    Vector2T() {}

    // Construct with 2 values
    Vector2T(const T xIn, const T yIn)
    {
        x = xIn;
        y = yIn;
    }

    // Construct using a pointer to 2 values
    explicit Vector2T(const T* elts) // explicit means don't allow conversion from pointer to Vector2 without writing the constructor name
		                             // i.e. Vector2 v = somepointer; // Not allowed      Vector2 v = Vector2(somepointer); // OK, explicitly asked for constructor
    {
        // See comment on similar constructor in Matrix4x4 class regarding this kind of constructor
        x = elts[0];
        y = elts[1];
    }


    /*-----------------------------------------------------------------------------------------
        Member operators
    -----------------------------------------------------------------------------------------*/

    // Addition of another vector to this one, e.g. Position += Velocity
    Vector2T& operator+= (const Vector2T& v);

    // Subtraction of another vector from this one, e.g. Velocity -= Gravity
    Vector2T& operator-= (const Vector2T& v);

    // Negate this vector (e.g. Velocity = -Velocity)
    Vector2T& operator- ();

    // Plus sign in front of vector - called unary positive and usually does nothing. Included for completeness (e.g. Velocity = +Velocity)
    Vector2T& operator+ ();

	// Multiply vector by scalar (scales vector)
    // Integer vectors can be multiplied by a float but the resulting vector will rounded to integers
    Vector2T& operator*= (FloatTypeFor<T> s);

	// Divide vector by scalar (scales vector)
    // Integer vectors can be divided by a float but the resulting vector will rounded to integers
    Vector2T& operator/= (FloatTypeFor<T> s);


    /*-----------------------------------------------------------------------------------------
        Other member functions
    -----------------------------------------------------------------------------------------*/

    // Returns length of the vector (return value always floating point, even with an integer vector)
    FloatTypeFor<T> Length() const;
};


/*-----------------------------------------------------------------------------------------
    Non-member operators
-----------------------------------------------------------------------------------------*/
// Vector-vector addition
template <typename T> Vector2T<T> operator+ (const Vector2T<T>& v, const Vector2T<T>& w);

// Vector-vector subtraction
template <typename T> Vector2T<T> operator- (const Vector2T<T>& v, const Vector2T<T>& w);

// Vector-scalar multiplication & division
// Integer vectors can be multiplied/divided by a float but the resulting vector will rounded to integers
template <typename T> Vector2T<T> operator* (const Vector2T<T>& v, FloatTypeFor<T> s);
template <typename T> Vector2T<T> operator* (FloatTypeFor<T> s, const Vector2T<T>& v);
template <typename T> Vector2T<T> operator/ (const Vector2T<T>& v, FloatTypeFor<T> s);


/*-----------------------------------------------------------------------------------------
    Non-member functions
-----------------------------------------------------------------------------------------*/

// Distance between two Vector2 points
template <typename T> FloatTypeFor<T> Distance(const Vector2T<T>& v1, const Vector2T<T>& v2)  { return (v2 - v1).Length(); }

// Dot product of two given vectors (order not important) - non-member version
template <typename T> T Dot(const Vector2T<T>& v1, const Vector2T<T>& v2);

// Return unit length vector in the same direction as given one (not supported for int coordinates Vector2i)
template <typename T> Vector2T<T> Normalise(const Vector2T<T>& v);


#endif // _VECTOR2_H_DEFINED_
