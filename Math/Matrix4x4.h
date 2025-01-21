//--------------------------------------------------------------------------------------
// Matrix4x4 class specialised for affine matrices for 3D graphics
// Supports float (Matrix4x4) and double (Matrix4x4d)
//--------------------------------------------------------------------------------------
// Template classes usually put all code in header file. However, here we use explicit template instantiation at the
// end of the .cpp file (see the comment there), which allows us to put the code in the cpp file and just the class
// interface here, just like an ordinary non-templated class

#ifndef _MATRIX4X4_H_DEFINED_
#define _MATRIX4X4_H_DEFINED_

#include "Vector3.h"
#include "Vector4.h"


// Template class to support float or double values. Do not use this typename, use the simpler types below
template <typename T> class Matrix4x4T;

// Define convenient names for matrices of different types so we can avoid angle bracket syntax in main code
using Matrix4x4f = Matrix4x4T<float>;  // 4x4 Matrix with float values
using Matrix4x4d = Matrix4x4T<double>; // 4x4 Matrix with double values
using Matrix4x4 = Matrix4x4f; // Add extra simple name for float coords - the most common use-case


// Full declaration
template <typename T> class Matrix4x4T
{
// Allow public access for such a simple, well-defined class
public:
	// Matrix elements
	T e00, e01, e02, e03;
	T e10, e11, e12, e13;
	T e20, e21, e22, e23;
	T e30, e31, e32, e33;

 
    /*-----------------------------------------------------------------------------------------
        Constructors
    -----------------------------------------------------------------------------------------*/

    // Default constructor - leaves values uninitialised (for performance)
    #pragma warning(suppress: 26495) // disable warning about constructor leaving things uninitialised ("suppress" affects next line only)
    Matrix4x4T<T>() {}


    // Construct with 16 values
    Matrix4x4T<T>(T v00, T v01, T v02, T v03, 
                  T v10, T v11, T v12, T v13,
                  T v20, T v21, T v22, T v23,
                  T v30, T v31, T v32, T v33)
    {
        e00 = v00; e01 = v01; e02 = v02; e03 = v03; 
        e10 = v10; e11 = v11; e12 = v12; e13 = v13;
        e20 = v20; e21 = v21; e22 = v22; e23 = v23;
        e30 = v30; e31 = v31; e32 = v32; e33 = v33;
    }


    // Construct using a pointer to 16 values
    explicit Matrix4x4T<T>(T* elts) // explicit means don't allow conversion from pointer to Matrix4x4 without writing the constructor name
	                                // i.e. Matrix4x4 m = somepointer; // Not allowed      Matrix4x4 m = Matrix4x4(somepointer); // OK, explicitly asked for constructor
    {
        // Ugly, but useful constructor. Convert the pointer parameter to a matrix pointer then use the default
        // assignment = operator to copy that matrix to this one. Not guaranteed to be portable code but will
        // work in practice. There are more modern, standard ways to support this kind of constructor, but would
        // need advanced C++ work to do properly that would distract from understanding the basics of the class.
        // For production code the more advanced way would be better. Ask tutor if you want to know more.
        *this = *reinterpret_cast<Matrix4x4T<T>*>(elts);
    }


    // Construct matrix from position, Euler angles (x,y,z rotations) and scale (x,y & z separately)
    Matrix4x4T<T>(Vector3T<T> position, Vector3T<T> rotations, Vector3T<T> scale)
    {
        *this = MatrixScaling(scale) * MatrixRotation(rotations) * MatrixTranslation(position);
    }

    // Construct matrix from position, Euler angles (x,y,z rotations) and uniform scale
    // Scale and rotations have defaults, so only position required
    explicit Matrix4x4T<T>(Vector3T<T> position, Vector3T<T> rotations = { 0, 0, 0 }, float scale = 1) 
        : Matrix4x4T<T>(position, rotations, { scale, scale, scale }) {} // Forward to constructor above


    /*-----------------------------------------------------------------------------------------
        Data access
    -----------------------------------------------------------------------------------------*/

	// Direct access to the xyz values in a row (range 0-3) of the matrix using a Vector3
    // Returns a reference so can result be used to get or set values
    // E.g. float value = myMatrix.Row(3).y;   myMatrix.Row(1) = Vector3{0,1,0};
    Vector3T<T>& Row(int row)
    {
        // Non-standard conversion but works in practice and is very useful in this particular circumstance
        // As with the constructor conversion above the "correct" ways to do this need much more advanced C++
        return *reinterpret_cast<Vector3T<T>*>(&e00 + row * 4); 
    }

    // Direct access to the xyz values in a row (range 0-3) of the matrix using a const Vector3 reference
    // Used as getter where matrix is const
    const Vector3T<T>& Row(int row) const
    {
        return *reinterpret_cast<const Vector3T<T>*>(&e00 + row * 4);
    }

    // Direct access to X axis of matrix
    // Returns a reference so can be used to get or set (Vector3 v = myMatrix.XAxis();  or  myMatrix.XAxis() = {1,2,3};)
    Vector3T<T>& XAxis() { return Row(0); }

    // Direct access to Y axis of matrix
    // Returns a reference so can be used to get or set (Vector3 v = myMatrix.YAxis();  or  myMatrix.YAxis() = {1,2,3};)
    Vector3T<T>& YAxis() { return Row(1); }

    // Direct access to Z axis of matrix
    // Returns a reference so can be used to get or set (Vector3 v = myMatrix.ZAxis();  or  myMatrix.ZAxis() = {1,2,3};)
    Vector3T<T>& ZAxis() { return Row(2); }


    // Direct access to X axis of matrix - returns a const reference - used as getter where matrix is const
    const Vector3T<T>& XAxis() const { return Row(0); }

    // Direct access to Y axis of matrix - returns a const reference - used as getter where matrix is const
    const Vector3T<T>& YAxis() const { return Row(1); }

    // Direct access to Z axis of matrix - returns a const reference - used as getter where matrix is const
    const Vector3T<T>& ZAxis() const { return Row(2); }


    // Direct access to position of matrix
    // Returns a reference so can be used to get or set (Vector3 v = myMatrix.Position();  or  myMatrix.Position() = {1,2,3};)
    Vector3T<T>& Position()  { return Row(3); }

    // Direct access to position of matrix - returns a const reference - used as getter where matrix is const
    const Vector3T<T>& Position() const { return Row(2); }


    /*-----------------------------------------------------------------------------------------
       Getters/Setters
    -----------------------------------------------------------------------------------------*/

    // Converts rotational part of the matrix into Euler angles (X,Y,Z floats like TL-Engine). Getter only.
    // Assumes rotation order is Z first, then X, then Y
    Vector3T<T> GetRotation() const;

    // Sets rotation of the matrix from given Euler angles (X,Y,Z floats like TL-Engine). Does not affect position or scale.
    // Uses rotation order of Z first, then X, then Y
    void SetRotation(Vector3T<T> rotation);

    // Get current scale of the three x,y,z axes in the matrix. Getter only.
    Vector3T<T> GetScale() const { return { XAxis().Length(), YAxis().Length() , ZAxis().Length() }; }

    // Sets scaling of the matrix seperately in X,Y and Z. Does not affect position or rotation.
    void SetScale(Vector3T<T> scale);

    // Sets scaling of the matrix (same scaling in X,Y and Z). Does not affect position or rotation.
    void SetScale(T scale) { SetScale({ scale, scale, scale }); }


    /*-----------------------------------------------------------------------------------------
        Operators
    -----------------------------------------------------------------------------------------*/

	// Post-multiply this matrix by the given one
    Matrix4x4T<T>& operator*=(const Matrix4x4T<T>& m);

    // Assume given Vector3 is a point and transform by the given matrix
    // Returns a Vector4 (w=1 for point), but result can also be put into a Vector3 since Vector4 can cast to Vector3
    Vector4T<T> TransformPoint(const Vector3T<T>& v) const;

    // Assume given Vector3 is a vector and transform by the given matrix
    // Returns a Vector4 (w=0 for vector), but result can also be put into a Vector3 since Vector4 can cast to Vector3
    Vector4T<T> TransformVector(const Vector3T<T>& v) const;


    /*-----------------------------------------------------------------------------------------
        Simple transformations
    -----------------------------------------------------------------------------------------*/

	// Move position of matrix by given amount in world X direction
	void MoveX(T x)  { e30+= x; }

	// Move position of matrix by given amount in world YX direction
	void MoveY(T y)  { e31+= y; }

	// Move position of matrix by given amount in world Z direction
	void MoveZ(T z)  { e32+= z; }


	// Move position of matrix by given amount in local X direction
	void MoveLocalX(T x)  { Position() += Normalise(XAxis()) * x; }

	// Move position of matrix by given amount in local YX direction
	void MoveLocalY(T y)  { Position() += Normalise(YAxis()) * y; }

	// Move position of matrix by given amount in local Z direction
	void MoveLocalZ(T z)  { Position() += Normalise(ZAxis()) * z; }


	// Rotate matrix by given amount around the world X axis
	void RotateX(T x);

	// Rotate matrix by given amount around the world Y axis
	void RotateY(T y);

	// Rotate matrix by given amount around the world Z axis
	void RotateZ(T z);


	// Rotate matrix by given amount around its local X axis
	void RotateLocalX(T x);

	// Rotate matrix by given amount around its local Y axis
	void RotateLocalY(T y);

	// Rotate matrix by given amount around its local Z axis
	void RotateLocalZ(T z);


	/*-----------------------------------------------------------------------------------------
	    Other functions
	-----------------------------------------------------------------------------------------*/

	// Rotate this matrix to face the given target point keeping it's position/scale the same (the Z axis will turn to point at the target).
    // Assumes that the matrix is an affine matrix that already holds a position. Will retain the matrix's current scaling
    void FaceTarget(const Vector3T<T>& target);

    // Rotate this matrix to face in the given direction keeping position/scale the same (the Z axis will turn to match the given direction)
    // Assumes that the matrix is an affine matrix that already holds a position. Will retain the matrix's current scaling
    void FaceDirection(const Vector3T<T>& direction);


    // Transpose the matrix (rows become columns), returns matrix so result can be used in further operations, e.g. Matrix4x4 m3 = Transpose(m1) * m2
    // Different APIs use different conventions for rows and columns so this operation is sometimes needed
    // It is also used in some mathematical operations
    Matrix4x4T<T>& Transpose();


    /*-----------------------------------------------------------------------------------------
        Static members
    -----------------------------------------------------------------------------------------*/
    
    // Identity matrix, use like this:  Matrix4x4 newMatrix = Matrix4x4::Identity;
    static const Matrix4x4T<T> Identity;
};

