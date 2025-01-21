//--------------------------------------------------------------------------------------
// Entity and EntityTemplate base classes
//--------------------------------------------------------------------------------------
// An entity template is a blueprint for a type of entity. We can 'instantiate' entities into the scene
// using this template data. The templates contain the geometry used by the entity type as well as any
// shared data that all entities of that type will use (e.g. Max HP, top speed etc.) 
//
// An entity is a single instance of an entity template in the scene, holding positional information and
// any data specific to that instance (e.g. current HP, speed etc.)
// 
// The base EntityTemplate class only contains geometry with no other data. This is sufficient for static
// entity types or those that don't need any data at a template level. Inherited template classes are used
// for more complex types of entity that need more shared data
//
// The base Entity class can handle the rendering needs of most entity types. However, there is no entity data
// except the transformation matrices, and the Update function does the bare minimum (simple death check). So
// this class is only suitable for static scene objects. More complex entities will have other gameplay data
// and code so they will need new classes inherited from the base Entity
// 
// An EntityTemplate can return the collection of entities currently using it so the game code may not need
// to maintain the same data. E.g. if you need to do something with all tank entities, you could use the tank
// template's list of current tank entities rather than hold your own. However, the template holds pointers to
// the entity base class and you may need to cast to the actual type. If this is a problem then you might need
// to maintain your own collections elsewhere in the game code.


#ifndef _ENTITY_H_INCLUDED_
#define _ENTITY_H_INCLUDED_

#include "EntityTypes.h"

#include "Mesh.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "ColourTypes.h"

#include <vector>
#include <string>
#include <memory>

// Forward declaration of Mesh class allows us to use Mesh pointers before that class has been fully declared
// This reduces the number of include files needed here, which in turn minimises dependencies and speeds up compilation
//
// Note: if you have a class containing a unique_ptr to a forward declaration (that is the case here) then this class must also have a destructor
// signature here and declaration in the cpp file - this is required even if the destructor is default/empty. Otherwise you get compile errors.
class Mesh;


/*-----------------------------------------------------------------------------------------
   Entity Template class
-----------------------------------------------------------------------------------------*/
class EntityTemplate
{
	friend class EntityManager; // Manager is a friend class so it can update which entities use this template without needing to expose a public function to do that

	/*-----------------------------------------------------------------------------------------
	   Construction / Destruction
	-----------------------------------------------------------------------------------------*/
public:
	// Base entity template constructor needs template type (e.g. "Green GunBoat") and the associated mesh (e.g. "gunboat_green.fbx")
	// Can throw std::runtime_error if the mesh fails to load
	EntityTemplate(const std::string& type, const std::string& meshFilename, ImportFlags importFlags = {});

	// Destructor - polymorphic base class destructors should always be virtual, also see comment on forward declarations above
	virtual ~EntityTemplate();


	/*-----------------------------------------------------------------------------------------
	   Data Access
	-----------------------------------------------------------------------------------------*/
public:
	// The name of the type of entity this template is a blueprint for
	std::string GetType()
	{
		return mType;
	}

	// Returns reference to the mesh used by enities based on this template
	Mesh& GetMesh() // Would prefer the function to be just called "Mesh" as it returns a reference but that would clash with the class name 'Mesh'
	{
		return *mMesh;
	}

	// Return a vector of entity IDs using this template. The vector is const, you cannot change its contents, but you can change the entities themselves
	const std::vector<EntityID>& Entities()
	{
		return mEntities;
	}


	/*-----------------------------------------------------------------------------------------
	   Private data
	-----------------------------------------------------------------------------------------*/
private:
	// Type of the template
	std::string mType;

	// The mesh representing this entity
	std::unique_ptr<Mesh> mMesh;

	// Pointers to entities based on this template
	std::vector<EntityID> mEntities;
};




