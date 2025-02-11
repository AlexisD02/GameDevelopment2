#include "Shield.h"
#include "SceneGlobals.h"
#include "MathHelpers.h"

// Shield entity rotates around the Y-axis and pulsates in scale, to give a visual effect.
bool Shield::Update(float frameTime)
{
    mElapsed += frameTime;

    // Retrieve the parent boat
    Boat* parentBoat = gEntityManager->GetEntity<Boat>(mParentBoatID);
    if (!parentBoat || parentBoat->GetState() == "Destroyed")
    {
        // Remove the shield if the parent boat is gone
        return false;
    }

    // Follow the boat's position
    Vector3 boatPos = parentBoat->Transform().Position();
    Vector3 shieldOffset(0.0f, 2.0f, 0.0f); // Slightly above the boat
    Transform().Position() = boatPos + shieldOffset;

    // Rotate the shield
    float rotationSpeed = 15.0f;
    Transform().RotateLocalY(ToRadians(rotationSpeed * frameTime));

    // Make the shield pulse
    float pulseFrequency = 0.5f;
    float pulseAmplitude = 0.05f;
    float scaleFactor = 1.0f + pulseAmplitude * std::sin(2.0f * std::numbers::pi_v<float> * pulseFrequency * mElapsed);
    Transform().SetScale(scaleFactor);

    // Decrement shield duration
    mShieldDuration -= frameTime;
    if (mShieldDuration <= 0.0f)
    {
        gMessenger->DeliverMessage(GetID(), mParentBoatID, MessageType::ShieldDestroyed);

        return false;
    }

    return true;
}
