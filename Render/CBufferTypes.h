//--------------------------------------------------------------------------------------
// Constant Buffer related types
//--------------------------------------------------------------------------------------
// Constant buffers allow us to pass data from C++ to shaders
//
// Different constant buffers are used for different frequency of update. E.g. the per-frame constant buffer holds data that
// only changes each frame such as light positions, whereas the per-material constant buffer changes for each new material
// rendered within a single frame - it might hold things such as material diffuse colour.

#ifndef _C_BUFFER_TYPES_H_INCLUDED_
#define _C_BUFFER_TYPES_H_INCLUDED_

#include "MeshTypes.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "ColourTypes.h"

#define NOMINMAX // Use NOMINMAX to stop certain Microsoft headers defining "min" and "max", which breaks some libraries (e.g. assimp)
#include <d3d11.h>


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// Per-frame buffer holds data that is constant for an entire frame, updated from C++ to the GPU shaders once per frame
// We hold the data together in a structure and send the whole thing to a "constant buffer" on the GPU each frame when
// we have finished updating the scene. There is a structure in the shader code that exactly matches this one
struct PerFrameConstants
{
	Vector3   light1Position = { 0, 100, 0 };
	float     viewportWidth  = 1280;          // Using viewport width and height as padding - data needs to fit into blocks of 4 floats/ints (16 byte blocks)
	ColourRGB light1Colour   = { 1, 1, 1 };
	float     viewportHeight = 720;

	Vector3   light2Position = { 100, 100, 100 };
	float     padding1 = {};
	ColourRGB light2Colour   = { 1, 1, 1 };
	float     padding2 = {};

	ColourRGB ambientColour  = { 0, 0, 0 };
	float     padding3 = {};

	// Miscellaneous water variables
	//float     waterPlaneY   = 0;        // Y coordinate of the water plane (before adding the height map)
	//float     waveScale     = 1;        // How tall the waves are (rescales weight heights and normals)
	//Vector2f  waterMovement = { 0, 0 }; // An offset added to the water height map UVs to make the water surface move
};


// Per-camera buffer holds data that is constant whilst rendering the scene from a single camera. We might render the
// scene more than once in a frame, e.g. for portals or mirrors. Other than that it works in the same way as the per-frame buffer above
struct PerCameraConstants
{
	// These are the matrices used to position the camera
	Matrix4x4 cameraMatrix         = Matrix4x4::Identity; // World matrix for the camera (inverse of the view matrix below)
	Matrix4x4 viewMatrix           = Matrix4x4::Identity;
	Matrix4x4 projectionMatrix     = Matrix4x4::Identity;
	Matrix4x4 viewProjectionMatrix = Matrix4x4::Identity; // The above two matrices multiplied together to combine their effects

	Vector3   cameraPosition = { 0, 0, 0 };
	float     padding4 = {};
};


// Data required to render the next mesh in the scene, most importantly its transformation matrices. This data is updated and sent to
// the GPU several times every frame, once per mesh (or mesh type). Other than that it works in the same way as the per-frame buffer
struct PerMeshConstants
{
	Matrix4x4  worldMatrix = Matrix4x4::Identity;
	ColourRGBA meshColour = { 1, 1, 1, 1 };  // Per-mesh colour, typically used to tint it (e.g. tinting light meshes to the colour of the light they emit)
	Matrix4x4  boneMatrices[MAX_BONES] = {};
};


// Data required to render the material used by the next submesh in the scene. This data is updated and sent to the GPU several times
// every frame, once per submesh. Other than that it works in the same way as the above buffers
struct PerMaterialConstants
{
	ColourRGBA diffuseColour  = { 1, 1, 1, 1 }; // Alpha will be imported from mesh file
	ColourRGB  specularColour = { 0, 0, 0 };
	float      specularPower  = 0;
	float      parallaxDepth  = 0;
};



#endif //_C_BUFFER_TYPES_H_INCLUDED_
