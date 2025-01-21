//--------------------------------------------------------------------------------------
// EntityManager class encapsulates creation, update and rendering of entities
//--------------------------------------------------------------------------------------
// The Entity manager can create and destroy entities and entity templates
// Each entity template is identified by a type (string), and existing templates can be
// searched for by type. Each new entity is given a unique ID and existing entities can be
// searched for by ID. The manager can also run the Update and Render functions of all entities
// Certain rendering settings can be applied at a per-entity level, e.g. transparency, or tint colour

#ifndef _ENTITY_MANAGER_H_INCLUDED_
#define _ENTITY_MANAGER_H_INCLUDED_

#include "Entity.h"

#include <string>
#include <map>
#include <stdexcept>
#include <stdint.h>


//--------------------------------------------------------------------------------------
// Entity Manager Class
//--------------------------------------------------------------------------------------
class EntityManager
{
	//--------------------------------------------------------------------------------------
	// Entity / Template Creation
	//--------------------------------------------------------------------------------------
public:
	// Create any type of entity template that inherits from EntityTemplate
	// Returns nullptr if entity template creation fails. Call GetLastError for a text description of the error
	// There is no need to delete the returned pointer as this manager handles object lifetimes. Indeed, you can even ignore the returned
	// pointer, because entity creation is done by template name, not pointer
	// 
	// All entity templates must have a constructor whose first two parameters are type and meshFilename. However, they may
	// also have further parameters. When calling this function, after passing the type & meshFilename, also pass any other
	// constructor parameters neededby the template type you are creating. See BoatTemplate constructor in Boat for an example.
	// It is a good practice to include an importFlags parameter in new templates so the mesh import of your template can be
	// tweaked, but it is not necessary. 
	// 
	//   E.g. EntityTemplate* TreeType = CreateEntityTemplate<EntityTemplate>("Tree", "tree.fbx");              // Create base class entity template with minimal settings
	//        TankTemplate*   TankType = CreateEntityTemplate<TankTemplate>  ("Tank", "tank.fbx", 100, 10); // Create inherited tank entity template with additional constructor parameters
	//
	// The extra parameters are forwarded to the constructor by using a parameter pack (...) and std::forward. The syntax is ugly, but is powerful
	// and worth knowing about. Look in particular how ConstructorTypes and constructorValues are used with ... to see what is going on here
	template <typename EntityTemplateType, typename ...ConstructorTypes>
	EntityTemplateType* CreateEntityTemplate(std::string type, std::string meshFilename, ConstructorTypes&&... constructorValues)
	{
		// See if template with the same name already exists, if so delete the existing one
		if (mEntityTemplates.contains(type))  mEntityTemplates.erase(type);

		// Try to construct new entity template
		EntityTemplateType* entityTemplate;
		try
		{
			entityTemplate = new EntityTemplateType(type, meshFilename, std::forward<ConstructorTypes>(constructorValues)...);
		}
		catch (std::runtime_error e)
		{
			mLastError = e.what(); // This picks up the error message put in the exception
			return nullptr;
		}

		// Add new entity template to map (map.emplace is needed here instead of map.insert because the map contains a unique_ptr, same effect though)
		mEntityTemplates.emplace(type, entityTemplate);

		return entityTemplate;
	}


	// Create any type of entity that inherits from Entity
	// Returns NO_ID if the template type does not exist or if entity creation fails. Call GetLastError for a text description of the error
	// 
	// All entities must have a constructor whose first two parameters are an EntityTemplate& then an EntityID. However, they may
	// also have further parameters. When calling this function you first pass a template name (no need to pass the ID, the manager will create that)
	// Then pass any other parameters the entity's constructor expects after the template and ID
	// Note that even the base class entity type has further constructor parameters aside from the template name:
	//   E.g. Entity* Tree1 = CreateEntity<Entity>("Tree");                                   // Create base class entity with default settings
	//        Entity* Tree2 = CreateEntity<Entity>("Tree", Matrix4x4{(10, 0, 20}), "Bob");    // Create base class entity with initial position and name
	//        Tank*   Tank1 = CreateEntity<Tank>  ("Tank", 100, Matrix4x4{10, 0, 20}, "Bob"); // Create inherited Tank entity with initial HP, position and name
	//
	// The extra parameters are forwarded to the constructor by using a parameter pack (...) and std::forward. The syntax is ugly, but is powerful
	// and worth knowing about. Look in particular how ConstructorTypes and constructorValues are used with ... to see what is going on here
	template <typename EntityType, typename ...ConstructorTypes>
	EntityID CreateEntity(std::string templateType, ConstructorTypes&&... constructorValues)
	{
		// Check that requested entity template exists
		if (!mEntityTemplates.contains(templateType))
		{
			mLastError = "Entity Manager: Cannot find entity template '" + templateType + "'";
			return NO_ID;
		}
		
		// Get template and ID for new entity
		EntityTemplate* entityTemplate = mEntityTemplates[templateType].get();
		EntityID newID = mNextID++;

		// Try to construct new entity
		EntityType* entity;
		try
		{
			entity = new EntityType(*entityTemplate, newID, std::forward<ConstructorTypes>(constructorValues)...);
		}
		catch (std::runtime_error e)
		{
			mLastError = e.what(); // This picks up the error message put in the exception
			return NO_ID;
		}

		// Add new entity to map (map.emplace is needed here instead of map.insert because the map contains a unique_ptr, same effect though)
		mEntities.emplace(newID, entity);

		// Tell template about this new entity that is using it
		entityTemplate->mEntities.push_back(newID);

		return newID;
	}


