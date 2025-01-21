//--------------------------------------------------------------------------------------
// Entity class, base class for all scene elements
//--------------------------------------------------------------------------------------
// The Entity base class holds a pointer to its template class, ID and name, and the current transforms of its geometry
// 
// This base Entity class can handle the rendering needs of most entity types. However, there is no entity data
// here except the transformation matrices, and the Update function does the bare minimum (simple death check).
// So this class alone is only suitable for static scene objects. More complex entities will have other gameplay
// data and code either in inherited classes

#include "Entity.h"
#include "Mesh.h"
#include "SceneGlobals.h" // For entity manager and messenger


/*-----------------------------------------------------------------------------------------
	Entity Template and Entity Construction
-----------------------------------------------------------------------------------------*/

// Base entity template constructor needs template type (e.g. "Green GunBoat") and the associated mesh (e.g. "gunboat_green.fbx")
// Can throw std::runtime_error if the mesh fails to load
EntityTemplate::EntityTemplate(const std::string& type, const std::string& meshFilename, ImportFlags importFlags /* = {}*/)
	: mType(type)
{
	mMesh = std::make_unique<Mesh>(meshFilename, importFlags);
}

// Destructor - nothing to do, only required because polymorphic base classes must always have one. Also see comment on forward declarations in header file
EntityTemplate::~EntityTemplate() {}


// Entity constructor, needs pointer to common template data and ID, may also pass 
// May also pass a name and initial transformation for root (defaults are empty named entity at origin)
Entity::Entity(EntityTemplate& entityTemplate, EntityID ID, const Matrix4x4& transform /*= Matrix4x4::Identity*/, const std::string& name /*= ""*/)
    : mTemplate(entityTemplate), mID(ID), mTransforms(mTemplate.GetMesh().NodeCount()), mName(name)
{
	// Set initial matrices from mesh defaults
	for (int i = 0; i < mTransforms.size(); ++i)
		mTransforms[i] = mTemplate.GetMesh().DefaultTransform(i);

	// Override default root matrix (which should be the identity matrix) with constructor parameters
	mTransforms[0] = transform;
}

// Destructor - nothing to do, only required because polymorphic base classes must always have one
Entity::~Entity() {}



/*-----------------------------------------------------------------------------------------
	Entity Update / Rendering
-----------------------------------------------------------------------------------------*/

// Update the entity including message processing for each entity
// ***Entity Update functions should return false if the entity is to be destroyed***
bool Entity::Update([[maybe_unused]] float frameTime)
{
	// Could add common processing that applies to *all* entities here, but typically there is nothing common for everything so this method does nothing
	return true;
}


// Render the entity's geometry
void Entity::Render()
{
	mTemplate.GetMesh().Render(mTransforms, mRenderColour);
}
