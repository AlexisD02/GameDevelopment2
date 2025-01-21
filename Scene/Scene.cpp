//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"

#include "Boat.h"
#include "EntityManager.h"
#include "Entity.h"
#include "Camera.h"
#include "SceneGlobals.h"

#include "Mesh.h"
#include "RenderMethod.h"
#include "Texture.h"
#include "CBuffer.h"
#include "CBufferTypes.h"
#include "State.h"
#include "RenderGlobals.h"

#include "Matrix4x4.h" 
#include "Vector3.h" 
#include "ColourTypes.h" 
#include "Input.h"

#include <sstream>
#include <cmath>
#include <stdexcept>


//--------------------------------------------------------------------------------------
// Initialise scene geometry
//--------------------------------------------------------------------------------------

// Constructs a demo scene ready to be rendered/updated
Scene::Scene()
{
	// Create the global constant buffers used by this app
	if (!CreateCBuffers())  throw std::runtime_error("Error creating constant buffers");

	// Create entity manager and messenger prior to any entities
	gEntityManager = std::make_unique<EntityManager>();
	gMessenger     = std::make_unique<Messenger>();

	// Initialise SpriteFont helper library for text drawing
	mSpriteBatch = std::make_unique<DirectX::DX11::SpriteBatch>(DX->Context());
	mSmallFont   = std::make_unique<DirectX::DX11::SpriteFont>(DX->Device(), L"tahoma12.spritefont");
	mMediumFont  = std::make_unique<DirectX::DX11::SpriteFont>(DX->Device(), L"tahoma16.spritefont");


	////// Camera

	mCamera = std::make_unique<Camera>();
	mCamera->Transform().Position() = { 120, 80, -180 };
	mCamera->Transform().SetRotation({ ToRadians(8), ToRadians(-40), 0 });
	mCamera->SetAspectRatio(static_cast<float>(DX->GetBackbufferWidth()) / DX->GetBackbufferHeight());
	mCamera->SetFOV(ToRadians(75));
	mCamera->SetNearClip(1);
	mCamera->SetFarClip(10000);


	////// Load Entity templates

	// Create entity templates for scenery (equivalent to loading a mesh in TL-Engine, but with additional base stats)
	// Put entity template type in <>, then parameters are: template name, mesh filename and optional mesh import flags
	// Ignoring the returned template pointers as we don't use them (entities are created using the template name not pointer)
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Sky",       "Sky.fbx",  ImportFlags::NoLighting); // Skybox will be a plain texture - no lighting effects
	gEntityManager->CreateEntityTemplate<EntityTemplate>("WaterFar",  "WaterFar.fbx");
	gEntityManager->CreateEntityTemplate<EntityTemplate>("WaterNear", "WaterNear.fbx");
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Snow1",     "Snow1.fbx"   );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Snow2",     "Snow2.fbx"   );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Snow3",     "Snow3.fbx"   );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Snow4",     "Snow4.fbx"   );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Snow5",     "Snow5.fbx"   );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Rock1",     "Rock1.fbx"   );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Rock2",     "Rock2.fbx"   );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Pillar",    "Pillar.fbx"  );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Light",     "Light.x"     );
	gEntityManager->CreateEntityTemplate<EntityTemplate>("Missile",   "Missile.fbx" );

	// Create entity templates for more complex entities
	//**** IMPORTANT ****
	// Note the template type on the call below is <BoatTemplate> so CreateEntityTemplate will create a BoatTemplate
	// The BoatTemplate constructor takes a name for the template, the mesh file to use, then several other parameters
	// to customise that boat type: maxSpeed, acceleration, turnSpeed, gunTurnSpeed, maxHP, missileDamage
	// There is also a final optional import settings parameter but not using that here
	// Intellisense does not know about this system and will not show you these parameters - take care if you make your own template classes
	gEntityManager->CreateEntityTemplate<BoatTemplate>("Blue Tanker",   "Boat1.fbx", 25.0f,  8.0f, 0.8f, 1.2f, 100.0f, 25.0f); // Template name, mesh name, boat stats
	gEntityManager->CreateEntityTemplate<BoatTemplate>("Green Fleeter", "Boat2.fbx", 35.0f, 10.0f, 1.0f, 1.0f,  80.0f, 20.0f);
	gEntityManager->CreateEntityTemplate<BoatTemplate>("Purple Eater",  "Boat3.fbx", 15.0f,  6.0f, 1.0f, 0.8f,  90.0f, 40.0f);

	// Test for any errors loading entity templates
	if (gEntityManager->GetLastError() != "")  throw std::runtime_error(gEntityManager->GetLastError());


	////// Create Entities

	// Creates scenery entities (instances - equivalent of TL-Engine models)
	// Put entity type in <>, then parameters are: template name (from templates created above), then initial matrix (position, rotation, scale)
	// Create static entities - ignore the returned entity IDs for these as we will never do anything with them
	// Pass template to use for this entity then an optional initial transform matrix and optional name
	// Matrix4x4 can be initialised like this:  Matrix4x4(position, [optional]rotation, [optional]scale), where scale can be a float or a Vector3
	gEntityManager->CreateEntity<Entity>("Sky",    Matrix4x4({ 0, 0, 0 }, { 0, 0, 0 }, 75.0f));
	gEntityManager->CreateEntity<Entity>("WaterFar");
	gEntityManager->CreateEntity<Entity>("WaterNear");
	gEntityManager->CreateEntity<Entity>("Snow2",  Matrix4x4({ -550, -5,  780 }, { 0, ToRadians(-40), 0 }));
	gEntityManager->CreateEntity<Entity>("Snow1",  Matrix4x4({ -900,  0,  450 }, { 0, ToRadians(-65), 0 }));
	gEntityManager->CreateEntity<Entity>("Snow2",  Matrix4x4({ -800,  0,  300 }, { 0, ToRadians(-90), 0 }));
	gEntityManager->CreateEntity<Entity>("Snow4",  Matrix4x4({ -700,  0, -300 }, { 0, ToRadians( 95), 0 }));
	gEntityManager->CreateEntity<Entity>("Snow3",  Matrix4x4({ -400,  0, -550 }, { 0, ToRadians( 80), 0 }));
	gEntityManager->CreateEntity<Entity>("Snow5",  Matrix4x4({  250,  0,  860 }, { 0, ToRadians(110), 0 }));
	gEntityManager->CreateEntity<Entity>("Rock1",  Matrix4x4({ 1200,  0,  200 }, { 0, ToRadians( 90), 0 }));
	gEntityManager->CreateEntity<Entity>("Rock2",  Matrix4x4({  600,  0, -700 }, { 0, ToRadians(140), 0 }));
	gEntityManager->CreateEntity<Entity>("Pillar", Matrix4x4({  300,  0, -700 }, { 0, ToRadians( 20), 0 }));

	// Create game entities
	//**** IMPORTANT ****
	// Note the template type on the call below is <Boat> so CreateEntity will create a Boat entity
	// The Boat constructor takes an EntityTemplate* and an EntityID, then other parameters
	// For CreateEntity we pass a template name "Blue Tanker" instead of the EntityTemplate* and an EntityID
	// After that we can pass any other parameters that the Boat constructor uses, first the initial speed (see Boat.h), 
	// then an optional matrix transform and name
	// Intellisense does not know about this system and will not show you the required parameters - take care
	mBoat1 = gEntityManager->CreateEntity<Boat>("Blue Tanker",   15.0f, Matrix4x4({ -50, -1.5f,  0 }, { 0, 0, 0 }, 1.0f), "Murphy"); // Template to use, initial speed, initial transform, name
	mBoat2 = gEntityManager->CreateEntity<Boat>("Green Fleeter", 25.0f, Matrix4x4({  50, -1.5f,  0 }, { 0, ToRadians(180), 0 }, 1.0f), "Sly");
	mBoat3 = gEntityManager->CreateEntity<Boat>("Purple Eater",  22.0f, Matrix4x4({   0, -1.5f, 50 }, { 0, ToRadians( 90), 0 }, 1.0f), "Slam");

	// Create light entity (visual representation of light), scale of light adjusts its brightness
	mLight = gEntityManager->CreateEntity<Entity>("Light", Matrix4x4({ -3250, 8000, -10000 }, { 0, 0, 0 }, 150.0f));

	// Test for any errors creating entities
	if (gEntityManager->GetLastError() != "")  throw std::runtime_error(gEntityManager->GetLastError());


	////// Additional scene setup

	// Set light information
	// Note how we convert the light Entity ID (mLight1) to a pointer to use it.
	// We could check lightPtr isn't nullptr, but no need as we are certain it will exist here. However, in gameplay code with entities that may not be the case
	auto lightPtr = gEntityManager->GetEntity(mLight);
	lightPtr->RenderColour() = {1.0f, 0.6f, 0.2f};  // The light mesh will be tinted to the light's colour
	lightPtr->RenderGroup() = 1;                    // Put additive blended entities in a different render group

	// Ambient light is used as global illumination in Blinn-Phong lighting (2nd year style graphics - see mesh code)
	// Global illumination is the light from the scene that is not directly from light sources
	mAmbientColour = { 0.5f, 0.5f, 0.5f };

	// When using PBR we use an environment map for global illumination - a cube map of the scene we are in
	std::tie(std::ignore, mEnvironmentMap) = DX->Textures()->LoadTexture("Media/sea-cube.dds", true);
	RenderState::SetEnvironmentMap(mEnvironmentMap);
}


