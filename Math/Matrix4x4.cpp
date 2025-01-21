//--------------------------------------------------------------------------------------
// Matrix4x4 class specialised for affine matrices for 3D graphics
// Supports float (Matrix4x4) and double (Matrix4x4d)
//--------------------------------------------------------------------------------------

#include "Matrix4x4.h"
#include "MathHelpers.h"
#include <algorithm>
#include <cmath>

/*-----------------------------------------------------------------------------------------
    Getters / Setters
-----------------------------------------------------------------------------------------*/
// Converts rotational part of the matrix into Euler angles (X,Y,Z floats like TL-Engine). Getter only.
// Assumes rotation order will be Z first, then X, then Y
template<typename T> Vector3T<T> Matrix4x4T<T>::GetRotation() const
{
    // Calculate matrix scaling
    T scaleX = std::sqrt(e00 * e00 + e01 * e01 + e02 * e02);
    T scaleY = std::sqrt(e10 * e10 + e11 * e11 + e12 * e12);
    T scaleZ = std::sqrt(e20 * e20 + e21 * e21 + e22 * e22);

    // Calculate inverse scaling to extract rotational values only
    T invScaleX = 1 / scaleX;
    T invScaleY = 1 / scaleY;
    T invScaleZ = 1 / scaleZ;

    T sX, cX, sY, cY, sZ, cZ;

    sX = -e21 * invScaleZ;
    cX = std::sqrt(1 - sX * sX);

    // If no gimbal lock...
    if (!IsZero(std::abs(cX)))
    {
        T invCX = 1 / cX;
        sZ = e01 * invCX * invScaleX;
        cZ = e11 * invCX * invScaleY;
        sY = e20 * invCX * invScaleZ;
        cY = e22 * invCX * invScaleZ;
    }
    else
    {
        // Gimbal lock - force Z angle to 0
        sZ = 0;
        cZ = 1;
        sY = -e02 * invScaleX;
        cY =  e00 * invScaleX;
    }

    return { std::atan2(sX, cX), std::atan2(sY, cY), std::atan2(sZ, cZ) };
}


// Sets rotation of the matrix from given Euler angles (X,Y,Z floats like TL-Engine). Does not affect position or scale.
// Uses rotation order of Z first, then X, then Y
template<typename T>void Matrix4x4T<T>::SetRotation(Vector3T<T> rotation)
{
    // To put rotation angles into a matrix we build a rotation matrix from scratch then apply existing scaling and position
    Matrix4x4T<T> rotationMatrix = MatrixRotationZ(rotation.z) * MatrixRotationX(rotation.x) * MatrixRotationY(rotation.y);
    rotationMatrix.SetScale(GetScale());
    rotationMatrix.Position() = Position();
    *this = rotationMatrix;
}


// Sets scaling of the matrix seperately in X,Y and Z. Does not affect position or rotation.
template<typename T>void Matrix4x4T<T>::SetScale(Vector3T<T> scale)
{
    // Remove current scaling from each row with normalise then multiply by new scaling
    Row(0) = Normalise(Row(0)) * scale.x;
    Row(1) = Normalise(Row(1)) * scale.y;
    Row(2) = Normalise(Row(2)) * scale.z;
}


/*-----------------------------------------------------------------------------------------
    Operators
-----------------------------------------------------------------------------------------*/

