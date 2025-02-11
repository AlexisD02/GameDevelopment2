#ifndef _SHIELD_H_INCLUDED_
#define _SHIELD_H_INCLUDED_

#include "Entity.h"      // Base class for entities
#include "EntityTypes.h" // For EntityID, etc.
#include "Vector3.h"     // For Vector3 type
#include "Matrix4x4.h"   // For Matrix4x4 type
#include <string>

class Shield : public Entity
{
public:
    // Constructor: The entity template, unique ID, initial transform, duration of the shield, and optional name are passed in.
    Shield(EntityTemplate& entityTemplate, EntityID id, const Matrix4x4& transform, EntityID parentBoatID)
        : Entity(entityTemplate, id, transform),
        mParentBoatID(parentBoatID), mElapsed(0.0f), mShieldDuration(7.0f)
    {}

    // Update function
    virtual bool Update(float frameTime) override;

private:
    EntityID mParentBoatID; // The ID of the boat this shield is attached to
    float mElapsed;  // Time elapsed since the shield was spawned
    float mShieldDuration;  // Duration before shield disappears
};

#endif // _SHIELD_H_INCLUDED_