// Empty destructor - nothing to do, but see comment on forward declarations in header file
Scene::~Scene() {}


//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render the entire scene
void Scene::Render()
{
	// Setup the rendering viewport to the size of the main window
	D3D11_VIEWPORT vp;
	vp.Width =  static_cast<FLOAT>(DX->GetBackbufferWidth());
	vp.Height = static_cast<FLOAT>(DX->GetBackbufferHeight());
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	DX->Context()->RSSetViewports(1, &vp);


	// Put data that's constant for the entire frame (e.g. current light information) into the constant buffer
	auto lightPtr = gEntityManager->GetEntity(mLight);
	float light1Intensity = lightPtr->Transform().GetScale().x * lightPtr->Transform().GetScale().x; // Square the light model's scale to get the light intensity
	gPerFrameConstants.light1Colour   = lightPtr->RenderColour();
	gPerFrameConstants.light1Colour   = (Vector3)gPerFrameConstants.light1Colour * light1Intensity;// Temporarily use vector maths on the colour (vector3 * float)
	gPerFrameConstants.light1Position = lightPtr->Transform().Position();

	gPerFrameConstants.ambientColour  = mAmbientColour;

	gPerFrameConstants.viewportWidth  = static_cast<float>(DX->GetBackbufferWidth());
	gPerFrameConstants.viewportHeight = static_cast<float>(DX->GetBackbufferHeight());

	// Send over to GPU
	DX->CBuffers()->UpdateCBuffer(gPerFrameConstantBuffer, gPerFrameConstants);


	// Render the scene from the main camera
	RenderFromCamera(mCamera.get());


	// Output UI text for cars
	mSpriteBatch->Begin(); // Using DirectX helper library SpriteBatch to draw text

	// Write type of each boat - helper function uses 3D->2D conversion (picking) to draw text at 3D world point. Note the check to ensure the entity exists
	auto boat1Ptr = gEntityManager->GetEntity(mBoat1);
	auto boat2Ptr = gEntityManager->GetEntity(mBoat2);
	auto boat3Ptr = gEntityManager->GetEntity(mBoat3);
	if (boat1Ptr != nullptr)  DrawTextAtWorldPt(boat1Ptr->Transform().Position(), boat1Ptr->Template().GetType(), mCamera.get(), true, ColourRGB(0x6060ff));
	if (boat2Ptr != nullptr)  DrawTextAtWorldPt(boat2Ptr->Transform().Position(), boat2Ptr->Template().GetType(), mCamera.get(), true, ColourRGB(0x00ff00));
	if (boat3Ptr != nullptr)  DrawTextAtWorldPt(boat3Ptr->Transform().Position(), boat3Ptr->Template().GetType(), mCamera.get(), true, ColourRGB(0xff00ff));

	mSpriteBatch->End();
	RenderState::Reset(); // Must call this after using SpriteBatch functions


	// Rendering is complete, "present" the image to the screen
	DX->PresentFrame(mLockFPS);
}