	//--------------------------------------------------------------------------------------
	// Entity / Template Deletion
	//--------------------------------------------------------------------------------------
public:
	// Destroy the given entity template, also destroying all the entities that are based on it
	// Entity templates can use a lot of memory (meshes and textures) so they should be released when possible, but watch out
	// for the implications of all their entities being destroyed
	// Returns true on success, false if there is no entity template with the given type
	bool DestroyEntityTemplate(std::string type);

	// Destroy the entity with the given ID. Returns true on success, false if there isn't an entity with this ID
	bool DestroyEntity(EntityID id);


	//--------------------------------------------------------------------------------------
	// Access
	//--------------------------------------------------------------------------------------
public:
	// Returns the entity template with the given type name. Returns as a base class EntityTemplate pointer by default but if you
	// know the inherited type you can specify it to get a pointer to the inherited type
	//   E.g. EntityTemplate* baseTemplate   = myEntityManager->GetTemplate("Basic Wizard");
	//   Or:  WizardTemplate* wizardTemplate = myEntityManager->GetTemplate<WizardTemplate>("Basic Wizard");
	// Returns nullptr if no template of the given name exists or if you use an invalid template type (e.g. if you ask for a CarTemplate for "Basic Wizard")
	template <typename T = EntityTemplate>
	T* GetTemplate(std::string type)
	{
		if (!mEntityTemplates.contains(type))  return nullptr;
		
		T* entityTemplate;
		try
		{
			entityTemplate = dynamic_cast<T*>(mEntityTemplates[type].get());
		}
		catch (std::bad_cast e)
		{
			return nullptr;
		}
		
		return entityTemplate;
	}

	// Returns the entity with the given name. Returns as a base class Entity pointer by default but if you know the inherited type
	// you can specify it to get a pointer to the inherited entity type
	//   E.g. Entity* tankBase = myEntityManager->GetEntity("MyTank");
	//   Or:  Tank*   tank     = myEntityManager->GetEntity<Tank>("MyTank");
	// Returns nullptr if no entity with the given ID exists or if you use an invalid entity type (e.g. if you ask for a Wizard with "MyTank")
	template <typename T = Entity>
	T* GetEntity(std::string name)
	{
		// Linear search for name - not efficient, but this function is useful to make simpler examples of entity code
		for (auto& entity : mEntities)
		{
			if (entity.second->GetName() == name)  return dynamic_cast<T*>(entity.second.get());
		}
		return nullptr;
	}
		
		
	// Returns the entity with the given ID. Returns as a base class Entity pointer by default but if you know the inherited type
	// you can specify it to get a pointer to the inherited entity type
	//   E.g. Entity* tankBase = myEntityManager->GetEntity(tankID);
	//   Or:  Tank*   tank     = myEntityManager->GetEntity<Tank>(tankID);
	// Returns nullptr if no entity with the given ID exists or if you use an invalid entity type (e.g. if you ask for a Wizard with tankID)
	template <typename T = Entity>
	T* GetEntity(EntityID id)
	{
		if (!mEntities.contains(id))  return nullptr;

		T* entity;
		try
		{
			entity = dynamic_cast<T*>(mEntities[id].get());
		}
		catch (std::bad_cast e)
		{
			return nullptr;
		}

		return entity;
	}


	//--------------------------------------------------------------------------------------
	// Entity Usage
	//--------------------------------------------------------------------------------------
public:
	// Render all entities in a particular render group (see render group comments in Entity.h)
	void RenderGroup(unsigned int group);

	// Render all entities regardless of group
	void RenderAll();
	
	// Call all current entity's Update functions. Any entity that returns false will be destroyed
	void UpdateAll(float frameTime);


	// If the CreateEntityTemplate or CreateEntity functions return nullptr to indicate an error, the text description
	// of the (most recent) error can be fetched with this function
	std::string GetLastError() { return mLastError; }

	// Clear any error - useful to do before a block of Create operations, then check last error only once at the end
	void ClearLastError() { mLastError = ""; }


	//--------------------------------------------------------------------------------------
	// Private Data
	//--------------------------------------------------------------------------------------
private:
	// Entity templates are ordered and searched for by name
	std::map<std::string, std::unique_ptr<EntityTemplate>> mEntityTemplates;

	// Entities are ordered and searched for by ID (not by name for performance reasons)
	std::map<EntityID, std::unique_ptr<Entity>> mEntities;

	// The next entity will get this ID, incremented on each creation
	EntityID mNextID = FIRST_ENTITY_ID;

	// Description of the most recent error from CreateEntityTemplate or CreateEntity
	std::string mLastError;
};


#endif // _ENTITY_MANAGER_H_INCLUDED_
