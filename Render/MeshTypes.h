//--------------------------------------------------------------------------------------
// Mesh related types
//--------------------------------------------------------------------------------------

#ifndef _MESH_TYPES_H_INCLUDED_
#define _MESH_TYPES_H_INCLUDED_

#include "Utility.h" // For enum flags support (ENUM_FLAG_OPERATORS)

#include <vector>
#include <stdint.h>


//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------

// Maximum bones allowed in a single mesh (across all its submeshes)
static const int MAX_BONES = 64;


//--------------------------------------------------------------------------------------
// Geometry
//--------------------------------------------------------------------------------------

// Types of data that a single vertex may have in a particular submesh
enum class GeometryTypes : uint32_t
{
	Position  = 0x1,
	Normal    = 0x2,
	Tangent   = 0x4,
	Bitangent = 0x8,
	UV        = 0x10,
	Colour    = 0x20,
	BoneData  = 0x40,

	PosUV         = Position + UV,
	PosNorm       = Position + Normal,
	PosNormUV     = Position + Normal + UV,
	PosNormTangUV = Position + Normal + Tangent + UV,
};
// Use above enum as flags (adds bitwise operators)
ENUM_FLAG_OPERATORS(GeometryTypes)


#endif // _MESH_TYPES_H_INCLUDED_