// Render one viewpoint of the scene from the given camera, helper function for Scene::Render
void Scene::RenderFromCamera(Camera* camera)
{
	// Set camera matrices in the constant buffer and send over to GPU
	gPerCameraConstants.cameraMatrix         = camera->Transform();
	gPerCameraConstants.viewMatrix           = camera->GetViewMatrix();
	gPerCameraConstants.projectionMatrix     = camera->GetProjectionMatrix();
	gPerCameraConstants.viewProjectionMatrix = camera->GetViewProjectionMatrix();
	gPerCameraConstants.cameraPosition       = camera->Transform().Position();
	DX->CBuffers()->UpdateCBuffer(gPerCameraConstantBuffer, gPerCameraConstants);

	// Target the back buffer for rendering, clear depth buffer
	DX->Context()->OMSetRenderTargets(1, &DX->BackBuffer(), DX->DepthBuffer());
	DX->Context()->ClearDepthStencilView(DX->DepthBuffer(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Render solid models (render group 0)
	DX->States()->SetRasterizerState(RasterizerState::CullBack); // Default GPU states for non-blended rendering
	DX->States()->SetDepthState(DepthState::DepthOn);
	DX->States()->SetBlendState(BlendState::BlendNone);
	gEntityManager->RenderGroup(0);

	// Render additive blended models (render group 1)
	DX->States()->SetRasterizerState(RasterizerState::CullNone); // GPU states for additive blending
	DX->States()->SetDepthState(DepthState::DepthReadOnly);      // Don't write to depth buffer to stop sorting errors on additive / multiplicative blending and similar
	DX->States()->SetBlendState(BlendState::BlendAdditive);
	gEntityManager->RenderGroup(1);
}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------

// Update entire scene. frameTime is the time passed since the last frame
void Scene::Update(float frameTime)
{
	// Update all entities
	gEntityManager->UpdateAll(frameTime);

	// Control camera
	static float movementSpeed = 40.0f;
	const float rotationSpeed = 1.5f;   // Radians per second for rotation
	if (KeyHit(Key_F1))  movementSpeed = 40.0f;
	if (KeyHit(Key_F2))  movementSpeed = 200.0f;
	mCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D, movementSpeed, rotationSpeed);

	// Toggle FPS limiting
	if (KeyHit(Key_P))  mLockFPS = !mLockFPS;
}


//--------------------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------------------

// Draw given text at the given 3D point, also pass camera in use. Optionally centre align and colour the text
void Scene::DrawTextAtWorldPt(const Vector3& point, std::string text, Camera* camera, bool centreAlign /*= false*/, ColourRGB colour /*= { 1, 1, 1 }*/)
{
	auto pixelPt = camera->PixelFromWorldPt(point, DX->GetBackbufferWidth(), DX->GetBackbufferHeight());
	if (pixelPt.z >= camera->GetNearClip())
	{
		if (centreAlign)
		{
			auto textSize = mSmallFont->MeasureString(text.c_str());
			pixelPt.x -= DirectX::XMVectorGetX(textSize) / 2;
		}
		mSmallFont->DrawString(mSpriteBatch.get(), text.c_str(), DirectX::XMFLOAT2{ pixelPt.x, pixelPt.y }, DirectX::FXMVECTOR{ colour.r, colour.g, colour.b, 0 });
	}
}

