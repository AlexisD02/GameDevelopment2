//--------------------------------------------------------------------------------------
// StateManager class encapsulates the handling of GPU pipeline states such as blending or culling
//--------------------------------------------------------------------------------------
// The state manager handles various global settings that adjust how the GPU renders:
// - Rasterizer state (Wireframe mode, don't cull back faces etc.)
// - Depth stencil state (How to use the depth and stencil buffer)
// - Blender state (Additive blending, alpha blending etc.)
// Each state requires a DirectX object to be created in advance and set when required.
// The StateManager class entirely manages these DirectX objects (creation and cleanup)
// States can be set at render time with a simple call specifying an enum value RenderState.

#include "State.h"

#include <stdexcept>


//--------------------------------------------------------------------------------------
// Construction
//--------------------------------------------------------------------------------------
// Create the state manager, pass DirectX device and context
StateManager::StateManager(ID3D11Device* device, ID3D11DeviceContext* context)
{
    mDXDevice  = device;
    mDXContext = context;

    //--------------------------------------------------------------------------------------
    // Rasterizer States
    //--------------------------------------------------------------------------------------
    // Rasterizer states adjust how triangles are filled in and when they are shown
    // Each block of code creates a rasterizer state
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    CComPtr <ID3D11RasterizerState> newRasterizerState;

    ////-------- Back face culling --------////
    // This is the usual mode - don't show inside faces of objects
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK; // Remove back faces
    rasterizerDesc.DepthClipEnable = TRUE; // Advanced setting - only used in rare cases

    // Create a DirectX object for the description above that can be used by a shader
    if (FAILED(mDXDevice->CreateRasterizerState(&rasterizerDesc, &newRasterizerState)))
    {
        throw std::runtime_error("Error creating cull-back state");
    }
    mRasterizerStates[RasterizerState::CullBack] = newRasterizerState;


    ////-------- Front face culling --------////
    // This is an unusual mode - it shows inside faces only so the model looks inside-out
    newRasterizerState = {}; // Need to null out CComPtr if we want to reuse the variable
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_FRONT; // Remove front faces
    rasterizerDesc.DepthClipEnable = TRUE;
    if (FAILED(mDXDevice->CreateRasterizerState(&rasterizerDesc, &newRasterizerState)))
    {
        throw std::runtime_error("Error creating cull-front state");
    }
    mRasterizerStates[RasterizerState::CullFront] = newRasterizerState;


    ////-------- No culling --------////
    // Used for transparent or flat objects - show both sides of faces
    newRasterizerState = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;  // Don't remove any faces
    rasterizerDesc.DepthClipEnable = TRUE;

    // Create a DirectX object for the description above that can be used by a shader
    if (FAILED(mDXDevice->CreateRasterizerState(&rasterizerDesc, &newRasterizerState)))
    {
        throw std::runtime_error("Error creating cull-none state");
    }
    mRasterizerStates[RasterizerState::CullNone] = newRasterizerState;


    ////-------- Wireframe mode with back face culling --------////
    // Wireframe, but don't see through the objects, only the outside will be shown
    newRasterizerState = {}; 
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthClipEnable = TRUE;
    if (FAILED(mDXDevice->CreateRasterizerState(&rasterizerDesc, &newRasterizerState)))
    {
        throw std::runtime_error("Error creating wireframe + cull back state");
    }
    mRasterizerStates[RasterizerState::CullBackWireframe] = newRasterizerState;


    ////-------- Wireframe mode with no culling --------////
    // See-throught wireframe mode
    newRasterizerState = {}; 
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;
    if (FAILED(mDXDevice->CreateRasterizerState(&rasterizerDesc, &newRasterizerState)))
    {
        throw std::runtime_error("Error creating wireframe + cull none state");
    }
    mRasterizerStates[RasterizerState::CullBackWireframe] = newRasterizerState;


    //--------------------------------------------------------------------------------------
    // Depth-Stencil States
    //--------------------------------------------------------------------------------------
    // Depth-stencil states adjust how the depth and stencil buffers are used. The stencil buffer is rarely used so 
    // these states are most often used to switch the depth buffer on and off. See depth buffers lab for details
    // Each block of code creates a rasterizer state
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    CComPtr<ID3D11DepthStencilState> newDepthState;

    ////-------- Enable depth buffer --------////
    // Enable depth buffer - default setting
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = FALSE;

    // Create a DirectX object for the description above that can be used by a shader
    if (FAILED(mDXDevice->CreateDepthStencilState(&depthStencilDesc, &newDepthState)))
    {
        throw std::runtime_error("Error creating use-depth-buffer state");
    }
    mDepthStates[DepthState::DepthOn] = newDepthState;


    ////-------- Enable depth buffer reads only --------////
    // Disables writing to depth buffer - used for transparent objects because they should not be entered in the buffer but do need to check if they are behind something
    newDepthState = {}; // Need to null out CComPtr if we want to reuse the variable
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Disable writing to depth buffer
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = FALSE;
    if (FAILED(mDXDevice->CreateDepthStencilState(&depthStencilDesc, &newDepthState)))
    {
        throw std::runtime_error("Error creating depth-read-only state");
    }
    mDepthStates[DepthState::DepthReadOnly] = newDepthState;


    ////-------- Disable depth buffer --------////
    // Entirely disable depth buffer, rendering will draw over everything
    newDepthState = {};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = FALSE;
    if (FAILED(mDXDevice->CreateDepthStencilState(&depthStencilDesc, &newDepthState)))
    {
        throw std::runtime_error("Error creating no-depth-buffer state");
    }
    mDepthStates[DepthState::DepthOff] = newDepthState;


    //--------------------------------------------------------------------------------------
    // Blending States
    //--------------------------------------------------------------------------------------
    // Blend states enable different kinds of transparency for rendered objects
    // Each block of code creates a blending mode
    D3D11_BLEND_DESC blendDesc = {};
    CComPtr<ID3D11BlendState> newBlendState;

    //** Leave the following settings alone, they are used only in highly unusual cases and will remain the same for all blending modes here
    //** Despite the word "Alpha" in the variable names, these are not the settings used for alpha blending
    blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;


    ////-------- No blending state --------////
    // Default rendering mode, no transparency
    blendDesc.RenderTarget[0].BlendEnable = FALSE;              // Disable blending
    blendDesc.RenderTarget[0].SrcBlend    = D3D11_BLEND_ONE;    // How to blend the source (texture colour)
    blendDesc.RenderTarget[0].DestBlend   = D3D11_BLEND_ZERO;   // How to blend the destination (colour already on screen)
    blendDesc.RenderTarget[0].BlendOp     = D3D11_BLEND_OP_ADD; // How to combine the above two, almost always ADD

    // Create a DirectX object for the description above that can be used by a shader
    if (FAILED(mDXDevice->CreateBlendState(&blendDesc, &newBlendState)))
    {
        throw std::runtime_error("Error creating no-blend state");
    }
    mBlendStates[BlendState::BlendNone] = newBlendState;


    ////-------- Additive Blending State --------////
    newBlendState = {}; // Need to null out CComPtr if we want to reuse the variable
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend    = D3D11_BLEND_ONE;    // ONE * the source (rendered colour)
    blendDesc.RenderTarget[0].DestBlend   = D3D11_BLEND_ONE;    // ONE * the destination (colour already on screen)
    blendDesc.RenderTarget[0].BlendOp     = D3D11_BLEND_OP_ADD; // ADD together, i.e. result is source + destination, additive blending
    if (FAILED(mDXDevice->CreateBlendState(&blendDesc, &newBlendState)))
    {
        throw std::runtime_error("Error creating additive blending state");
    }
    mBlendStates[BlendState::BlendAdditive] = newBlendState;


    ////-------- Multiplicative Blending State --------////
    newBlendState = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend    = D3D11_BLEND_ZERO;      // ZERO * the source (rendered colour)
    blendDesc.RenderTarget[0].DestBlend   = D3D11_BLEND_SRC_COLOR; // SRC_COLOR * the destination (colour already on screen)
    blendDesc.RenderTarget[0].BlendOp     = D3D11_BLEND_OP_ADD;    // ADD together, i.e. result is 0 * src_colour + src_colour * dest_colour,
                                                                   //               0 * part cancels out, leaving just src_color * dest_colour, multiplicative blending
    if (FAILED(mDXDevice->CreateBlendState(&blendDesc, &newBlendState)))
    {
        throw std::runtime_error("Error creating multuplicative blending state");
    }
    mBlendStates[BlendState::BlendMultiplicative] = newBlendState;


    ////-------- Alpha Blending State --------////
    newBlendState = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;                      // Enable blending
    blendDesc.RenderTarget[0].SrcBlend    = D3D11_BLEND_SRC_ALPHA;     // SRC_ALPHA * the source colour (rendered colour)
    blendDesc.RenderTarget[0].DestBlend   = D3D11_BLEND_INV_SRC_ALPHA; // (1- SRC_ALPHA) * destination colour  (colour already on screen)
    blendDesc.RenderTarget[0].BlendOp     = D3D11_BLEND_OP_ADD;        // ADD together, i.e. result is src_alpha * src_colour + (1 - src_apha) * destination colour, alpha blending
    if (FAILED(mDXDevice->CreateBlendState(&blendDesc, &newBlendState)))
    {
        throw std::runtime_error("Error creating alpha blending state");
    }
    mBlendStates[BlendState::BlendAlpha] = newBlendState;
}


