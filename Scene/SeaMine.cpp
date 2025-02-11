#include "SeaMine.h"
#include "SceneGlobals.h"
#include "Boat.h"
#include "Messenger.h"
#include "MathHelpers.h"

#include <vector>
#include <cmath>

bool SeaMine::Update(float frameTime)
{
    // Handle rising phase
    if (mRiseTimer < mRiseDuration)
    {
        mRiseTimer += frameTime;
        float progress = mRiseTimer / mRiseDuration;
        progress = std::min(progress, 1.0f); // Clamp to 1.0

        Vector3 minePos = Transform().Position();
        minePos.y = mStartY + (mBaseY - mStartY) * progress;
        Transform().Position() = minePos;
    }
    else // Start oscillating after rising
    {
        mOscillationTime += frameTime;
        Vector3 minePos = Transform().Position();
        minePos.y = mBaseY + 0.7f * std::sin(mOscillationTime);
        Transform().Position() = minePos;
    }

    // Rotate continuously
    Transform().RotateLocalY(0.35f * frameTime);

    // Check for nearby boats
    std::vector<Boat*> boats = gEntityManager->GetAllBoatEntities();
    Vector3 minePos = Transform().Position();

    for (Boat* boatPtr : boats)
    {
        Vector3 diff = boatPtr->Transform().Position() - minePos;
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distanceSq <= (mExplosionRadius * mExplosionRadius))
        {
            gMessenger->DeliverMessage(GetID(), boatPtr->GetID(), MessageType::MineHit);
            return false; // Destroy the mine
        }
    }

    return true; // Keep the mine alive
}