/*-----------------------------------------------------------------------------------------
   Entity Class
-----------------------------------------------------------------------------------------*/
class Entity
{
	/*-----------------------------------------------------------------------------------------
	   Construction
	-----------------------------------------------------------------------------------------*/
public:
	// Base entity constructor, needs pointer to common template data and unique ID (UID)
	// May also pass a name and initial transformation for root (defaults are empty named entity at origin)
    Entity(EntityTemplate& entityTemplate, EntityID UID, const Matrix4x4& transform = Matrix4x4::Identity, const std::string& name = "");

	// Destructor - polymorphic base class destructors should always be virtual
	virtual ~Entity();


	/*-----------------------------------------------------------------------------------------
	   Data Access
	-----------------------------------------------------------------------------------------*/
public:
	// Direct access to entity's template, can access base or inherited template classes:
	// 
	//   string templateType = entity->Template().GetType();                    // No template parameter - gets base class template
	//   float vehicleMaxSpeed = entity->Template<VehicleTemplate>().mMaxSpeed; // Template parameter <VehicleTemplate>, so returns VehicleTemplate
	template<typename T = EntityTemplate>
	T& Template() { return dynamic_cast<T&>(mTemplate); }

	// Entity identity getters
	EntityID           GetID()    { return mID; }
	const std::string& GetName()  { return mName; }

	// Direct access to the transformation matrix of the given node of the entity's mesh.
	// Root matrix (node=0) is in world space, all other nodes are parent-relative.
	// Matrix4x4 supports many functions for model manipulation - see Matrix4x4.h for details.
	// Examples: entity.Transform().Position() = {0,0,0};    entity.Transform(2).SetScale(10);    Vector3 facing = entity.Transform().ZAxis();
	//           entity.Transform().FaceTarget(enemyPosition);    Vector3 angles = entity.Transform(1).GetRotation();
	Matrix4x4& Transform(int node = 0)  { return mTransforms[node]; }

	// Calculate the absolute transformation matrix of a given node. All nodes except the root 0 store their transformations
	// relative to their parent. Use this method if you want the real world-space transformation of a node (not relative to parent)
	Matrix4x4 AbsoluteTransform(int node) { return mTemplate.GetMesh().AbsoluteMatrix(mTransforms, node); }


	// Direct access to the render group value for this entity. Value can be get or set. Default is 0
	// Each entity has a render group value and the groups of entities with the same value can be rendered seperately.
	// The meanings of the group values are for the caller to decide.
	// An example would be to give index 0 to entities rendered with no blending and give index 1 to entities 
	// rendered with additive blending. Then render group 0 and group 1 seperately, changing render states between
	unsigned int& RenderGroup()   { return mRenderGroup;  }

	// Direct access to the render colour for this entity. Value can be get or set.
	ColourRGBA& RenderColour()  { return mRenderColour; }


	/*-----------------------------------------------------------------------------------------
	   Update / Render
	-----------------------------------------------------------------------------------------*/
public:
	// Update the entity including message processing for each entity
	// Virtual function, inherited classes should override this with their own processing
	// ***Entity Update functions should return false if the entity is to be destroyed***
	virtual bool Update(float frameTime);

	// Render the entity's geometry
	void Render();


	/*-----------------------------------------------------------------------------------------
	   Private Data
	-----------------------------------------------------------------------------------------*/
private:
	// The template used by this entity - the common data for all entities of this type
	EntityTemplate& mTemplate;

	// Unique identifier for the entity
	EntityID mID;

	// Name for the entity, can be empty "" and does not need to be unique
	std::string mName;

	// Transformation matrices that position this entity. mTransforms[0] is the world matrix for the entire model. The remaining transforms position sub-parts of
	// the model with each matrix relative to the parent part (recall animation material in 2nd year Graphics). The hierarchy tree is defined in the entity template -> mesh
	std::vector<Matrix4x4> mTransforms;

	// Each entity has a render group value and the groups of entities with the same value can be rendered seperately.
	unsigned int mRenderGroup = 0;

	// Each entity can have a custom colour
	ColourRGBA mRenderColour = { 1, 1, 1, 1 };
};


#endif //_ENTITY_H_INCLUDED_
