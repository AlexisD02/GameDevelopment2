//--------------------------------------------------------------------------------------
// Class encapsulating DirectX
//--------------------------------------------------------------------------------------
// Handles DirectX setup / shutdown and provides manager classes to work with DirectX objects (e.g. textures, shaders)

#include "DXDevice.h"

#include "State.h"
#include "Shader.h"
#include "Texture.h"
#include "CBuffer.h"

#include <stdexcept>


//--------------------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------------------

// Creates DirectX, a swap chain (front/back buffer) attached to given window, a depth buffer of matching size and 
// manager classes for DirectX resources such as shaders or textures
DXDevice::DXDevice(HWND window)
{
    bool debug = true; // Set to true to emit DirectX warnings/errors to the output window

    // Get the window size
    RECT rect;
    if (!GetClientRect(window, &rect))   throw std::runtime_error("Error querying window size");
    mBackbufferWidth  = rect.right - rect.left;
    mBackbufferHeight = rect.bottom - rect.top;


    // Create a DXGI factory - allows us to query graphics hardware / monitors prior to initialising DirectX (although not doing that here)
    HRESULT hr = S_OK;
    CComPtr<IDXGIFactory4> dxgiFactory;
    hr = CreateDXGIFactory2(debug ? DXGI_CREATE_FACTORY_DEBUG : 0, __uuidof(IDXGIFactory4), (void**)(&dxgiFactory));


    // Initialise Direct3D device
    UINT flags = debug ? D3D11_CREATE_DEVICE_DEBUG : 0;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, flags, 0, 0, D3D11_SDK_VERSION, &mD3DDevice, nullptr, &mD3DContext);
    if (FAILED(hr))  throw std::runtime_error("Error creating Direct3D device");


    // Create a swap-chain (a back buffer / front buffer to render to) set up for the window passed to this constructor
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width  = mBackbufferWidth;
    scDesc.Height = mBackbufferHeight;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;
    hr = dxgiFactory->CreateSwapChainForHwnd(mD3DDevice, window, &scDesc, nullptr, nullptr, &mSwapChain);


    // Get the back buffer texture from the swap chain we just created - primarily needed for the next step
    hr = mSwapChain->GetBuffer(0, __uuidof(mBackBufferTexture), (LPVOID*)&mBackBufferTexture);
    if (FAILED(hr))  throw std::runtime_error("Error creating swap chain");
    
    // Create a "render target view" of the back buffer - we need this to render to the back buffer
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = mD3DDevice->CreateRenderTargetView(mBackBufferTexture, &rtvDesc, &mBackBufferRenderTarget);
    if (FAILED(hr))  throw std::runtime_error("Error creating render target view");


    // Create depth buffer to go along with the back buffer. First create a texture to hold the depth buffer values
    D3D11_TEXTURE2D_DESC dbDesc = {};
    dbDesc.Width     = mBackbufferWidth; // Same size as viewport / back-buffer
    dbDesc.Height    = mBackbufferHeight;
    dbDesc.MipLevels = 1;
    dbDesc.ArraySize = 1;
    dbDesc.Format    = DXGI_FORMAT_R32_TYPELESS; // Important point for when using depth buffer as texture, must use the TYPELESS constant shown here
    dbDesc.SampleDesc.Count   = 1;
    dbDesc.SampleDesc.Quality = 0;
    dbDesc.Usage              = D3D11_USAGE_DEFAULT;
    dbDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // Using this depth buffer in shaders so must say so;
    dbDesc.CPUAccessFlags     = 0;
    dbDesc.MiscFlags          = 0;
    hr = mD3DDevice->CreateTexture2D(&dbDesc, nullptr, &mDepthStencilTexture);
    if (FAILED(hr))  throw std::runtime_error("Error creating depth buffer texture");


    // Create the depth stencil view - an object to allow us to use the texture just created as a depth buffer
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    hr = mD3DDevice->CreateDepthStencilView(mDepthStencilTexture, &dsvDesc, &mDepthStencil);
    if (FAILED(hr))  throw std::runtime_error("Error creating depth buffer view");


    // Also create a shader resource view for the depth buffer - required when we want to access the depth buffer as a texture
    D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
    descSRV.Format = DXGI_FORMAT_R32_FLOAT;
    descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    descSRV.Texture2D.MipLevels = 1;
    descSRV.Texture2D.MostDetailedMip = 0;
    hr = mD3DDevice->CreateShaderResourceView(mDepthStencilTexture, &descSRV, &mDepthShaderView);
    if (FAILED(hr))  throw std::runtime_error("Error creating depth buffer shader resource view");


    // Create DirectX resource managers - can only do this after DirectX has been successfully initialised.
    // These functions will throw std::runtime_error if they fail
    mStateManager   = std::make_unique<StateManager>  (mD3DDevice, mD3DContext);
    mShaderManager  = std::make_unique<ShaderManager> (mD3DDevice, mD3DContext);
    mTextureManager = std::make_unique<TextureManager>(mD3DDevice, mD3DContext);
    mCBufferManager = std::make_unique<CBufferManager>(mD3DDevice, mD3DContext);
}


// Using destructor to detach all resources from each other to ensure correct DirectX clean up. Also see comment on forward declarations in header file
DXDevice::~DXDevice()
{
    if (mD3DContext)  mD3DContext->ClearState();
}


/*-----------------------------------------------------------------------------------------
   Usage
-----------------------------------------------------------------------------------------*/

// Tell DirectX that rendering to the back buffer is finished and it can be presented to the screen
// Pass true to lock FPS to monitor refresh rate
void DXDevice::PresentFrame(bool vsync)
{
    DXGI_PRESENT_PARAMETERS presentParams = {};
    mSwapChain->Present1(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_DO_NOT_WAIT, &presentParams);
}
