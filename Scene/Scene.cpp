//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include "ParseLevel.h" 

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

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <sstream>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <algorithm>


//--------------------------------------------------------------------------------------
// Initialise scene geometry
//--------------------------------------------------------------------------------------

// Constructs a scene ready to be rendered/updated
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

    //----------------------------------------------------------------------
    // Load the level from XML.
    //----------------------------------------------------------------------
    {
        // Create an instance of the XML parser and pass it our entity manager.
        ParseLevel levelParser(*gEntityManager);
        // Adjust the file path as needed.
        if (!levelParser.ParseFile("Entities.xml"))
        {
            throw std::runtime_error("Error parsing level file (Entities.xml)");
        }
    }

    ////// Camera

    mCamera = std::make_unique<Camera>();
    mCamera->Transform().Position() = { 120, 80, -180 };
    mCamera->Transform().SetRotation({ ToRadians(8), ToRadians(-40), 0 });
    mCamera->SetAspectRatio(static_cast<float>(DX->GetBackbufferWidth()) / DX->GetBackbufferHeight());
    mCamera->SetFOV(ToRadians(75));
    mCamera->SetNearClip(1);
    mCamera->SetFarClip(10000);

    // Test for any errors loading entity templates
    if (gEntityManager->GetLastError() != "")  throw std::runtime_error(gEntityManager->GetLastError());

    allBoats = gEntityManager->GetAllBoatEntities();

    for (Boat* boatPtr : allBoats)
    {
        if (!boatPtr) continue;

        auto chaseCamera = std::make_unique<Camera>();

        // Configure the chase camera's default settings
        chaseCamera->SetAspectRatio(static_cast<float>(DX->GetBackbufferWidth()) / DX->GetBackbufferHeight());
        chaseCamera->SetFOV(ToRadians(60)); // Slightly narrower FOV for chase cameras
        chaseCamera->SetNearClip(0.1f);
        chaseCamera->SetFarClip(10000.0f);

        // Set the chase camera's position and orientation based on the boat
        Vector3 boatPos = boatPtr->Transform().Position();
        Vector3 boatForward = boatPtr->Transform().ZAxis(); // Assuming ZAxis points forward

        // Position the camera behind and above the boat
        Vector3 cameraPos = boatPos - boatForward * mChaseDistance + Vector3(0, mChaseHeight, 0);
        chaseCamera->Transform().Position() = cameraPos;

        // Calculate yaw based on the boat's forward direction
        float yaw = std::atan2(boatForward.x, boatForward.z);

        // Set a fixed downward pitch angle
        float pitch = mChasePitch;

        // Apply rotation to the chase camera
        chaseCamera->Transform().SetRotation({ pitch, yaw, 0 });

        // Store the chase camera
        mChaseCameras.emplace_back(std::move(chaseCamera));
    }

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
    //*******************************
    // Prepare ImGUI for this frame
    //*******************************

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

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

    // Determine which camera to use
    Camera* activeCamera = mCamera.get(); // Default main camera
    if (mActiveCameraIndex >= 0 && mActiveCameraIndex < static_cast<int>(mChaseCameras.size()))
    {
        // Ensure that the camera in use is still valid.
        Camera* potentialCamera = mChaseCameras[mActiveCameraIndex].get();
        if (potentialCamera)
            activeCamera = potentialCamera;
        else
            mActiveCameraIndex = -1;
    }

    // Render the scene from the active camera
    RenderFromCamera(activeCamera);

    // Output UI text for boats
    mSpriteBatch->Begin(); // Using DirectX helper library SpriteBatch to draw text

    allBoats = gEntityManager->GetAllBoatEntities();

    for (Boat* boatPtr : allBoats)
    {
        if (!boatPtr) continue;

        std::string text;

        if (!mShowExtendedBoatUI)
        {
            text = boatPtr->Template().GetType() + ": " + boatPtr->GetName();
        }
        else
        {
            float hp = boatPtr->GetHP();
            std::string state = boatPtr->GetState();
            int fired = boatPtr->GetMissilesFired();
            int missilesLeft = boatPtr->GetMissilesRemaining();
            float speed = boatPtr->GetSpeed();

            // Format speed with two decimal places
            std::ostringstream speedStream;
            speedStream << std::fixed << std::setprecision(2) << speed;

            text = boatPtr->GetName()
                + " [HP=" + std::to_string((int)hp)
                + ", State=" + state
                + ", Fired=" + std::to_string(fired)
                + ", Missiles=" + std::to_string(missilesLeft)
                + ", Speed=" + speedStream.str()
                + "]";
        }
        // Determine label color
        if (mSelectedBoat && (boatPtr == mSelectedBoat)) { colour = ColourRGB(0xffff00); } // Yellow for selected entity
        else if (mNearestEntity && (boatPtr == mNearestEntity)) { colour = ColourRGB(0xff0000); } // Red for nearest entity
        else if (boatPtr->GetTeam() == Team::TeamA) { colour = ColourRGB(0x6060ff); } // Blue for team A
        else if (boatPtr->GetTeam() == Team::TeamB) { colour = ColourRGB(0x00ff00); } // Green for team B
        else if (boatPtr->GetTeam() == Team::TeamC) { colour = ColourRGB(0x9932CC); } // Dark Orchid for team C
        else { colour = ColourRGB(0xffffff); }

        Vector3 boatPos = boatPtr->Transform().Position();

        // Draw the text label using activeCamera
        DrawTextAtWorldPt(boatPos, text, activeCamera, true);

        if (!boatPtr->GetBoatText().empty())
        {
            float timeLeft = boatPtr->GetBoatTextTimer();
            float maxTime = 3.0f;
            float baseOffset = 10.0f;
            float totalRise = 10.0f;

            float timeSinceBoatBehaviour = maxTime - timeLeft;
            float fraction = timeSinceBoatBehaviour / maxTime;

            if (fraction > 1.0f) fraction = 1.0f;

            float extraOffset = totalRise * fraction;
            float finalOffset = baseOffset + extraOffset;

            // Position the an text above the boat for certain time
            Vector3 boatAddLabelPos = boatPos + Vector3(0.0f, finalOffset, 0.0f);

            colour = ColourRGB(0xffcc00);

            DrawTextAtWorldPt(boatAddLabelPos, boatPtr->GetBoatText(), activeCamera, true);
        }
    }

    HandleMousePicking();

    reloadStations = gEntityManager->GetAllReloadStationEntities();
    for (ReloadStation* reloadStation : reloadStations)
    {
        // Prepare the text label for the reload station
        std::string text = reloadStation->GetName();

        // Draw the text label above the reload station
        Vector3 labelPosition = reloadStation->Transform().Position() + Vector3(0, 10, 0);
        colour = ColourRGB(0xffffff);

        DrawTextAtWorldPt(labelPosition, text, activeCamera, true);
    }

    mSpriteBatch->End();
    RenderState::Reset(); // Must call this after using SpriteBatch functions

    //*******************************
    // Draw ImGUI interface
    //*******************************
    // Draw ImGUI elements at any time between the frame preparation code at the top
    // of this function, and the finalisation code below
    DrawGUI();

    //*******************************
    // Finalise ImGUI for this frame
    //*******************************
    ImGui::Render();
    DX->Context()->OMSetRenderTargets(1, &DX->BackBuffer(), nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Rendering is complete, "present" the image to the screen
    DX->PresentFrame(mLockFPS);
}

