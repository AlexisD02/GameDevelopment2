// A XML parser to read and setup a level - uses TinyXML2 to parse the file into its own structure, the 
// methods in this class traverse that structure and create entities and templates as appropriate

#include "ParseLevel.h"
#include "tinyxml2.h"
#include <cstdlib>
#include <cstring>

// This kind of statement would be bad practice in a include file, but here in a cpp it is a reasonable convenience
// since this file is dedicated to this namespace (and the exposed names won't leak into other parts of the program)
using namespace tinyxml2;

// Parse the entire level file and create all the templates and entities inside
bool ParseLevel::ParseFile(const string& fileName)
{
    // The tinyXML object XMLDocument will hold the parsed structure and data from the XML file
    // NOTE: even though there is a "using namespace tinyxml2;" at the top of the file, you still need to
    // give this declaration a tinyxml2:: because the Windows header files have also declared XMLDocument
    // which is not in a namespace so it collides with the TinyXML2 definition. An excellent example about
    // why namespaces are important (and this occurs more than once in Windows header files)
    tinyxml2::XMLDocument xmlDoc;

    // Open and parse document into tinyxml2 object xmlDoc
    XMLError error = xmlDoc.LoadFile(fileName.c_str());
    if (error != XML_SUCCESS) return false;

    // Note: I am ignoring XML nodes here and traversing the XML elements directly. Elements are a kind of node, they are
    // the tags but other kinds of node are comments, embedded documents or free text (not in a tag). I am ignoring those

    // No XML element in the level file means malformed XML or not an XML document at all
    XMLElement* element = xmlDoc.FirstChildElement();
    if (element == nullptr)  return false;

    while (element != nullptr)
    {
        // We found a "Level" tag at the root level, parse it
        string elementName = element->Name();
        if (elementName == "Scene")
        {
            ParseSceneElement(element);
        }

        element = element->NextSiblingElement();
    }

    return true;
}

// Parse a "Level" tag within the level XML file
bool ParseLevel::ParseSceneElement(XMLElement* rootElement)
{
    XMLElement* element = rootElement->FirstChildElement();
    while (element != nullptr)
    {
        // Things expected in a "Level" tag
        string elementName = element->Name();
        if      (elementName == "EntityTemplates")  ParseEntityTemplates(element);
        else if (elementName == "Entities")         ParseEntitiesElement(element);

        element = element->NextSiblingElement();
    }

    return true;
}

//------------------------------------------------------------------------------
// Parse the <EntityTemplates> element.
bool ParseLevel::ParseEntityTemplates(XMLElement* templatesElem)
{
    XMLElement* templateElem = templatesElem->FirstChildElement("EntityTemplate");
    while (templateElem != nullptr)
    {
        // Read the type, name and mesh attributes - these are all required, so fail on error
        const XMLAttribute* attr = templateElem->FindAttribute("Type");
        if (attr == nullptr)  return false;
        string type = attr->Value();

        attr = templateElem->FindAttribute("Name");
        if (attr == nullptr)  return false;
        string name = attr->Value();

        attr = templateElem->FindAttribute("Mesh");
        if (attr == nullptr)  return false;
        string mesh = attr->Value();

        if (type == "EntityTemplate")
        {
            // Check for an optional import flags attribute.
            attr = templateElem->FindAttribute("ImportFlags");
            if (attr && std::string(attr->Value()) == "NoLighting") {
                mEntityManager->CreateEntityTemplate<EntityTemplate>(name, mesh, ImportFlags::NoLighting);
            }
            else {
                mEntityManager->CreateEntityTemplate<EntityTemplate>(name, mesh);
            }
        }
        else if (type == "BoatTemplate")
        {
            attr = templateElem->FindAttribute("MaxSpeed");
            if (attr == nullptr)  return false;
            float maxSpeed = attr->FloatValue();

            attr = templateElem->FindAttribute("Acceleration");
            if (attr == nullptr)  return false;
            float acceleration = attr->FloatValue();

            attr = templateElem->FindAttribute("TurnSpeed");
            if (attr == nullptr)  return false;
            float turnSpeed = attr->FloatValue();

            attr = templateElem->FindAttribute("GunTurnSpeed");
            if (attr == nullptr)  return false;
            float gunTurnSpeed = attr->FloatValue();

            attr = templateElem->FindAttribute("MaxHP");
            if (attr == nullptr)  return false;
            float maxHP = attr->FloatValue();

            attr = templateElem->FindAttribute("MissileDamage");
            if (attr == nullptr)  return false;
            float missileDamage = attr->FloatValue();

            attr = templateElem->FindAttribute("Team");
            if (attr == nullptr)  return false;
            std::string teamStr = attr->Value();
            Team teamEnum = (teamStr == "TeamA") ? Team::TeamA : Team::TeamB;

            mEntityManager->CreateEntityTemplate<BoatTemplate>(name, mesh, maxSpeed, acceleration, turnSpeed, gunTurnSpeed, maxHP, missileDamage, teamEnum);
        }
        // You can add other template types here as needed.

        templateElem = templateElem->NextSiblingElement("EntityTemplate");
    }
    return true;
}

