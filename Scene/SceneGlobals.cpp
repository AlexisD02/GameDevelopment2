//--------------------------------------------------------------------------------------
// Scene related global variables
//--------------------------------------------------------------------------------------
// This app is mostly well-encapsulated, but there are a few globals in use to simplify the code architecture
// 
// The gEntityManager object is global mainly so any entity class can access it to look up other entities by ID
// 
// The gMessenger object is global so any entity class can send messages to other entities
//
// The gScene object is global so entity classes can access overall game data - in this case the Scene object maintains the overall list of cars in the game
// 
// It would be possible to deal with these as "singleton" classes. These are classes that only ever have a single object.
// It might make the code a tiny bit cleaner the benefit is marginal - singletons have the same downsides as globals

#include "SceneGlobals.h"

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

// Global entity manager
std::unique_ptr<EntityManager> gEntityManager;

// Global messenger
std::unique_ptr <Messenger> gMessenger;

// Overall game scene
std::unique_ptr<Scene> gScene;
