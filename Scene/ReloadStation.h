#ifndef _RELOAD_STATION_H_INCLUDED_
#define _RELOAD_STATION_H_INCLUDED_

#include "Entity.h"

class ReloadStation : public Entity
{
public:
    // Constructor: The entity template, unique ID, initial transform, and optional name are passed in.
    ReloadStation(EntityTemplate& entityTemplate, EntityID ID, const Matrix4x4& transform = Matrix4x4::Identity, const std::string& name = "")
        : Entity(entityTemplate, ID, transform, name) {}
};

#endif //_RELOAD_STATION_H_INCLUDED_