// Post-multiply this matrix by the given one
template<typename T> Matrix4x4T<T>& Matrix4x4T<T>::operator*=(const Matrix4x4T<T>& m)
{
    if (this == &m)
    {
        // Special case of multiplying by self - no copy optimisations possible so use non-member version of multiply operator
        *this = m * m;
    }
    else
    {
        T t0, t1, t2;

        t0 = e00 * m.e00 + e01 * m.e10 + e02 * m.e20 + e03 * m.e30;
        t1 = e00 * m.e01 + e01 * m.e11 + e02 * m.e21 + e03 * m.e31;
        t2 = e00 * m.e02 + e01 * m.e12 + e02 * m.e22 + e03 * m.e32;
        e03 = e00 * m.e03 + e01 * m.e13 + e02 * m.e23 + e03 * m.e33;
        e00 = t0;
        e01 = t1;
        e02 = t2;

        t0 = e10 * m.e00 + e11 * m.e10 + e12 * m.e20 + e13 * m.e30;
        t1 = e10 * m.e01 + e11 * m.e11 + e12 * m.e21 + e13 * m.e31;
        t2 = e10 * m.e02 + e11 * m.e12 + e12 * m.e22 + e13 * m.e32;
        e13 = e10 * m.e03 + e11 * m.e13 + e12 * m.e23 + e13 * m.e33;
        e10 = t0;
        e11 = t1;
        e12 = t2;

        t0 = e20 * m.e00 + e21 * m.e10 + e22 * m.e20 + e23 * m.e30;
        t1 = e20 * m.e01 + e21 * m.e11 + e22 * m.e21 + e23 * m.e31;
        t2 = e20 * m.e02 + e21 * m.e12 + e22 * m.e22 + e23 * m.e32;
        e23 = e20 * m.e03 + e21 * m.e13 + e22 * m.e23 + e23 * m.e33;
        e20 = t0;
        e21 = t1;
        e22 = t2;

        t0 = e30 * m.e00 + e31 * m.e10 + e32 * m.e20 + e33 * m.e30;
        t1 = e30 * m.e01 + e31 * m.e11 + e32 * m.e21 + e33 * m.e31;
        t2 = e30 * m.e02 + e31 * m.e12 + e32 * m.e22 + e33 * m.e32;
        e33 = e30 * m.e03 + e31 * m.e13 + e32 * m.e23 + e33 * m.e33;
        e30 = t0;
        e31 = t1;
        e32 = t2;
    }
    return *this;
}


// Assume given Vector3 is a point and transform by the given matrix
// Returns a Vector4 (w=1 for point), but result can also be put into a Vector3 since Vector4 can cast to Vector3
template<typename T> Vector4T<T> Matrix4x4T<T>::TransformPoint(const Vector3T<T>& v) const
{
    Vector4T<T> vOut;

    vOut.x = v.x * e00 + v.y * e10 + v.z * e20 + e30;
    vOut.y = v.x * e01 + v.y * e11 + v.z * e21 + e31;
    vOut.z = v.x * e02 + v.y * e12 + v.z * e22 + e32;
    vOut.w = 1;

    return vOut;
}

// Assume given Vector3 is a vector and transform by the given matrix
// Returns a Vector4 (w=0 for vector), but result can also be put into a Vector3 since Vector4 can cast to Vector3
template<typename T> Vector4T<T> Matrix4x4T<T>::TransformVector(const Vector3T<T>& v) const
{
    Vector4T<T> vOut;

    vOut.x = v.x * e00 + v.y * e10 + v.z * e20;
    vOut.y = v.x * e01 + v.y * e11 + v.z * e21;
    vOut.z = v.x * e02 + v.y * e12 + v.z * e22;
    vOut.w = 0;

    return vOut;
}



/*-----------------------------------------------------------------------------------------
    Non-member Operators
-----------------------------------------------------------------------------------------*/

