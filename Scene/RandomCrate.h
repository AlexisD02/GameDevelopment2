#ifndef _RANDOM_CRATE_H_INCLUDED_
#define _RANDOM_CRATE_H_INCLUDED_

#include "Entity.h"
#include "EntityTypes.h"
#include "Messenger.h"
#include "Vector3.h"
#include <string>

class RandomCrate : public Entity
{
public:
    // Constructor: The entity template, unique ID, initial transform, and optional name are passed in.
    RandomCrate(EntityTemplate& entityTemplate, EntityID id, const Matrix4x4& transform, CrateType crateType)
        : Entity(entityTemplate, id, transform)
    {
        mCrateType = crateType;
    }

    // The Update function is called every frame.
    virtual bool Update(float frameTime) override;

    CrateType GetCrateType() const { return mCrateType; }

public:
    float collisionRadius = 15.0f;

private:
    CrateType mCrateType;
    float mOscillationTime = 0.0f;
    float mBaseY = -0.4f;
};

#endif // _RANDOM_CRATE_H_INCLUDED_