//------------------------------------------------------------------------------
// Parse a list of entity tags, creating each entity when enough data has been read.
// Ordinary entities (those not inside a <Team> tag) are parsed in this pass.
bool ParseLevel::ParseEntitiesElement(XMLElement* entitiesElem)
{
    //--------------------
    // Ordinary entities

    // First, iterate over all <Entity> elements.
    XMLElement* element = entitiesElem->FirstChildElement("Entity");
    while (element != nullptr)
    {
        // Read the required attributes: "Type" and "Template"
        const XMLAttribute* attr = element->FindAttribute("Type");
        if (attr == nullptr)  return false;
        std::string entityType = attr->Value();

        attr = element->FindAttribute("Template");
        if (attr == nullptr)  return false;
        std::string templateName = attr->Value();

        // (Optional: if you want to use the entity name, you can also read it.)
        std::string entityName;
        attr = element->FindAttribute("Name");
        if (attr != nullptr)
            entityName = attr->Value();

        // Set default transform values.
        Vector3 pos(0, 0, 0);
        Vector3 rot(0, 0, 0);
        float scale = 1.0f;

        // Look for a <Transform> element.
        XMLElement* transformElem = element->FirstChildElement("Transform");
        if (transformElem)
        {
            // Read the Position element.
            XMLElement* child = transformElem->FirstChildElement("Position");
            if (child != nullptr)
                pos = GetVector3FromElement(child);

            // Read the Rotation element.
            child = transformElem->FirstChildElement("Rotation");
            if (child != nullptr)
            {
                rot = GetVector3FromElement(child);
                // Convert rotation from degrees to radians.
                rot.x = ToRadians(rot.x);
                rot.y = ToRadians(rot.y);
                rot.z = ToRadians(rot.z);
            }

            // Read the Scale element.
            child = transformElem->FirstChildElement("Scale");
            if (child != nullptr)
            {
                attr = child->FindAttribute("Value");
                if (attr == nullptr)
                    return false;
                scale = attr->FloatValue();
            }
        }

        // Build the transform matrix regardless.
        Matrix4x4 transform(pos, rot, scale);

        // Depending on the entity type, create the appropriate entity.
        if (entityType == "Boat")
        {
            // For Boat entities, also read a <Speed> element.
            float speed = 0.0f;
            XMLElement* speedElem = element->FirstChildElement("Speed");
            if (speedElem != nullptr)
                speed = speedElem->FloatAttribute("Value", 0.0f);
            mEntityManager->CreateEntity<Boat>(templateName, speed, transform, entityName);
        }

        else if (entityType == "Obstacle")
        {
            mEntityManager->CreateEntity<Obstacle>(templateName, transform, entityName);
        }
        else if (entityType == "Obstacle")
        {
            Vector3 halfExtents(10.0f, 20.0f, 10.0f);

            XMLElement* collisionElem = element->FirstChildElement("Collision");
            if (collisionElem)
            {
                // Look for a <HalfExtents> element
                XMLElement* heElem = collisionElem->FirstChildElement("HalfExtents");
                if (heElem)
                {
                    halfExtents = GetVector3FromElement(heElem);
                }
            }
            // Create the obstacle with the parsed transform and half-extents.
            mEntityManager->CreateEntity<Obstacle>(templateName, transform, entityName, halfExtents);
        }

        else if (entityType == "ReloadStation")
        {
            mEntityManager->CreateEntity<ReloadStation>(templateName, transform, entityName);
        }
        else
        {
            mEntityManager->CreateEntity<Entity>(templateName, transform);
        }

        // Move to the next <Entity> element.
        element = element->NextSiblingElement("Entity");
    }

    return true;
}

//------------------------------------------------------------------------------
// Helper method: Read a Vector3 from an XML element.
// Expects the element to have uppercase attributes "X", "Y", and "Z".
// Also supports an optional child element "Randomise" to add a random offset.
Vector3 ParseLevel::GetVector3FromElement(XMLElement* element)
{
    Vector3 vec(0, 0, 0);
    element->QueryFloatAttribute("X", &vec.x);
    element->QueryFloatAttribute("Y", &vec.y);
    element->QueryFloatAttribute("Z", &vec.z);

    // Check for an optional "Randomise" child element.
    XMLElement* randomElem = element->FirstChildElement("Randomise");
    if (randomElem)
    {
        float randomVal = 0;
        const XMLAttribute* attr = randomElem->FindAttribute("X");
        if (attr != nullptr)
        {
            randomVal = attr->FloatValue() * 0.5f;
            vec.x += Random(-randomVal, randomVal);
        }
        attr = randomElem->FindAttribute("Y");
        if (attr != nullptr)
        {
            randomVal = attr->FloatValue() * 0.5f;
            vec.y += Random(-randomVal, randomVal);
        }
        attr = randomElem->FindAttribute("Z");
        if (attr != nullptr)
        {
            randomVal = attr->FloatValue() * 0.5f;
            vec.z += Random(-randomVal, randomVal);
        }
    }
    return vec;
}
