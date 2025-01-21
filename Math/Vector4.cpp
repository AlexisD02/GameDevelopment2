//--------------------------------------------------------------------------------------
// Vector4 class, encapsulating (x,y,z,w) coordinates and supporting functions.
// Used for points and vectors when they are being multiplied by 4x4 matrices
// Supports float (Vector4) and double (Vector4d) 
//--------------------------------------------------------------------------------------

#include "Vector4.h"


/*-----------------------------------------------------------------------------------------
    Template Instatiation
-----------------------------------------------------------------------------------------*/
// Instantiate this template class for specific numeric types

template class Vector4T<float>;  // Vector4 or Vector4f
template class Vector4T<double>; // Vector4d
// Not supporting int Vector4
