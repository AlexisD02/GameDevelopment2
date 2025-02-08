//--------------------------------------------------------------------------------------
// Vector3 class, encapsulating (x,y,z) coordinates and supporting functions
// Supports float (Vector3), double (Vector3d) and int (Vector3i)
//--------------------------------------------------------------------------------------
// Template classes usually put all code in header file. However, here we use explicit template instantiation at the
// end of the .cpp file (see the comment there), which allows us to put the code in the cpp file and just the class
// interface here, just like an ordinary non-templated class

#ifndef _VECTOR3_H_DEFINED_
#define _VECTOR3_H_DEFINED_

#include "MathHelpers.h"


// Template class to support float, double or int coordinates. Do not use this typename, use the simpler types below
template <typename T> class Vector3T;

// Define convenient names for vectors of different types to avoid the messy angle bracket syntax
using Vector3i = Vector3T<int>;    // 3D vector with int coordinates
using Vector3f = Vector3T<float>;  // 3D vector with float coords
using Vector3d = Vector3T<double>; // 3D vector with double coords
using Vector3 = Vector3f; // Simple name for 3D vector with float coords



// Full declaration
template <typename T> class Vector3T
{
// Allow public access for such a simple, well-defined class
public:
    // Vector components
    T x;
	T y;
	T z;

    /*-----------------------------------------------------------------------------------------
        Constructors
    -----------------------------------------------------------------------------------------*/

	// Default constructor - leaves values uninitialised (for performance)
    #pragma warning(suppress: 26495) // disable warning about constructor leaving things uninitialised ("suppress" affects next line only)
    Vector3T() {}

	// Construct with 3 values
	Vector3T(const T xIn, const T yIn, const T zIn)
	{
		x = xIn;
		y = yIn;
		z = zIn;
	}
	
    // Construct using a pointer to three values
    explicit Vector3T(const T* elts) // explicit means don't allow conversion from pointer to Vector3 without writing the constructor name
                                     // i.e. Vector3 v = somepointer; // Not allowed      Vector3 v = Vector3(somepointer); // OK, explicitly asked for constructor
    {
        // See comment on similar constructor in Matrix4x4 class regarding this kind of constructor
        x = elts[0];
        y = elts[1];
        z = elts[2];
    }


    /*-----------------------------------------------------------------------------------------
        Member operators
    -----------------------------------------------------------------------------------------*/

    // Addition of another vector to this one, e.g. Position += Velocity
    Vector3T& operator+= (const Vector3T& v);

    // Subtraction of another vector from this one, e.g. Velocity -= Gravity
    Vector3T& operator-= (const Vector3T& v);

    // Negate this vector (e.g. Velocity = -Velocity)
    Vector3T& operator- ();

    // Plus sign in front of vector - called unary positive and usually does nothing. Included for completeness (e.g. Velocity = +Velocity)
    Vector3T& operator+ ();

    // Multiply vector by scalar (scales vector)
    // Integer vectors can be multiplied by a float but the resulting vector will rounded to integers
    Vector3T& operator*= (FloatTypeFor<T> s);

	// Divide vector by scalar (scales vector)
    // Integer vectors can be divided by a float but the resulting vector will rounded to integers
    Vector3T& operator/= (FloatTypeFor<T> s);


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
template <typename T> Vector3T<T> operator+ (const Vector3T<T>& v, const Vector3T<T>& w);

// Vector-vector subtraction
template <typename T> Vector3T<T> operator- (const Vector3T<T>& v, const Vector3T<T>& w);

// Vector-scalar multiplication & division
// Integer vectors can be multiplied/divided by a float but the resulting vector will rounded to integers
template <typename T> Vector3T<T> operator* (const Vector3T<T>& v, FloatTypeFor<T> s);
template <typename T> Vector3T<T> operator* (FloatTypeFor<T> s, const Vector3T<T>& v);
template <typename T> Vector3T<T> operator/ (const Vector3T<T>& v, FloatTypeFor<T> s);


/*-----------------------------------------------------------------------------------------
    Non-member functions
-----------------------------------------------------------------------------------------*/

// Distance between two Vector3 points
template <typename T> FloatTypeFor<T> Distance(const Vector3T<T>& v1, const Vector3T<T>& v2)  { return (v2 - v1).Length(); }

// Dot product of two given vectors (order not important) - non-member version
template <typename T> T Dot(const Vector3T<T>& v1, const Vector3T<T>& v2);

// Cross product of two given vectors (order is important) - non-member version
template <typename T> Vector3T<T> Cross(const Vector3T<T>& v1, const Vector3T<T>& v2);

// Return unit length vector in the same direction as given one
template <typename T> Vector3T<T> Normalise(const Vector3T<T>& v);

template <typename T> Vector3T<T> Lerp(const Vector3T<T>& a, const Vector3T<T>& b, float t);

template <typename T> Vector3T<T> OffsetNorm(const Vector3T<T>& offset, float dist);

#endif // _VECTOR3_H_DEFINED_
