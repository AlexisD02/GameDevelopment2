//--------------------------------------------------------------------------------------
// Common include file for all shaders
//--------------------------------------------------------------------------------------
// Using include files to define constant buffers available to shaders

// Prevent this include file being included multiple times (just as we do with C++ header files)
#ifndef _COMMON_HLSLI_DEFINED_
#define _COMMON_HLSLI_DEFINED_


//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------

static const float PI = 3.14159265f;


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// These structures are "constant buffers" - a way of passing variables over from C++ to the GPU
// They are called constants but that only means constant for the duration of a single GPU draw call.
// These "constants" correspond to variables in C++ that we will change per-mesh, or per-frame etc.
//
// Note constant buffers are not structs: we don't use the name of the constant buffer in the shaders, these
// are really just a collection of global variables (hence the 'g')


// The constants (settings) that need to be sent to the GPU each frame (see C++ PerFrameConstants declaration for contents)
// These variables must match exactly the PerFrameConstants structure in the C++ code
cbuffer PerFrameConstants : register(b0) // The b0 gives this constant buffer the slot number 0 - used in the C++ code
{
    float3   gLight1Position;
    float    gViewportWidth;  // Using viewport width and height as padding - data needs to fit into blocks of 4 floats/ints (16 byte blocks)
    float3   gLight1Colour;
    float    gViewportHeight;

    float3   gLight2Position;
    float    padding1;
    float3   gLight2Colour;
    float    padding2;

    float3   gAmbientColour;
    float    padding3;
}


// The constants (settings) that need to be sent to the GPU each frame (see C++ PerFrameConstants declaration for contents)
// These variables must match exactly the PerFrameConstants structure in the C++ code
cbuffer PerCameraConstants : register(b1) // The b1 gives this constant buffer the slot number 1 - used in the C++ code
{
    float4x4 gCameraMatrix;         // World matrix for the camera (inverse of the view matrix below)
    float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;
    float4x4 gViewProjectionMatrix; // The above two matrices multiplied together to combine their effects

    float3   gCameraPosition;
    float    padding4;
}

static const int MAX_BONES = 64;


// The constants (settings) that need to change for each mesh rendered, e.g. transformation matrices (see C++ PerMeshConstants declaration for contents)
// These variables must match exactly the PerMeshConstants structure in the C++ code
cbuffer PerMeshConstants : register(b2) // The b1 gives this constant buffer the number 1 - used in the C++ code
{
    float4x4 gWorldMatrix;
    float4   gMeshColour;  // Per-mesh colour, typically used to tint it (e.g. tinting light meshes to the colour of the light they emit)
	float4x4 gBoneMatrices[MAX_BONES];
}


// The constants (settings) that need to change for each submesh to suit the material it uses (see C++ PerMaterialConstants declaration for contents)
// These variables must match exactly the PerMaterialConstants structure in the C++ code
cbuffer PerMaterialConstants : register(b3)
{
    float4  gMaterialDiffuseColour;
    float3  gMaterialSpecularColour;
    float   gMaterialSpecularPower;
    float   gParallaxDepth;
}

#endif // _COMMON_HLSLI_DEFINED_