//--------------------------------------------------------------------------------------
// Usage
//--------------------------------------------------------------------------------------

// Enable the given rasterizer state. Returns true on success. If it fails, use GetLastError to get a description of the error
bool StateManager::SetRasterizerState(RasterizerState state)
{
    if (mCurrentRasterizerState == state)   return true;
    if (!mRasterizerStates.contains(state))
    {
        mLastError = "Cannot find requested rasterizer state";
        return false;
    }
    auto newState = mRasterizerStates[state];
    mDXContext->RSSetState(newState);
    mCurrentRasterizerState = state;
    return true;
}

// Enable the given depth state. Returns true on success. If it fails, use GetLastError to get a description of the error
bool StateManager::SetDepthState(DepthState state)
{
    if (mCurrentDepthState == state)   return true;
    if (!mDepthStates.contains(state))
    {
        mLastError = "Cannot find requested depth state";
        return false;
    }
    auto newState = mDepthStates[state];
    mDXContext->OMSetDepthStencilState(newState, 0);
    mCurrentDepthState = state;
    return true;
}

// Enable the given blend state. Returns true on success. If it fails, use GetLastError to get a description of the error
bool StateManager::SetBlendState(BlendState state)
{
    if (mCurrentBlendState == state)   return true;
    if (!mBlendStates.contains(state))
    {
        mLastError = "Cannot find requested blend state";
        return false;
    }
    auto newState = mBlendStates[state];
    mDXContext->OMSetBlendState(newState, nullptr, 0xffffff);
    mCurrentBlendState = state;
    return true;
}


//--------------------------------------------------------------------------------------
// Static private data
//--------------------------------------------------------------------------------------

// States that are already set on the GPU
// Uses these values to avoid sending requests to the GPU when the current GPU setting is already correct
RasterizerState StateManager::mCurrentRasterizerState = RasterizerState::CullBack;
DepthState      StateManager::mCurrentDepthState      = DepthState::     DepthOn;
BlendState      StateManager::mCurrentBlendState      = BlendState::     BlendNone;
