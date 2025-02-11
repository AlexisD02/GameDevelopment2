#include "RandomCrate.h"
#include "SceneGlobals.h"
#include "Boat.h"

bool RandomCrate::Update(float frameTime)
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
        Vector3 cratePos = Transform().Position();
        cratePos.y = mBaseY + 0.7f * std::sin(mOscillationTime);
        Transform().Position() = cratePos;
    }

	Transform().RotateLocalY(0.75f * frameTime);

    std::vector<Boat*> boats = gEntityManager->GetAllBoatEntities();
    float collisionRadiusSq = collisionRadius * collisionRadius;
    Vector3 cratePos = Transform().Position();

    for (Boat* boat : boats)
    {
        Vector3 diff = boat->Transform().Position() - cratePos;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSq <= collisionRadiusSq)
        {
            // Prepare crate pickup message data.
            CratePickupData data;
            data.type = mCrateType;

            // Send the crate pickup message to the boat.
            // The message could carry a variant or a struct. Adjust to your messenger design.
            gMessenger->DeliverMessage(GetID(), boat->GetID(), MessageType::CrateCollected, data);

            // Return false to destroy the crate.
            return false;
        }
    }

    return true;
}