// Matrix-matrix multiplication
template<typename T> Matrix4x4T<T> operator*(const Matrix4x4T<T>& m1, const Matrix4x4T<T>& m2)
{
    Matrix4x4T<T> mOut;

    mOut.e00 = m1.e00*m2.e00 + m1.e01*m2.e10 + m1.e02*m2.e20 + m1.e03*m2.e30;
    mOut.e01 = m1.e00*m2.e01 + m1.e01*m2.e11 + m1.e02*m2.e21 + m1.e03*m2.e31;
    mOut.e02 = m1.e00*m2.e02 + m1.e01*m2.e12 + m1.e02*m2.e22 + m1.e03*m2.e32;
    mOut.e03 = m1.e00*m2.e03 + m1.e01*m2.e13 + m1.e02*m2.e23 + m1.e03*m2.e33;

    mOut.e10 = m1.e10*m2.e00 + m1.e11*m2.e10 + m1.e12*m2.e20 + m1.e13*m2.e30;
    mOut.e11 = m1.e10*m2.e01 + m1.e11*m2.e11 + m1.e12*m2.e21 + m1.e13*m2.e31;
    mOut.e12 = m1.e10*m2.e02 + m1.e11*m2.e12 + m1.e12*m2.e22 + m1.e13*m2.e32;
    mOut.e13 = m1.e10*m2.e03 + m1.e11*m2.e13 + m1.e12*m2.e23 + m1.e13*m2.e33;

    mOut.e20 = m1.e20*m2.e00 + m1.e21*m2.e10 + m1.e22*m2.e20 + m1.e23*m2.e30;
    mOut.e21 = m1.e20*m2.e01 + m1.e21*m2.e11 + m1.e22*m2.e21 + m1.e23*m2.e31;
    mOut.e22 = m1.e20*m2.e02 + m1.e21*m2.e12 + m1.e22*m2.e22 + m1.e23*m2.e32;
    mOut.e23 = m1.e20*m2.e03 + m1.e21*m2.e13 + m1.e22*m2.e23 + m1.e23*m2.e33;

    mOut.e30 = m1.e30*m2.e00 + m1.e31*m2.e10 + m1.e32*m2.e20 + m1.e33*m2.e30;
    mOut.e31 = m1.e30*m2.e01 + m1.e31*m2.e11 + m1.e32*m2.e21 + m1.e33*m2.e31;
    mOut.e32 = m1.e30*m2.e02 + m1.e31*m2.e12 + m1.e32*m2.e22 + m1.e33*m2.e32;
    mOut.e33 = m1.e30*m2.e03 + m1.e31*m2.e13 + m1.e32*m2.e23 + m1.e33*m2.e33;

    return mOut;
}


// Vector-matrix multiplication, transforms the vector by the matrix
// Use the member functions TransformPoint or TransformVector if you have Vector3 values to be transformed
template<typename T> Vector4T<T> operator*(const Vector4T<T>& v, const Matrix4x4T<T>& m)
{
    Vector4T<T> vOut;

    vOut.x = v.x * m.e00 + v.y * m.e10 + v.z * m.e20 + v.w * m.e30;
    vOut.y = v.x * m.e01 + v.y * m.e11 + v.z * m.e21 + v.w * m.e31;
    vOut.z = v.x * m.e02 + v.y * m.e12 + v.z * m.e22 + v.w * m.e32;
    vOut.w = v.x * m.e03 + v.y * m.e13 + v.z * m.e23 + v.w * m.e33;

    return vOut;
}



/*-----------------------------------------------------------------------------------------
    Simple transformations
-----------------------------------------------------------------------------------------*/

// Rotate matrix by given amount around the world X axis
template<typename T> void Matrix4x4T<T>::RotateX(T x)  { *this *= MatrixRotationX<T>(x); }

// Rotate matrix by given amount around the world Y axis
template<typename T> void Matrix4x4T<T>::RotateY(T y)  { *this *= MatrixRotationY<T>(y); }

// Rotate matrix by given amount around the world Z axis
template<typename T> void Matrix4x4T<T>::RotateZ(T z)  { *this *= MatrixRotationZ<T>(z); }


// Rotate matrix by given Matrix4x4T<T>::amount around its local X axis
template<typename T> void Matrix4x4T<T>::RotateLocalX(T x)  { *this = MatrixRotationX<T>(x) * *this; }

// Rotate matrix by given amount around its local Y axis
template<typename T> void Matrix4x4T<T>::RotateLocalY(T y)  { *this = MatrixRotationY<T>(y) * *this; }

// Rotate matrix by given amount around its local Z axis
template<typename T> void Matrix4x4T<T>::RotateLocalZ(T z)  { *this = MatrixRotationZ<T>(z) * *this; }



/*-----------------------------------------------------------------------------------------
    Other member functions
-----------------------------------------------------------------------------------------*/

// Rotate this matrix to face the given target point keeping it's position/scale the same (the Z axis will turn to point at the target).
// Assumes that the matrix is an affine matrix that already holds a position. Will retain the matrix's current scaling
template<typename T> void Matrix4x4T<T>::FaceTarget(const Vector3T<T>& target)
{
    FaceDirection(target - Position());
}