void Scene::DrawGUI() {
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("CO3301 Game Development - Control Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // ===================== Global Settings =====================
    if (ImGui::CollapsingHeader("Global Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Lock FPS
        static bool lockFPS = mLockFPS;
        if (ImGui::Checkbox("Lock FPS", &lockFPS)) {
            mLockFPS = lockFPS;
        }

        // Ambient Light
        static float ambientLight = mAmbientColour.r;
        if (ImGui::SliderFloat("Ambient Light", &ambientLight, 0.0f, 1.0f)) {
            mAmbientColour = { ambientLight, ambientLight, ambientLight };
        }

        // Pause Game
        static bool pauseGame = false;
        if (ImGui::Checkbox("Pause Game", &pauseGame)) {
            SetPauseState(pauseGame);
        }
    }

    // ===================== Boat Management =====================
    if (ImGui::CollapsingHeader("Boat Management")) {
        // Build a list of boat names and their IDs.
        std::vector<std::pair<std::string, EntityID>> boatData;
        allBoatIDS = gEntityManager->GetAllBoatIDS();

        // Populate boatData only with valid boat entities.
        for (EntityID boatID : allBoatIDS) {
            Boat* boatPtr = gEntityManager->GetEntity<Boat>(boatID);
            if (!boatPtr) continue;
            boatData.emplace_back(boatPtr->GetName(), boatID);
        }

        // Create a vector of const char* for the ImGui combo
        std::vector<const char*> boatNames;
        for (const auto& entry : boatData) {
            boatNames.push_back(entry.first.c_str());
        }

        // Display the boat selection dropdown
        static int selectedBoatIndex = -1;
        if (ImGui::Combo("Select Boat", &selectedBoatIndex, boatNames.data(), static_cast<int>(boatNames.size()))) {
            if (selectedBoatIndex >= 0 && selectedBoatIndex < static_cast<int>(boatData.size())) {
                // Retrieve the selected boat ID and update mSelectedUIBoat
                EntityID selectedBoatID = boatData[selectedBoatIndex].second;
                mSelectedUIBoat = gEntityManager->GetEntity<Boat>(selectedBoatID);
            }
        }

        // Validate mSelectedUIBoat: re-check that it still exists.
        if (mSelectedUIBoat) {
            Boat* temp = gEntityManager->GetEntity<Boat>(mSelectedUIBoat->GetID());
            if (!temp) {
                mSelectedUIBoat = nullptr;
                selectedBoatIndex = -1;
            }
            else {
                mSelectedUIBoat = temp;
            }
        }

        // Display Selected Boat Details if one is selected.
        if (mSelectedUIBoat) {
            ImGui::Text("Selected Boat: %s", mSelectedUIBoat->GetName().c_str());
            ImGui::Text("State: %s", mSelectedUIBoat->GetState().c_str());
            ImGui::Text("Speed: %.2f", mSelectedUIBoat->GetSpeed());

            if (ImGui::Button("Spectate Boat")) {
                // Find the chase camera that is closest to the selected boat.
                if (mSelectedUIBoat) {
                    Vector3 boatPos = mSelectedUIBoat->Transform().Position();
                    int bestIndex = -1;
                    float bestDistance = FLT_MAX;
                    for (size_t i = 0; i < mChaseCameras.size(); ++i) {
                        Camera* cam = mChaseCameras[i].get();
                        if (!cam) continue;
                        Vector3 camPos = cam->Transform().Position();
                        float distance = (camPos - boatPos).Length();
                        if (distance < bestDistance) {
                            bestDistance = distance;
                            bestIndex = static_cast<int>(i);
                        }
                    }
                    if (bestIndex != -1) {
                        mActiveCameraIndex = bestIndex;
                    }
                }
            }

            ImGui::Text("HP: %.1f", mSelectedUIBoat->GetHP());

            // Allow modifying HP.
            static float setHP = mSelectedUIBoat->GetHP();
            ImGui::InputFloat("Set HP", &setHP);
            if (ImGui::Button("Apply HP")) {
                setHP = std::max(0.0f, setHP); // Prevent negative HP
                if (setHP < mSelectedUIBoat->GetHP()) {
                    mSelectedUIBoat->SetHP(setHP);
                }
            }

            ImGui::Text("Missiles: %d", mSelectedUIBoat->GetMissilesRemaining());

            // Allow adding or reducing missiles.
            static int setMissiles = mSelectedUIBoat->GetMissilesRemaining();
            ImGui::InputInt("Set Missiles", &setMissiles);
            if (ImGui::Button("Apply Missiles")) {
                setMissiles = std::max(0, setMissiles); // Prevent negative missiles
                mSelectedUIBoat->AddMissiles(setMissiles);
            }

            // Display the current team
            ImGui::Text("Team: %s", mSelectedUIBoat->GetTeamName());

            // Set up the combo box to allow changing the team.
            int teamIndex = static_cast<int>(mSelectedUIBoat->GetTeam());
            if (ImGui::Combo("Set Team", &teamIndex, teamNames, IM_ARRAYSIZE(teamNames))) {
                // Update the boat's team.
                mSelectedUIBoat->SetTeam(static_cast<Team>(teamIndex));
            }

            // Change Boat State buttons.
            ImGui::Text("Change State:");
            if (ImGui::Button("Stop Boat")) {
                gMessenger->DeliverMessage(SYSTEM_ID, mSelectedUIBoat->GetID(), MessageType::Stop);
            }
            ImGui::SameLine();
            if (ImGui::Button("Destroy Boat")) {
                gMessenger->DeliverMessage(SYSTEM_ID, mSelectedUIBoat->GetID(), MessageType::Die);
            }
            if (ImGui::Button("Patrol")) {
                gMessenger->DeliverMessage(SYSTEM_ID, mSelectedUIBoat->GetID(), MessageType::Start);
            }
            ImGui::SameLine();
            if (ImGui::Button("Evade")) {
                gMessenger->DeliverMessage(SYSTEM_ID, mSelectedUIBoat->GetID(), MessageType::Evade);
            }
            ImGui::SameLine();
            if (ImGui::Button("Inactive")) {
                gMessenger->DeliverMessage(SYSTEM_ID, mSelectedUIBoat->GetID(), MessageType::Stop);
            }
        }
    }

    // ===================== Configuration =====================
    if (ImGui::CollapsingHeader("Configuration")) {
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::TreeNode("Configuration Options")) {
            ImGui::CheckboxFlags("Enable Keyboard Navigation", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);
            ImGui::CheckboxFlags("Enable Gamepad Navigation", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad);
            ImGui::CheckboxFlags("Enable Mouse Position Setting", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NavEnableSetMousePos);
            ImGui::CheckboxFlags("Disable Mouse", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NoMouse);

            // Restore Mouse if Disabled
            if (io.ConfigFlags & ImGuiConfigFlags_NoMouse) {
                if (ImGui::Button("Restore Mouse")) {
                    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
                }
            }

            ImGui::TreePop();
        }

        // Backend Flags (Read-Only or Display Only)
        if (ImGui::TreeNode("Backend Flags")) {
            ImGuiBackendFlags backend_flags = io.BackendFlags; // Local copy to avoid modifying actual flags
            ImGui::CheckboxFlags("Has Gamepad", (unsigned int*)&backend_flags, ImGuiBackendFlags_HasGamepad);
            ImGui::CheckboxFlags("Has Mouse Cursors", (unsigned int*)&backend_flags, ImGuiBackendFlags_HasMouseCursors);
            ImGui::CheckboxFlags("Has Set Mouse Pos", (unsigned int*)&backend_flags, ImGuiBackendFlags_HasSetMousePos);
            ImGui::CheckboxFlags("Renderer Has Vtx Offset", (unsigned int*)&backend_flags, ImGuiBackendFlags_RendererHasVtxOffset);
            ImGui::TreePop();
        }

        // Style Editor
        if (ImGui::TreeNode("Style Editor")) {
            ImGui::ShowStyleEditor();
            ImGui::TreePop();
        }
    }

    // ===================== Debugging & Metrics =====================
    if (ImGui::CollapsingHeader("Debugging & Metrics")) {
        // Metrics Window
        static bool showMetricsWindow = false;
        if (ImGui::Checkbox("Show Metrics Window", &showMetricsWindow)) {
            if (showMetricsWindow) ImGui::ShowMetricsWindow(&showMetricsWindow);
        }

        // Style Editor Window
        static bool showStyleEditor = false;
        if (ImGui::Checkbox("Show Style Editor", &showStyleEditor)) {
            if (showStyleEditor) {
                ImGui::Begin("Style Editor", &showStyleEditor);
                ImGui::ShowStyleEditor();
                ImGui::End();
            }
        }
    }

    // End of the main Control Panel window
    ImGui::End();
}


//--------------------------------------------------------------------------------------
// Render From Camera
//--------------------------------------------------------------------------------------
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
// Update Chase Cameras
//--------------------------------------------------------------------------------------
void Scene::UpdateChaseCameras(float frameTime)
{
    size_t cameraIndex = 0; // To track chase cameras corresponding to boats
    std::vector<std::unique_ptr<Camera>> validCameras; // Store valid cameras

    // Associate each chase camera with its respective boat
    allBoats = gEntityManager->GetAllBoatEntities();

    for (Boat* boatPtr : allBoats)
    {
        if (!boatPtr || boatPtr->GetState() == "Destroyed") continue; // Skip destroyed boats

        // Ensure we have enough chase cameras for the boats
        if (cameraIndex >= mChaseCameras.size()) break;

        // Get the associated chase camera
        Camera* chaseCam = mChaseCameras[cameraIndex].get();
        if (!chaseCam)
        {
            ++cameraIndex;
            continue;
        }

        // Update valid cameras list
        validCameras.emplace_back(std::move(mChaseCameras[cameraIndex]));

        // Get boat's position and forward direction
        Vector3 boatPos = boatPtr->Transform().Position();
        Vector3 boatForward = boatPtr->Transform().ZAxis();

        // Desired camera position: behind the boat at a fixed distance and increased height
        Vector3 desiredPos = boatPos - boatForward * mChaseDistance + Vector3(0, mChaseHeight, 0);

        // Smoothly interpolate camera position for smooth following
        Vector3 currentPos = chaseCam->Transform().Position();
        float smoothSpeed = 5.0f;
        Vector3 newPos = Lerp(currentPos, desiredPos, smoothSpeed * frameTime);
        chaseCam->Transform().Position() = newPos;

        // Calculate yaw based on boat's forward direction
        float yaw = std::atan2(boatForward.x, boatForward.z);

        // Set a fixed downward pitch angle
        float pitch = mChasePitch;

        // Apply rotation to the chase camera
        chaseCam->Transform().SetRotation({ pitch, yaw, 0 });

        // Move to the next camera
        ++cameraIndex;
    }

    // Replace mChaseCameras with only valid cameras
    mChaseCameras = std::move(validCameras);

    // Ensure active camera index remains valid
    if (mActiveCameraIndex >= static_cast<int>(mChaseCameras.size()))
    {
        mActiveCameraIndex = static_cast<int>(mChaseCameras.size()) - 1;
    }

    if (mChaseCameras.empty())
    {
        mActiveCameraIndex = -1; // No available chase cameras
    }
}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------

// Update entire scene. frameTime is the time passed since the last frame
void Scene::Update(float frameTime)
{
    mRandomCrateTimer -= frameTime;
    mRandomMineTimer -= frameTime;

    if (mGamePaused) {
        return;
    }

    // Update all entities
    gEntityManager->UpdateAll(frameTime);

    allBoatIDS = gEntityManager->GetAllBoatIDS();

    // Handle key inputs for starting and stopping boats
    if (KeyHit(Key_1))
    {
        for (EntityID& boatID : allBoatIDS)
        {
            gMessenger->DeliverMessage(SYSTEM_ID, boatID, MessageType::Start);
        }
    }

    if (KeyHit(Key_2))
    {
        for (EntityID& boatID : allBoatIDS)
        {
            gMessenger->DeliverMessage(SYSTEM_ID, boatID, MessageType::Stop);
        }
    }

    if (KeyHit(Key_7))
    {
        // Move to the next camera index
        ++mActiveCameraIndex;
        if (mActiveCameraIndex >= static_cast<int>(mChaseCameras.size()))
        {
            // Wrap around
            mActiveCameraIndex = 0;
        }
    }

    if (KeyHit(Key_8))
    {
        // Move to the previous camera index
        --mActiveCameraIndex;
        if (mActiveCameraIndex < 0)
        {
            // Wrap to the last camera
            mActiveCameraIndex = static_cast<int>(mChaseCameras.size()) - 1;
        }
    }

    // Switch back to main camera
    if (KeyHit(Key_9))
    {
        mActiveCameraIndex = -1;
    }

    // Toggle extended boat UI
    if (KeyHit(Key_0))
    {
        mShowExtendedBoatUI = !mShowExtendedBoatUI;
    }

    // Control the main camera only if it's active
    if (mActiveCameraIndex == -1)
    {
        static float movementSpeed = 40.0f;
        const float rotationSpeed = 1.5f;   // Radians per second for rotation
        if (KeyHit(Key_F1))  movementSpeed = 40.0f;
        if (KeyHit(Key_F2))  movementSpeed = 200.0f;
        mCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D, movementSpeed, rotationSpeed);
    }

    // Handles mouse click interactions for selecting nearest boat
    if (KeyHit(Mouse_LButton))
    {
        if (mSelectedBoat) {
            Vector2i mousePos = { 0, 0 };
            mousePos = GetMousePosition(GetActiveWindow());

            Vector3 intersect = { 0.0f, 0.0f, 0.0f };
            if (mCamera->WorldPtFromPixel(mousePos.x, mousePos.y, gPerFrameConstants.viewportWidth, gPerFrameConstants.viewportHeight, intersect))
            {
                gMessenger->DeliverMessage(SYSTEM_ID, mSelectedBoat->GetID(), MessageType::TargetPoint, TargetPointData{ intersect, 5.0f });
            }
            mSelectedBoat = nullptr;
        }
        else if (mNearestEntity)
        {
            mSelectedBoat = mNearestEntity;
            gMessenger->DeliverMessage(SYSTEM_ID, mSelectedBoat->GetID(), MessageType::Evade);
        }
    }

    if (AreBoatsActive()) // Only spawn if boats are active
    {
        existingCrates = gEntityManager->GetAllCratesEntities();
        if (existingCrates.size() < mMaxCrates && mRandomCrateTimer <= 0.0f) {
            Vector3 spawnPos(Random(-250.0f, 250.0f), -10.0f, Random(-250.0f, 250.0f));
            Matrix4x4 transform(spawnPos, { 0, 0, 0 }, 1.0f);

            float r = Random(0.0f, 1.0f);
            CrateType type;
            if (r < 0.33f)
                type = CrateType::Missile;
            else if (r < 0.66f)
                type = CrateType::Health;
            else
                type = CrateType::Shield;

            gEntityManager->CreateEntity<RandomCrate>("RandomCrate", transform, type);

            // Reset the timer for the next crate spawn
            mRandomCrateTimer = Random(12.0f, 20.0f);
        }

        // Check and spawn mines only if the current count is below the limit
        existingMines = gEntityManager->GetAllMinesEntities();
        if (existingMines.size() < mMaxMines && mRandomMineTimer <= 0.0f) {
            Vector3 spawnPos(Random(-250.0f, 250.0f), -20.0f, Random(-250.0f, 250.0f));
            Matrix4x4 transform(spawnPos, { 0, 0, 0 }, 1.0f);

            gEntityManager->CreateEntity<SeaMine>("SeaMine", transform);

            // Reset the timer for the next mine spawn
            mRandomMineTimer = Random(12.0f, 15.0f);
        }
    }

    // Toggle FPS limiting
    if (KeyHit(Key_F))  mLockFPS = !mLockFPS;
    if (KeyHit(Key_P))  mGamePaused = !mGamePaused;

    // Update chase cameras to follow their boats
    UpdateChaseCameras(frameTime);
}


