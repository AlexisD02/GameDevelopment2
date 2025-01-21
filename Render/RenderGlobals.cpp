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

#include "RenderGlobals.h"
#include "CBuffer.h"


//--------------------------------------------------------------------------------------
// DirectX Device
//--------------------------------------------------------------------------------------
// DX encapsulates the important D3DDevice and D3DContext objects used for almost all DirectX calls
// as well as a few other useful global DirectX data such as backbuffer size

std::unique_ptr<DXDevice> DX;


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Global buffer for each of the constant structures defined in "CBufferTypes.h"

PerFrameConstants  gPerFrameConstants;      // The constants (settings) that are sent to the GPU each frame (see above for structure contents)
ID3D11Buffer*      gPerFrameConstantBuffer; // The GPU buffer that will recieve per-frame constants

PerCameraConstants gPerCameraConstants;     // As above, but constants (settings) that change for each camera viewpoint
ID3D11Buffer*      gPerCameraConstantBuffer;

PerMeshConstants   gPerMeshConstants;       // As above, but constants (settings) that change per-mesh (e.g. world matrix)
ID3D11Buffer*      gPerMeshConstantBuffer;

// There are also per-material constants - settings that change to suit the material a submesh uses
// However those constants are held in the MeshRenderState objects held in the submeshes, so no global here
ID3D11Buffer* gPerMaterialConstantBuffer; // The GPU buffer that will receive per-material constants


//--------------------------------------------------------------------------------------
// Global Constant Buffer Creation
//--------------------------------------------------------------------------------------

// Create all the global constant buffers. Returns true on success
bool CreateCBuffers()
{
	// Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
	// These allow us to pass data from CPU to shaders such as lighting information or matrices
	// See the comments above where these variable are declared and also the UpdateScene function
	gPerFrameConstantBuffer    = DX->CBuffers()->CreateCBuffer(sizeof(PerFrameConstants));
	gPerCameraConstantBuffer   = DX->CBuffers()->CreateCBuffer(sizeof(PerCameraConstants));
	gPerMaterialConstantBuffer = DX->CBuffers()->CreateCBuffer(sizeof(PerMaterialConstants));
	gPerMeshConstantBuffer     = DX->CBuffers()->CreateCBuffer(sizeof(PerMeshConstants));

	if (gPerFrameConstantBuffer == nullptr || gPerCameraConstantBuffer   == nullptr ||
		gPerMeshConstantBuffer  == nullptr || gPerMaterialConstantBuffer == nullptr)
	{
		return false;
	}

	// Enable these constant buffers on all shaders. Each one is given a slot number that should match the number in the constant
	// buffer declaration in the shaders (see Common.hlsli)   e.g. cbuffer bufferName : register(b?)   where ? is the slot number
	DX->CBuffers()->EnableCBuffer(gPerFrameConstantBuffer,    0);
	DX->CBuffers()->EnableCBuffer(gPerCameraConstantBuffer,   1);
	DX->CBuffers()->EnableCBuffer(gPerMeshConstantBuffer,     2);
	DX->CBuffers()->EnableCBuffer(gPerMaterialConstantBuffer, 3);

	return true;
}
