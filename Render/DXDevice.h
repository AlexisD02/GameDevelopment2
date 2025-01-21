//--------------------------------------------------------------------------------------
// Class encapsulating DirectX
//--------------------------------------------------------------------------------------
// Handles DirectX setup / shutdown and provides manager classes to work with DirectX objects (e.g. textures, shaders)

#ifndef _DX_DEVICE_H_INCLUDED_
#define _DX_DEVICE_H_INCLUDED_

//#include "Common.h"

#define NOMINMAX // Use NOMINMAX to stop certain Microsoft headers defining "min" and "max", which breaks some libraries (e.g. assimp)
#include <d3d11.h>
#include <dxgi1_4.h>
#include <atlbase.h> // For CComPtr (see member variables)

#include <memory>


// Forward declarations of Manager classes allows us to use Manager class pointers before those classes have been fully declared
// This reduces the number of include files needed here, which in turn minimises dependencies and speeds up compilation
//
// Note: if you have a class containing a unique_ptr to a forward declaration (that is the case here) then this class must also have a destructor
// signature here and declaration in the cpp file - this is required even if the destructor is default/empty. Otherwise you get compile errors.
class StateManager;
class ShaderManager;
class TextureManager;
class CBufferManager;


//--------------------------------------------------------------------------------------
// DXDevice Class
//--------------------------------------------------------------------------------------
class DXDevice
{
	/*-----------------------------------------------------------------------------------------
	   Construction
	-----------------------------------------------------------------------------------------*/
public:
	// Creates DirectX, a swap chain (front/back buffer) attached to given window, a depth buffer of matching size and 
	// manager classes for DirectX resources such as shaders or textures
	DXDevice(HWND window);

	// DirectX cleanup
	~DXDevice();


	/*-----------------------------------------------------------------------------------------
	   Data Access
	-----------------------------------------------------------------------------------------*/
public:
	ID3D11Device*        Device()  { return mD3DDevice;  }
	ID3D11DeviceContext* Context() { return mD3DContext; }

	ID3D11RenderTargetView*& BackBuffer()  { return mBackBufferRenderTarget.p; }
	ID3D11DepthStencilView*& DepthBuffer() { return mDepthStencil.p;           }

	unsigned int GetBackbufferWidth()   { return mBackbufferWidth;  }
	unsigned int GetBackbufferHeight()  { return mBackbufferHeight; }

	auto States()   { return mStateManager  .get(); }
	auto Shaders()  { return mShaderManager .get(); }
	auto Textures() { return mTextureManager.get(); }
	auto CBuffers() { return mCBufferManager.get(); }


	/*-----------------------------------------------------------------------------------------
	   Usage
	-----------------------------------------------------------------------------------------*/

	// Tell DirectX that rendering to the back buffer is finished and it can be presented to the screen
	// Pass true to lock FPS to monitor refresh rate
	void PresentFrame(bool vsync);


	/*-----------------------------------------------------------------------------------------
	   Private Data
	-----------------------------------------------------------------------------------------*/
private:
	// Current viewport dimensions (not supporting "render scale" so this will be the size of the window and the back-buffer)
	int mBackbufferWidth;
	int mBackbufferHeight;

	// The main Direct3D (D3D) variables
	CComPtr<ID3D11Device>        mD3DDevice;  // D3D device for general GPU control
	CComPtr<ID3D11DeviceContext> mD3DContext; // D3D context for specific rendering tasks

	// Back buffer (where we render to) and swap chain (handles how the back buffer is presented to the screen)
	CComPtr<ID3D11Texture2D>        mBackBufferTexture;
	CComPtr<ID3D11RenderTargetView> mBackBufferRenderTarget;
	CComPtr<IDXGISwapChain1>        mSwapChain;

	// Depth buffer
	CComPtr<ID3D11Texture2D>          mDepthStencilTexture; // The texture holding the depth values
	CComPtr<ID3D11DepthStencilView>   mDepthStencil;        // The depth buffer itself, that uses the above texture
	CComPtr<ID3D11ShaderResourceView> mDepthShaderView;     // Allows access to the depth buffer as a texture in certain specialised shaders

	// Manager classes for DirectX resources. Each manager class creates DirectX resources of the given type
	// and maintains pointers to them. When the manager is destroyed its DirectX resources are released
	std::unique_ptr<StateManager>   mStateManager;
	std::unique_ptr<ShaderManager>  mShaderManager;
	std::unique_ptr<TextureManager> mTextureManager;
	std::unique_ptr<CBufferManager> mCBufferManager;
};


#endif //_DX_DEVICE_H_INCLUDED_
