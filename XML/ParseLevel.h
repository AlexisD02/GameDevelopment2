#ifndef _PARSE_LEVEL_H_INCLUDED_
#define _PARSE_LEVEL_H_INCLUDED_

#include "tinyxml2.h"      // TinyXML2 parser
#include "Vector3.h"       // For Vector3
#include "Matrix4x4.h"     // For Matrix4x4

#include "SceneGlobals.h"  // For ImportFlags, Team, etc.
#include "MathHelpers.h"   // For ToRadians

#include "EntityManager.h" // For creating templates and entities
#include "Boat.h"          // For Boat and BoatTemplate types
#include "ReloadStation.h" // For ReloadStation type
#include "Obstacle.h"      // For Obstacle type
#include "Entity.h"        // For generic Entity

#include <string>
#include <vector>
using std::string;
using std::vector;

/*---------------------------------------------------------------------------------------------
    ParseLevel class
    Reads and sets up a level by parsing an XML file.
---------------------------------------------------------------------------------------------*/
class ParseLevel
{

    /*---------------------------------------------------------------------------------------------
        Construction
    ---------------------------------------------------------------------------------------------*/
public:
    // Constructor just stores a pointer to the entity manager so all methods below can access it
    ParseLevel(EntityManager& entityManager) : mEntityManager(&entityManager) 
    {}

    /*-----------------------------------------------------------------------------------------
        Usage
    -----------------------------------------------------------------------------------------*/
public:
    bool ParseFile(const string& fileName);

    /*-----------------------------------------------------------------------------------------
        Private Helpers
    -----------------------------------------------------------------------------------------*/
private:
    bool ParseSceneElement(tinyxml2::XMLElement* rootElement);
    bool ParseEntityTemplates(tinyxml2::XMLElement* templatesElem);
    bool ParseEntitiesElement(tinyxml2::XMLElement* entitiesElem);

    Vector3 GetVector3FromElement(tinyxml2::XMLElement* rootElement);

    /*---------------------------------------------------------------------------------------------
        Private Data
    ---------------------------------------------------------------------------------------------*/

    // Constructer is passed a pointer to an entity manager used to create templates and
    // entities as they are parsed
    EntityManager* mEntityManager;
};

#endif // _PARSE_LEVEL_H_INCLUDED_
