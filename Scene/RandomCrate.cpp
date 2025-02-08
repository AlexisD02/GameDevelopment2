#include "RandomCrate.h"
#include "SceneGlobals.h"
#include "Boat.h"

bool RandomCrate::Update(float frameTime)
{
    mOscillationTime += frameTime;
    Vector3 pos = Transform().Position();

    pos.y = mBaseY + 1.0f * std::sin(mOscillationTime);

    // Update the crate's position.
    Transform().Position() = pos;
	Transform().RotateLocalY(0.75f * frameTime);

    std::vector<Boat*> boats = gEntityManager->GetAllBoatEntities();
    float collisionRadiusSq = collisionRadius * collisionRadius;

    for (Boat* boat : boats)
    {
        Vector3 diff = boat->Transform().Position() - pos;
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