// Initialise static identity matrix
template <typename T> const Matrix4x4T<T> Matrix4x4T<T>::Identity = { 1, 0, 0, 0,
                                                                      0, 1, 0, 0,
                                                                      0, 0, 1, 0,
                                                                      0, 0, 0, 1 };


/*-----------------------------------------------------------------------------------------
    Non-member Operators
-----------------------------------------------------------------------------------------*/

// Matrix-matrix multiplication
template<typename T> Matrix4x4T<T> operator*(const Matrix4x4T<T>& m1, const Matrix4x4T<T>& m2);

// Vector-matrix multiplication, transforms the vector by the matrix
// Use the member functions TransformPoint or TransformVector if you have Vector3 values to be transformed
template<typename T> Vector4T<T> operator*(const Vector4T<T>& v, const Matrix4x4T<T>& m);


/*-----------------------------------------------------------------------------------------
  Non-member functions
-----------------------------------------------------------------------------------------*/

// The following functions create a new matrix holding a particular transformation
// They can be used as temporaries in calculations, e.g.
//     Matrix4x4 m = MatrixScaling(3) * MatrixTranslation({ 10, -10, 20 });

// Return a translation matrix of the given vector
template<typename T> Matrix4x4T<T> MatrixTranslation(const Vector3T<T>& t);


// Return an X-axis rotation matrix of the given angle (in radians)
template<typename T> Matrix4x4T<T> MatrixRotationX(T x);

// Return a Y-axis rotation matrix of the given angle (in radians)
template<typename T> Matrix4x4T<T> MatrixRotationY(T y);

// Return a Z-axis rotation matrix of the given angle (in radians)
template<typename T> Matrix4x4T<T> MatrixRotationZ(T z);

// Return a rotation matrix that rotates around the given X,Y and Z angles (in radians). Order of rotations: ZXY
template<typename T> Matrix4x4T<T> MatrixRotation(Vector3T<T> rotations);


// Return a matrix that is a scaling in X,Y and Z of the values in the given vector
template<typename T> Matrix4x4T<T> MatrixScaling(const Vector3T<T>& s);

// Return a matrix that is a uniform scaling of the given amount
template<typename T> Matrix4x4T<T> MatrixScaling(const T s);



// Return the inverse of given matrix assuming that it is an affine matrix
// Most commonly used to get the view matrix from the camera's positioning matrix
template<typename T> Matrix4x4T<T> InverseAffine(const Matrix4x4T<T>& m);


#endif // _MATRIX4X4_H_DEFINED_
