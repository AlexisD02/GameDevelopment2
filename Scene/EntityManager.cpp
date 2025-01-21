//--------------------------------------------------------------------------------------
// EntityManager class encapsulates creation, update and rendering of entities
//--------------------------------------------------------------------------------------
// The Entity manager can create and destroy entities and entity templates
// Each entity template is identified by a type (string), and existing templates can be
// searched for by type. Each new entity is given a unique ID and existing entities can be
// searched for by ID. The manager can also run the Update and Render functions of all entities
// Certain rendering settings can be applied at a per-entity level, e.g. transparency, or tint colour

#include "EntityManager.h"


//--------------------------------------------------------------------------------------
// Entity / Template Destruction
//--------------------------------------------------------------------------------------

// Destroy the given entity template, also destroying all the entities that are based on it
// Entity templates can use a lot of memory (meshes and textures) so they should be released when possible, but watch out
// for the implications of all their entities being destroyed
// Returns true on success, false if there is no entity template with the given type
bool EntityManager::DestroyEntityTemplate(std::string type)
{
	// Check that requested entity template exists
	if (!mEntityTemplates.contains(type))  return false;

	// Destroy all the entities referring to this template
	auto entityTemplate = mEntityTemplates[type].get();
	while (entityTemplate->mEntities.size() > 0) // Can't use a for loop as we are destroying the container we are looping through, instead repeatedly remove last item
	{
		DestroyEntity(entityTemplate->mEntities.back());
		entityTemplate->mEntities.pop_back();
	}

	// Destroy the template
	mEntityTemplates.erase(type);
	return true;
}

// Destroy the entity with the given ID. Returns true on success, false if there isn't an entity with this ID
bool EntityManager::DestroyEntity(EntityID id)
{
	// Check that requested entity exists
	if (!mEntities.contains(id))  return false;

	// Remove entity from template collection of entities *UPDATE*
	auto& templateEntities = GetEntity(id)->Template().mEntities;
	auto removeEnd = std::remove(templateEntities.begin(), templateEntities.end(), id);
	templateEntities.erase(removeEnd, templateEntities.end());

	mEntities.erase(id);
	return true;
}


//--------------------------------------------------------------------------------------
// Entity Usage
//--------------------------------------------------------------------------------------

// Render all entities in a particular render group (see render group comments in Entity.h)
void EntityManager::RenderGroup(unsigned int group)
{
	for (auto& [ID, entity] : mEntities)
	{
		if (entity->RenderGroup() == group)  entity->Render();
	}
}


// Render all entities regardless of group
void EntityManager::RenderAll()
{
	for (auto& [ID, entity] : mEntities)  entity->Render();
}


// Call all current entity's Update functions. Any entity that returns false will be destroyed
void EntityManager::UpdateAll(float frameTime)
{
	// Loop needs to be performed carefully here since we can erase entities as we progress - so note how the ++it is not in the if statement
	for (auto it = mEntities.begin(); it != mEntities.end();)
	{
		auto entity = it->second.get();
		++it; // Do this before potential entity destruction on next line, which would invalidate the current it value

		// If entity update returns false the entity is destroyed *UPDATE*
		if (!entity->Update(frameTime))  DestroyEntity(entity->GetID());
			
	}
}

