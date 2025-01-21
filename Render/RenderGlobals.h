//--------------------------------------------------------------------------------------
// DirectX related global variables
//--------------------------------------------------------------------------------------
// This app is mostly well-encapsulated, but there are a few globals in use to simplify the code architecture
// 
// The single DirectX object (DX) is used for all DirectX calls and is used in many classes. So instead of passing
// a pointer to all objects that deal with DirectX we just make a single global.
//
// The constant buffers (per-frame, per camera, etc.) are used in various places in both rendering code and scene code.
// Again to avoid having to pass pointers around to all parts of the code that need access, they are made global.
//
// It would be possible to deal with these as "singleton" classes. These are classes that only ever have a single object.
// It might make the code a tiny bit cleaner the benefit is marginal - singletons have the same downsides as globals

#ifndef _RENDER_GLOBALS_H_INCLUDED_
#define _RENDER_GLOBALS_H_INCLUDED_

#include "DXDevice.h"
#include "CBufferTypes.h"

#include <memory>

//--------------------------------------------------------------------------------------
// DirectX Device
//--------------------------------------------------------------------------------------
// DX encapsulates the important D3DDevice and D3DContext objects used for almost all DirectX calls
// as well as a few other useful global DirectX data such as backbuffer size

extern std::unique_ptr<DXDevice> DX;


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Global buffer for each of the constant structures defined in CBufferTypes.h

extern PerFrameConstants  gPerFrameConstants;       // The constants (settings) that are sent to the GPU each frame (see above for structure contents)
extern ID3D11Buffer*      gPerFrameConstantBuffer;  // The GPU buffer that will recieve per-frame constants

extern PerCameraConstants gPerCameraConstants;      // As above, but constants (settings) that change for each camera viewpoint
extern ID3D11Buffer*      gPerCameraConstantBuffer;

extern PerMeshConstants   gPerMeshConstants;        // As above, but constants (settings) that change per-mesh (e.g. world matrix)
extern ID3D11Buffer*      gPerMeshConstantBuffer;

// There are also per-material constants - settings that change to suit the material a submesh uses
// However those constants are held in the MeshRenderState objects held in the submeshes, so no global here
extern ID3D11Buffer*      gPerMaterialConstantBuffer; // The GPU buffer that will receive per-material constants


// Create all the global constant buffers. Returns true on success
bool CreateCBuffers();



#endif //_RENDER_GLOBALS_H_INCLUDED_
