//--------------------------------------------------------------------------------------
// StateManager class encapsulates the handling of GPU pipeline states such as blending or culling
//--------------------------------------------------------------------------------------
// GPU states handle various global settings that adjust how the GPU renders:
// - Rasterizer state (Wireframe mode, don't cull back faces etc.)
// - Depth stencil state (How to use the depth and stencil buffer)
// - Blender state (Additive blending, alpha blending etc.)
// Each state requires a DirectX object to be created in advance and set when required.
// The StateManager class entirely manages these DirectX objects (creation and cleanup)
// States can be set at render time with a simple call specifying an enum value RenderState.

#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#define NOMINMAX // Use NOMINMAX to stop certain Microsoft headers defining "min" and "max", which breaks some libraries (e.g. assimp)
#include <d3d11.h>
#include <atlbase.h> // For CComPtr (see member variables)

#include <map>
#include <string>


//--------------------------------------------------------------------------------------
// Supported States
//--------------------------------------------------------------------------------------
// States supported by the StateManager. Set any of these states at render time with the SetState function
// All DirectX setup and cleanup is handled by this manager.

// States to show front or back or both sides of faces, also enable wireframe mode
enum class RasterizerState
{
	//// Rasterizer states
	// Front/Back face culling & wireframe mode
	CullBack, // Default
	CullFront,
	CullNone,
	CullBackWireframe,
	CullNoneWireframe,
};

// States to adjust how the depth buffer is used
enum class DepthState
{
	//// Depth Stencil states
	DepthOn, // Default
	DepthReadOnly,
	DepthOff,
};

// States to allow rendering to be blended with the background (transparent objects)
enum class BlendState
{
	BlendNone, // Default
	BlendAdditive,
	BlendMultiplicative,
	BlendAlpha,
};


//--------------------------------------------------------------------------------------
// State Manager Class
//--------------------------------------------------------------------------------------
class StateManager
{
	//--------------------------------------------------------------------------------------
	// Construction
	//--------------------------------------------------------------------------------------
public:
	// Create the state manager, pass DirectX device and context
	StateManager(ID3D11Device* device, ID3D11DeviceContext* context);


	//--------------------------------------------------------------------------------------
	// Usage
	//--------------------------------------------------------------------------------------
public:
	// Enable the given rasterizer state. Returns true on success. If it fails, use GetLastError to get a description of the error
	bool SetRasterizerState(RasterizerState state);

	// Enable the given depth state. Returns true on success. If it fails, use GetLastError to get a description of the error
	bool SetDepthState(DepthState state);

	// Enable the given blend state. Returns true on success. If it fails, use GetLastError to get a description of the error
	bool SetBlendState(BlendState state);

	// If the SetXXXState functions return false, the text description of the (most recent) error can be fetched with this function
	std::string GetLastError() { return mLastError; }


	//--------------------------------------------------------------------------------------
	// Private data
	//--------------------------------------------------------------------------------------
private:
	ID3D11Device*        mDXDevice;
	ID3D11DeviceContext* mDXContext;

	// Map of RasterizerState enum values to DirectX rasterizer state objects
	// CComPtr is a smart pointer that can be used for DirectX objects (similar to unique_ptr). Using CComPtr means we 
	// don't need a destructor to release everything from DirectX, it will happen automatically when the TextureManger
	// is destroyed. CComPtr requires <atlbase.h> to be included (and the ATL component to be installed as part of Visual Studio)
	std::map<RasterizerState, CComPtr<ID3D11RasterizerState>> mRasterizerStates;

	// Map of DepthState enum values to DirectX depth-stencil state objects. Using CComPtr smart pointer again (see comment above)
	std::map<DepthState, CComPtr<ID3D11DepthStencilState>> mDepthStates;

	// Map of BlendState enum values to DirectX blend state objects. Using CComPtr smart pointer again (see comment above)
	std::map<BlendState, CComPtr<ID3D11BlendState>> mBlendStates;

	// Description of the most recent error from LoadTexture or CreateSampler
	std::string mLastError;


	//--------------------------------------------------------------------------------------
	// Static private data
	//--------------------------------------------------------------------------------------
private:
	// States that are already set on the GPU
	// Uses these values to avoid sending requests to the GPU when the current GPU setting is already correct
	static RasterizerState mCurrentRasterizerState;
	static DepthState      mCurrentDepthState;
	static BlendState      mCurrentBlendState;
};


#endif //_STATE_H_INCLUDED_