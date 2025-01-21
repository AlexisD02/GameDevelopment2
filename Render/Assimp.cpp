//-------------------------------------------------------------------------------------------------
// Assimp Helper Code
//-------------------------------------------------------------------------------------------------
// Structures and functions to simplify 3d mesh import using the assimp library

#include "Assimp.h"


//-------------------------------------------------------------------------------------------------
// Assimp Helper Functions
//-------------------------------------------------------------------------------------------------

// Read an open assimp scene structure and create structures for each contained material - fills in materials parameter
void ReadMaterials(const aiScene& scene, std::vector<AssimpMaterial>& materials)
{
	materials.clear();
	for (unsigned int m = 0; m < scene.mNumMaterials; ++m)
	{
		AssimpMaterial material;
		aiMaterial* assimpMat = scene.mMaterials[m];

		aiReturn ret;
		aiString materialName;
		ret = assimpMat->Get(AI_MATKEY_NAME, materialName);
		if (ret != AI_SUCCESS)  materialName = "";
		material.name = materialName.C_Str();
		if (material.name == "")  material.name = defaultMaterialName + std::to_string(m); // All materials get names
		ret = assimpMat->Get(AI_MATKEY_COLOR_DIFFUSE, material.diffuseColour);
		if (ret != AI_SUCCESS)  material.diffuseColour = aiColor4D(1, 1, 1, 1);
		ret = assimpMat->Get(AI_MATKEY_COLOR_SPECULAR, material.specularColour);
		ret = assimpMat->Get(AI_MATKEY_COLOR_AMBIENT, material.ambientColour);
		ret = assimpMat->Get(AI_MATKEY_COLOR_EMISSIVE, material.emissiveColour);
		ret = assimpMat->Get(AI_MATKEY_COLOR_TRANSPARENT, material.transparentColour);
		ret = assimpMat->Get(AI_MATKEY_BASE_COLOR, material.pbrBaseColour);

		int shadingMode;
		ret = assimpMat->Get(AI_MATKEY_SHADING_MODEL, shadingMode);
		if (ret != AI_SUCCESS)  shadingMode = aiShadingMode_Blinn;
		if (shadingMode == aiShadingMode_Phong)  shadingMode = aiShadingMode_Blinn; // Simple preference
		material.shadingMode = aiShadingMode(shadingMode);
		ret = assimpMat->Get(AI_MATKEY_SHININESS_STRENGTH, material.specularStrength);
		if (ret != AI_SUCCESS)  material.specularStrength = 1;
		ret = assimpMat->Get(AI_MATKEY_SHININESS, material.specularPower);
		if (ret != AI_SUCCESS) 	material.specularPower = 0; // Force no specular - see below
		if (material.specularPower == 0) // Specular power of 0 gives artefacts in shaders - force to 1 and assume means no specular
		{
			material.specularColour = { 0, 0, 0 };
			material.specularStrength = 0;
			material.specularPower = 1;
		}
		ret = assimpMat->Get(AI_MATKEY_ENABLE_WIREFRAME, material.wireframe);
		if (ret != AI_SUCCESS)  material.wireframe = 0;
		ret = assimpMat->Get(AI_MATKEY_TWOSIDED, material.twoSided);
		if (ret != AI_SUCCESS)  material.twoSided = 0;

		ret = assimpMat->Get(AI_MATKEY_TRANSPARENCYFACTOR, material.transparencyFactor);
		if (ret != AI_SUCCESS)  material.transparencyFactor = 1;
		float opacity;
		ret = assimpMat->Get(AI_MATKEY_OPACITY, opacity);
		if (ret == AI_SUCCESS && 1 - opacity < material.transparencyFactor)  material.transparencyFactor = 1 - opacity;
		ret = assimpMat->Get(AI_MATKEY_REFRACTI, material.refractiveIndex);
		if (ret != AI_SUCCESS)  material.refractiveIndex = 1;
		ret = assimpMat->Get(AI_MATKEY_REFLECTIVITY, material.reflectivity);
		if (ret != AI_SUCCESS)  material.reflectivity = 0;

		int blendFunction;
		ret = assimpMat->Get(AI_MATKEY_BLEND_FUNC, blendFunction);
		if (ret != AI_SUCCESS)  blendFunction = 0;
		material.blendFunction = aiBlendMode(blendFunction);

		ret = assimpMat->Get(AI_MATKEY_METALLIC_FACTOR, material.pbrMetallicFactor);
		if (ret != AI_SUCCESS)  material.pbrMetallicFactor = 0;
		ret = assimpMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, material.pbrRoughnessFactor);
		if (ret != AI_SUCCESS)  material.pbrRoughnessFactor = 0.5f;
		ret = assimpMat->Get(AI_MATKEY_ANISOTROPY_FACTOR, material.pbrAnisotropyFactor);
		if (ret != AI_SUCCESS)  material.pbrAnisotropyFactor = 0;
		ret = assimpMat->Get(AI_MATKEY_SPECULAR_FACTOR, material.pbrSpecularFactor);
		if (ret != AI_SUCCESS)  material.pbrSpecularFactor = 0.04f;
		ret = assimpMat->Get(AI_MATKEY_GLOSSINESS_FACTOR, material.pbrGlossinessFactor);
		if (ret != AI_SUCCESS)  material.pbrGlossinessFactor = 0.5f;

		for (int type = 0; type < sizeof(textureTypes) / sizeof(textureTypes[0]); ++type)
		{
			unsigned int numTextures = assimpMat->GetTextureCount(textureTypes[type]);
			if (numTextures > 0) // Each texture type can be a stack of textures to be combined - however, this code only supports a single texture of each type
			{
				unsigned int texture = 0;
				AssimpTexture assimpTexture;

				ret = assimpMat->Get(AI_MATKEY_TEXTURE(textureTypes[type], texture), assimpTexture.filePath);
				if (ret == AI_SUCCESS)
				{
					ret = assimpMat->Get(AI_MATKEY_UVWSRC(textureTypes[type], texture), assimpTexture.uvIndex);
					if (ret != AI_SUCCESS)  assimpTexture.uvIndex = texture;
					ret = assimpMat->Get(AI_MATKEY_TEXBLEND(textureTypes[type], texture), assimpTexture.blendFactor);
					if (ret != AI_SUCCESS)  assimpTexture.blendFactor = 1.0f;
					int blendOperation;
					ret = assimpMat->Get(AI_MATKEY_TEXOP(textureTypes[type], texture), blendOperation);
					if (ret != AI_SUCCESS)  blendOperation = aiTextureOp_Multiply;
					assimpTexture.blendOperation = aiTextureOp(blendOperation);
					int mapMode;
					ret = assimpMat->Get(AI_MATKEY_MAPPINGMODE_U(textureTypes[type], texture), mapMode);
					if (ret != AI_SUCCESS)  mapMode = aiTextureMapMode_Wrap;
					assimpTexture.mapModes[0] = aiTextureMapMode(mapMode);
					ret = assimpMat->Get(AI_MATKEY_MAPPINGMODE_V(textureTypes[type], texture), mapMode);
					if (ret != AI_SUCCESS)  mapMode = aiTextureMapMode_Wrap;
					assimpTexture.mapModes[1] = aiTextureMapMode(mapMode);
					assimpMat->Get(AI_MATKEY_TEXFLAGS(textureTypes[type], texture), assimpTexture.textureFlags);
					if (ret != AI_SUCCESS)  assimpTexture.textureFlags = 0;

					material.textures[textureTypes[type]].push_back(assimpTexture);
				}
			}
		}

		materials.push_back(material);
	}
}



// Count the number of nodes with given assimp node as root - recursive
// May optionally filter (not count) nodes with no submeshes. Also specify if starting from root node
unsigned int CountDescendantsOf(aiNode* assimpNode, bool filterEmpty, bool isRoot /*= true*/)
{
	unsigned int count = 0;
	if (assimpNode->mNumMeshes > 0 || isRoot || !filterEmpty)  ++count;
	for (unsigned int child = 0; child < assimpNode->mNumChildren; ++child)
		count += CountDescendantsOf(assimpNode->mChildren[child], filterEmpty, false);
	return count;
}
