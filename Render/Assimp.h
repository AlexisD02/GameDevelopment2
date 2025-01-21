//--------------------------------------------------------------------------------------
// Assimp Importer Helpers
//--------------------------------------------------------------------------------------
// Structures and functions to simplify 3d mesh import using the assimp library

#ifndef _ASSIMP_H_INCLUDED_
#define _ASSIMP_H_INCLUDED_

#include <assimp/scene.h>

#include <map>
#include <vector>


//-------------------------------------------------------------------------------------------------
// Assimp Material Structures
//-------------------------------------------------------------------------------------------------
// Structures to hold assimp materials during mesh import. Temporary data, not used in final mesh.
// Only a few rendering approaches are supported so whilst most material properties are read from
// assimp, only a few are actually used. Typically looks for formated material/texture names to
// determine what render method to use for a material
// For files that do not have specially formatted material/texture names, creates a generic render
// method to suit the properties that are found (with defaults for missing properties)

// A single texture with blending information to previous texture / colour of same type
struct AssimpTexture
{
	aiString         filePath;
	unsigned int     uvIndex;
	float            blendFactor;
	aiTextureOp      blendOperation;
	aiTextureMapMode mapModes[2];
	int              textureFlags; // flags from aiTextureFlags
};


// Which Texture types to read from assimp. There are many more types but we cannot rely on assimp finding
// them as file formats vary greatly in what information they provide
const aiTextureType textureTypes[] = { aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR };


// A single material, with most of the information that assimp can extract
struct AssimpMaterial
{
	std::string name;

	aiShadingMode shadingMode;

	aiColor4D diffuseColour;
	aiColor3D specularColour;
	aiColor3D ambientColour;
	aiColor3D emissiveColour;
	aiColor3D transparentColour;
	aiColor4D pbrBaseColour;

	float specularPower;    // Phong exponent
	float specularStrength; // Specular multiplier

	float transparencyFactor;
	float refractiveIndex;
	float reflectivity;

	// Metallic/Roughness PBR route (assimp base colour is albedo)
	float pbrMetallicFactor;   //  0 -> 1
	float pbrRoughnessFactor;  //  0 -> 1
	float pbrAnisotropyFactor; // -1 -> 1

	// Specular/Glossiness PBR route (assimp diffuse colour is albedo)
	float pbrSpecularFactor;   // 0 -> 1
	float pbrGlossinessFactor; // 0 -> 1

	int         wireframe;
	int         twoSided;
	aiBlendMode blendFunction;

	std::map<aiTextureType, std::vector<AssimpTexture>> textures;
};

const std::string defaultMaterialName = "material"; // For materials without names, will have unique integer appended


//-------------------------------------------------------------------------------------------------
// Assimp Helper Functions
//-------------------------------------------------------------------------------------------------

// Read an open assimp scene structure and create structures for each contained material
void ReadMaterials(const aiScene& scene, std::vector<AssimpMaterial>& materials);

// Count the number of nodes with given assimp node as root - recursive
unsigned int CountDescendantsOf(aiNode* assimpNode, bool filterEmpty, bool isRoot = true);


#endif //_ASSIMP_H_INCLUDED_