//--------------------------------------------------------------------------------------
// Draw Text at World Point
//--------------------------------------------------------------------------------------
// Draw given text at the given 3D point, also pass camera in use. Optionally centre align and colour the text
void Scene::DrawTextAtWorldPt(const Vector3& point, std::string text, Camera* camera, bool centreAlign)
{
    auto pixelPt = camera->PixelFromWorldPt(point, static_cast<float>(DX->GetBackbufferWidth()), static_cast<float>(DX->GetBackbufferHeight()));
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

//--------------------------------------------------------------------------------------
// Handle Mouse Picking
//--------------------------------------------------------------------------------------
// Performs mouse picking to find the nearest boat entity to the cursor.
void Scene::HandleMousePicking()
{
    Vector3f pixelPos3D;
    Vector2i pixelPos;
    Vector2i mousePos = { 0, 0 };
    float nearestDistance = 50.0f;
    mNearestEntity = nullptr; // Reset the nearest entity

    allBoats = gEntityManager->GetAllBoatEntities();

    for (Boat* boatPtr : allBoats)
    {
        if (!boatPtr) continue;

        // Use active camera for picking
        pixelPos3D = mCamera.get()->PixelFromWorldPt(boatPtr->Transform().Position(), gPerFrameConstants.viewportWidth, gPerFrameConstants.viewportHeight);
        pixelPos = Vector2i(static_cast<int>(pixelPos3D.x), static_cast<int>(pixelPos3D.y));
        mousePos = GetRawMouse();
        float distToBoat = (pixelPos - mousePos).Length();

        if (distToBoat < nearestDistance)
        {
            mNearestEntity = boatPtr;
            nearestDistance = distToBoat;
        }

    }
}

bool Scene::AreBoatsActive()
{
    std::vector<Boat*> boats = gEntityManager->GetAllBoatEntities();
    for (Boat* boat : boats)
    {
        if (boat && boat->GetState() != "Inactive")
        {
            return true;
        }
    }
    return false; // All boats are inactive
}
