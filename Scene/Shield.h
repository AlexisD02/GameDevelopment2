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
    Shield(EntityTemplate& entityTemplate, EntityID id, const Matrix4x4& transform,
        float duration = 7.0f, const std::string& name = "")
        : Entity(entityTemplate, id, transform, name),
        mDuration(duration),
        mElapsed(0.0f)
    {
    }

    // Update function
    virtual bool Update(float frameTime) override;

    // Accessor for duration if needed.
    float GetDuration() const { return mDuration; }

private:
    float mDuration; // The duration the shield effect lasts
    float mElapsed;  // Time elapsed since the shield was spawned
};

#endif // _SHIELD_H_INCLUDED_