// Rotate this matrix to face in the given direction keeping position/scale the same (the Z axis will turn to match the given direction)
// Assumes that the matrix is an affine matrix that already holds a position. Will retain the matrix's current scaling
template<typename T> void Matrix4x4T<T>::FaceDirection(const Vector3T<T>& direction)
{
    Vector3T<T> newXAxis, newYAxis, newZAxis;

    // New matrix Z axis is in the given direction
    newZAxis = Normalise(direction);
    if (IsZero(newZAxis.Length()))  return; // Does nothing if target is in same place as this matrix

    // Try to cross product world Y axis ("up") with new facing direction (Z) to get new X axis. However, this
    // will not work if the facing direction required is also in the world Y direction (cross product of identical
    // vectors is zero), so in that circumstance use world -Z axis
    Vector3T<T> up = { 0, 1, 0 };
    if (IsZero(Dot(newZAxis, up) - 1))  up = { 0, 0, -1 };
    newXAxis = Normalise(Cross(up, newZAxis));

    // Cross product new X and Z axes to get new Y axis
    newYAxis = Cross(newZAxis, newXAxis); // Will be normalised since the new axes are

    // Set rows of matrix, using original matrix scales for each. Position will be unchanged
    Vector3T<T> scale = GetScale();
    Row(0) = newXAxis * scale.x;
    Row(1) = newYAxis * scale.y;
    Row(2) = newZAxis * scale.z;
}


// Transpose the matrix (rows become columns)
// Different APIs use different conventions for rows and columns so this operation is sometimes needed
// It is also used in some mathematical operations
template<typename T> Matrix4x4T<T>& Matrix4x4T<T>::Transpose()
{
    std::swap(e01, e10);
    std::swap(e02, e20);
    std::swap(e03, e30);
    std::swap(e12, e21);
    std::swap(e13, e31);
    std::swap(e23, e32);

    return *this;
}


/*-----------------------------------------------------------------------------------------
    Other non-member functions
-----------------------------------------------------------------------------------------*/

// The following functions create a new matrix holding a particular transformation
// They can be used as temporaries in calculations, e.g.
//     Matrix4x4 m = MatrixScaling(3.0f) * MatrixTranslation({ 10.0f, -10.0f, 20.0f });

// Return a translation matrix of the given vector
template<typename T> Matrix4x4T<T> MatrixTranslation(const Vector3T<T>& t)
{
    return Matrix4x4T<T>{ 1,   0,   0,    0,
                          0,   1,   0,    0,
                          0,   0,   1,    0,
                          t.x, t.y, t.z,  1 };
}


// Return an X-axis rotation matrix of the given angle (in radians)
template<typename T> Matrix4x4T<T> MatrixRotationX(T x)
{
    T sX = std::sin(x);
    T cX = std::cos(x);

    return Matrix4x4T<T>{ 1,   0,   0,  0,
                          0,  cX,  sX,  0,
                          0, -sX,  cX,  0,
                          0,   0,   0,  1 };
}

// Return a Y-axis rotation matrix of the given angle (in radians)
template<typename T> Matrix4x4T<T> MatrixRotationY(T y)
{
    T sY = std::sin(y);
    T cY = std::cos(y);

    return Matrix4x4T<T>{ cY,   0, -sY,  0,
                           0,   1,   0,  0,
                          sY,   0,  cY,  0,
                           0,   0,   0,  1 };
}

// Return a Z-axis rotation matrix of the given angle (in radians)
template<typename T> Matrix4x4T<T> MatrixRotationZ(T z)
{
    T sZ = std::sin(z);
    T cZ = std::cos(z);

    return Matrix4x4T<T>{ cZ,  sZ,  0,  0,
                         -sZ,  cZ,  0,  0,
                           0,   0,  1,  0,
                           0,   0,  0,  1 };
}

// Return a rotation matrix that rotates around the given X,Y and Z angles (in radians). Order of rotations: ZXY
template<typename T> Matrix4x4T<T> MatrixRotation(Vector3T<T> rotations)
{
    return MatrixRotationZ(rotations.z) * MatrixRotationX(rotations.x) * MatrixRotationY(rotations.y);
}



// Return a matrix that is a scaling in X,Y and Z of the values in the given vector
template<typename T> Matrix4x4T<T> MatrixScaling(const Vector3T<T>& s)
{
    return Matrix4x4T<T>{ s.x,   0,   0,  0,
                            0, s.y,   0,  0,
                            0,   0, s.z,  0,
                            0,   0,   0,  1 };
}

// Return a matrix that is a uniform scaling of the given amount
template<typename T> Matrix4x4T<T> MatrixScaling(const T s)
{
    return Matrix4x4T<T>{ s, 0, 0, 0,
                          0, s, 0, 0,
                          0, 0, s, 0,
                          0, 0, 0, 1 };
}


// Return the inverse of given matrix assuming that it is an affine matrix
// Most commonly used to get the view matrix from the camera's positioning matrix
template<typename T> Matrix4x4T<T> InverseAffine(const Matrix4x4T<T>& m)
{
    Matrix4x4T<T> mOut;

    // Calculate determinant of upper left 3x3
    T det0 = m.e11*m.e22 - m.e12*m.e21;
    T det1 = m.e12*m.e20 - m.e10*m.e22;
    T det2 = m.e10*m.e21 - m.e11*m.e20;
    T det  = m.e00*det0 + m.e01*det1 + m.e02*det2;

    // Calculate inverse of upper left 3x3
    T invDet = 1 / det;
    mOut.e00 = invDet * det0;
    mOut.e10 = invDet * det1;
    mOut.e20 = invDet * det2;

    mOut.e01 = invDet * (m.e21*m.e02 - m.e22*m.e01);
    mOut.e11 = invDet * (m.e22*m.e00 - m.e20*m.e02);
    mOut.e21 = invDet * (m.e20*m.e01 - m.e21*m.e00);

    mOut.e02 = invDet * (m.e01*m.e12 - m.e02*m.e11);
    mOut.e12 = invDet * (m.e02*m.e10 - m.e00*m.e12);
    mOut.e22 = invDet * (m.e00*m.e11 - m.e01*m.e10);

    // Transform negative translation by inverted 3x3 to get inverse
    mOut.e30 = -m.e30*mOut.e00 - m.e31*mOut.e10 - m.e32*mOut.e20;
    mOut.e31 = -m.e30*mOut.e01 - m.e31*mOut.e11 - m.e32*mOut.e21;
    mOut.e32 = -m.e30*mOut.e02 - m.e31*mOut.e12 - m.e32*mOut.e22;

    // Fill in right column for affine matrix
    mOut.e03 = 0;
    mOut.e13 = 0;
    mOut.e23 = 0;
    mOut.e33 = 1;

    return mOut;
}


/*-----------------------------------------------------------------------------------------
    Template Instatiation
-----------------------------------------------------------------------------------------*/
// Instantiate this template class for specific numeric types. Prevents use of other types and
// allows us to put code in this cpp file (normally template code must be all in the header file)

template class Matrix4x4T<float>;  // Matrix4x4 or Matrix4x4f
template class Matrix4x4T<double>; // Matrix4x4d

// Also need to instantiate all the non-member template functions
template Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);
template Vector4   operator*(const Vector4& v, const Matrix4x4& m);
template Matrix4x4 MatrixTranslation(const Vector3& t);
template Matrix4x4 MatrixRotationX(float x);
template Matrix4x4 MatrixRotationY(float y);
template Matrix4x4 MatrixRotationZ(float z);
template Matrix4x4 MatrixScaling(const Vector3& s);
template Matrix4x4 MatrixScaling(const float s);
template Matrix4x4 InverseAffine(const Matrix4x4& m);

template Matrix4x4d operator*(const Matrix4x4d& m1, const Matrix4x4d& m2);
template Vector4d   operator*(const Vector4d& v, const Matrix4x4d& m);
template Matrix4x4d MatrixTranslation(const Vector3d& t);
template Matrix4x4d MatrixRotationX(double x);
template Matrix4x4d MatrixRotationY(double y);
template Matrix4x4d MatrixRotationZ(double z);
template Matrix4x4d MatrixScaling(const Vector3d& s);
template Matrix4x4d MatrixScaling(const double s);
template Matrix4x4d InverseAffine(const Matrix4x4d& m);
