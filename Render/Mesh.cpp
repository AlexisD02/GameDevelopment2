//--------------------------------------------------------------------------------------
// Class encapsulating a mesh
//--------------------------------------------------------------------------------------
// Meshes are geometry held by EntityTemplates and used by Entities.
// Uses the assimp 3d asset loader library to read many kinds of 3D files into a mesh class. The process is very complex as
// there are many kinds of data that can be present in a 3D file and it must be broken down into consistently renderable mesh.
// 
// Basic process:
// - Allow assimp to read the file into an assimp "scene" (which we call a mesh)
// - Scan all the materials used in the mesh to identify rendering techniques, geometry and textures required
// - Break the mesh into "submeshes", each using only one of these materials
// - Convert the materials into "render methods" - the best way to draw a given submesh given the methods this app supports
// - Reprocess the assimp scene to extract just what data we need to support the render methods we chose, creating vertex and index buffers for each submesh
// - Convert each render method into DirectX objects - i.e. load textures, load shaders etc.
// - Copy over the "node" heirarchy of the mesh to our own format - for animated / skinned objects
// - Now we have a hierarchical structure controlling submeshes, each of which contains the DirectX content it needs to be rendered
// Once constructed, the Mesh class provides a Render function that only needs a collection of transform matrices to render the mesh

#include "Mesh.h"
#include "Assimp.h"

#include "Shader.h"  // Needed for helper function CreateSignatureForVertexLayout
#include "CBuffer.h" // Needed for helper function UpdateCBuffer
#include "CBufferTypes.h"
#include "RenderGlobals.h"

#include "Vector3.h" 
#include "Vector2.h" 
#include "Utility.h"  // For string utility functions 

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>

#include <stdexcept>


//--------------------------------------------------------------------------------------
// Render Method Helper Functions
//--------------------------------------------------------------------------------------

// Return a render method that describes how a given assimp material should be rendered, based on the material settings and any
// textures found that match the material texture name. A render method includes pixel rendering approach (SurfaceRenderMethod),
// shader constants such as diffuse colour, specular colour, parallax depth etc, and a list of textures/samplers.
// Later in the import process, the render method will be converted to a RenderState object containing DirectX objects we can
// set on the GPU to render using this material.
// 
// A render method also contains vertex processing approach (GeometryRenderMethod), which describes whether the mesh is skinned,
// or part of a rigid hierarchy or static. However, we are only considering a material type here and do not know about the mesh
// geometry yet so that field is left Unknown, to be updated later in the import process.
//
// Whilst assimp can provide information about shading model, pbr usage etc, it cannot be relied upon to correctly report these things since
// many file formats simply don't contain that information. So most logic here is based on the names of the texture files in use
//
// To use textures one of these two maps must exist - the suffix on this base map (diffuse or albedo) is optional, but recommended
// - Diffuse map: brick_diffuse.jpg or brick.jpg (Required for Blinn-Phong (2nd year style) lighting)
// - Albedo map:  brick_albedo.jpg or brick.jpg  (Required for PBR, however the _diffuse suffix will be accepted too)
//
// Other maps
// - Specular map:     brick_specular.jpg     (optional map for Blinn-Phong lighting (2nd year style) and PBR specular/glossiness approach - will fall back to specular colour/factor in material)
// - Normal map:       brick_normal.jpg       (required for normal mapping or PBR)
// - Displacement map: brick_displacement.jpg (required for parallax mapping, also called a height map but _height suffix is not supported)
// - Metalness map:    brick_metalness.jpg    (optional for PBR metalness/roughness approach - will fall back to metalness factor in material)
// - Roughness map:    brick_roughness.jpg    (required for PBR metalness/roughness approach)
// - Glossiness map:   brick_gloss.jpg        (required for PBR specular/glossiness approach)
// 
RenderMethod MaterialRenderMethod(AssimpMaterial& material, std::filesystem::path defaultPath, bool noLighting = false)
{
	RenderMethod renderMethod;

	renderMethod.geometryRenderMethod = GeometryRenderMethod::Unknown; // Can't be determined from a material, so left at this here
	renderMethod.surfaceRenderMethod  = SurfaceRenderMethod ::Unknown; // Will be determined in code below

	// Copy over base material colours and settings
	renderMethod.constants.diffuseColour = { material.diffuseColour.r, material.diffuseColour.g, material.diffuseColour.b, material.diffuseColour.a };
	aiColor3D colour = material.specularColour * material.specularStrength;
	renderMethod.constants.specularColour = { colour.r, colour.g, colour.b };
	renderMethod.constants.specularPower = material.specularPower;
	renderMethod.constants.parallaxDepth = 0.02f;

	// Only considering assimp's diffuse and base colour textures. It may have identified more, but we cannot expect consistency across different 3d file formats
	bool hasDiffuseTexture    = material.textures.find(aiTextureType_DIFFUSE)    != material.textures.end();
	bool hasBaseColourTexture = material.textures.find(aiTextureType_BASE_COLOR) != material.textures.end();


	/////////////////////////////////////
	// If diffuse and base colour missing assume no textures used - plain colour, possibly with lighting
	// Also allow caller to request to ignore textures
	if (!hasDiffuseTexture && !hasBaseColourTexture)
	{
		if (noLighting || material.shadingMode == aiShadingMode_Unlit)
			renderMethod.surfaceRenderMethod = SurfaceRenderMethod::UnlitColour;
		else
			renderMethod.surfaceRenderMethod = SurfaceRenderMethod::BlinnColour;
		return renderMethod;
	}


	// Temporary map from assimp texture addressing modes to our own
	const std::map<aiTextureMapMode, TextureAddressingMode> MapModeToSamplerState =
	{
		{aiTextureMapMode_Wrap,   TextureAddressingMode::AddressingWrap  },
		{aiTextureMapMode_Clamp,  TextureAddressingMode::AddressingClamp },
		{aiTextureMapMode_Decal,  TextureAddressingMode::AddressingClamp },
		{aiTextureMapMode_Mirror, TextureAddressingMode::AddressingMirror},
	};


	/////////////////////////////////////
	// Check for metalness/roughness PBR route


	// Use base colour if it exists, otherwise diffuse
	aiTextureType assimpBaseTextureType = hasBaseColourTexture ? aiTextureType_BASE_COLOR : aiTextureType_DIFFUSE;
	std::filesystem::path texturePath, secondaryPath;
	std::string stem, extension, base;
	if (hasDiffuseTexture || hasBaseColourTexture) 
	{
		texturePath = material.textures[assimpBaseTextureType][0].filePath.C_Str();
		if (texturePath.is_relative())  texturePath = defaultPath / texturePath;
		stem = texturePath.stem().string();
		extension = texturePath.extension().string();
		if      (EndsWithCI(stem, "albedo"))   base = stem.substr(0, stem.size() - 6);  // Remove "albedo" suffix
		else if (EndsWithCI(stem, "diffuse"))  base = stem.substr(0, stem.size() - 7);  // Will accept "diffuse" suffix also, remove it
		else                                   base = stem + "_"; // Without a recognised suffix for base map, assume the base map name has no suffix (but other maps must have correct suffixes)
	}

	// Get texture addressing mode for this texture, assume all others will use the same
	auto mapMode = material.textures[assimpBaseTextureType][0].mapModes[0];
	auto mapModeState = MapModeToSamplerState.contains(mapMode) ? MapModeToSamplerState.at(mapMode) : TextureAddressingMode::AddressingWrap;

	// User asked for no lighting, skip PBR tests
	if (noLighting)  goto FAIL_PBR2;

	// Add albedo texture as the first texture for this material's render method
	renderMethod.textures.push_back({ TextureType::Albedo, texturePath.string(), { TextureFilter::FilterAnisotropic, mapModeState } });

	// Look for other textures required for PBR, if core ones are missing, quit looking (using goto since it is much more readable than the nested ifs that would otherwise be required)
	secondaryPath = texturePath.parent_path() / (base + "roughness" + extension);
	if (!std::filesystem::exists(secondaryPath))  goto FAIL_PBR1;
	renderMethod.textures.push_back({ TextureType::Roughness, secondaryPath.string(), { TextureFilter::FilterAnisotropic, mapModeState } });

	secondaryPath = texturePath.parent_path() / (base + "normal" + extension);
	if (!std::filesystem::exists(secondaryPath))  goto FAIL_PBR1;
	renderMethod.textures.push_back({ TextureType::Normal, secondaryPath.string(), { TextureFilter::FilterTrilinear, mapModeState } });

	// Metalness map is optional, will fall back to material metalness factor
	secondaryPath = texturePath.parent_path() / (base + "metalness" + extension);
	if (std::filesystem::exists(secondaryPath))
		renderMethod.textures.push_back({ TextureType::Metalness, secondaryPath.string(), { TextureFilter::FilterAnisotropic, mapModeState } });

	// Displacement map is optional - determines if we have PBR normal or PBR parallax mapping
	secondaryPath = texturePath.parent_path() / (base + "displacement" + extension);
	if (std::filesystem::exists(secondaryPath))
	{
		renderMethod.textures.push_back({ TextureType::Displacement, secondaryPath.string(), { TextureFilter::FilterTrilinear, mapModeState } });
		renderMethod.surfaceRenderMethod = SurfaceRenderMethod::PbrParallaxMapping;
	}
	else
	{
		renderMethod.surfaceRenderMethod = SurfaceRenderMethod::PbrNormalMapping;
	}

	return renderMethod;


	/////////////////////////////////////
	// If we could not identify first PBR approach, then try alternative Specular/Glossiness PBR route
FAIL_PBR1:
	renderMethod.textures.clear(); // Wipe any textures found in the first PBR search

	// Once again, add albedo texture as the first texture for this material's render method
	renderMethod.textures.push_back({ TextureType::Albedo, texturePath.string(), { TextureFilter::FilterAnisotropic, mapModeState } });

	// Look for other textures required for alternate PBR approach, if core ones are missing, quit looking
	secondaryPath = texturePath.parent_path() / (base + "specular" + extension);
	if (!std::filesystem::exists(secondaryPath))  goto FAIL_PBR2;
	renderMethod.textures.push_back({ TextureType::Specular, secondaryPath.string() , { TextureFilter::FilterAnisotropic, mapModeState } });

	secondaryPath = texturePath.parent_path() / (base + "gloss" + extension);
	if (!std::filesystem::exists(secondaryPath))  goto FAIL_PBR2;
	renderMethod.textures.push_back({ TextureType::Gloss, secondaryPath.string(), { TextureFilter::FilterAnisotropic, mapModeState } });

	secondaryPath = texturePath.parent_path() / (base + "normal" + extension);
	if (!std::filesystem::exists(secondaryPath))  goto FAIL_PBR2;
	renderMethod.textures.push_back({ TextureType::Normal, secondaryPath.string(), { TextureFilter::FilterTrilinear, mapModeState } });

	// Once more displacement map is optional - determines if we have PBR normal or PBR parallax mapping
	secondaryPath = texturePath.parent_path() / (base + "displacement" + extension);
	if (std::filesystem::exists(secondaryPath))
	{
		renderMethod.textures.push_back({ TextureType::Displacement, secondaryPath.string(), { TextureFilter::FilterTrilinear, mapModeState } });
		renderMethod.surfaceRenderMethod = SurfaceRenderMethod::PbrAltNormalMapping;
	}
	else
	{
		renderMethod.surfaceRenderMethod = SurfaceRenderMethod::PbrAltParallaxMapping;
	}
	return renderMethod;


	/////////////////////////////////////
	// If could not identify either PBR approach, but do have textures then fall back to unlit texture or Blinn-Phong lit textures (2nd year Graphics style rendering)
FAIL_PBR2:
	renderMethod.textures.clear(); // Wipe any textures found in the PBR search

	// Add diffuse texture as the first texture for this material's render method
	renderMethod.textures.push_back({ TextureType::Diffuse, texturePath.string(), { TextureFilter::FilterAnisotropic, mapModeState } });
	
	// If not using lighting, other textures are unnecesary and we are finished
	if (noLighting || material.shadingMode == aiShadingMode_Unlit)
	{
		renderMethod.surfaceRenderMethod = SurfaceRenderMethod::UnlitTexture;
		return renderMethod;
	}
	
	// We have at least textured Blinn lighting
	renderMethod.surfaceRenderMethod = SurfaceRenderMethod::BlinnTexture;

	// Specular map is optional, however shader will try and use it so if there is no map then force a null texture with "" filename (null texture returns black always)
	secondaryPath = texturePath.parent_path() / (base + "specular" + extension);
	if (std::filesystem::exists(secondaryPath))
		renderMethod.textures.push_back({ TextureType::Specular, secondaryPath.string(), { TextureFilter::FilterAnisotropic, mapModeState } });
	else
		renderMethod.textures.push_back({ TextureType::Specular, "", {TextureFilter::FilterAnisotropic, mapModeState}}); // Null specular texture, do set sampler or DirectX debugger issues warnings

	// Look for support for normal and parallax mapping
	secondaryPath = texturePath.parent_path() / (base + "normal" + extension);
	if (std::filesystem::exists(secondaryPath))
	{
		renderMethod.surfaceRenderMethod = SurfaceRenderMethod::BlinnNormalMapping;
		renderMethod.textures.push_back({ TextureType::Normal, secondaryPath.string(), { TextureFilter::FilterTrilinear, mapModeState } });

		secondaryPath = texturePath.parent_path() / (base + "displacement" + extension);
		if (std::filesystem::exists(secondaryPath))
		{
			renderMethod.surfaceRenderMethod = SurfaceRenderMethod::BlinnParallaxMapping;
			renderMethod.textures.push_back({ TextureType::Displacement, secondaryPath.string(), { TextureFilter::FilterTrilinear, mapModeState } });
		}
	}

	return renderMethod;
}


// Return geometry types (position, normal, tangent etc.) required by the given render method
GeometryTypes RenderMethodGeometryRequirements(const RenderMethod& renderMethod)
{
	GeometryTypes GeometryTypes = GeometryTypes::Position;

	if (renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnColour ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnTexture ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnParallaxMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrParallaxMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrAltNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrAltParallaxMapping)
		GeometryTypes |= GeometryTypes::Normal;

	if (renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnParallaxMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrParallaxMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrAltNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrAltParallaxMapping)
		GeometryTypes |= GeometryTypes::Tangent;

	if (renderMethod.surfaceRenderMethod == SurfaceRenderMethod::UnlitTexture ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnTexture ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::BlinnParallaxMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrParallaxMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrAltNormalMapping ||
		renderMethod.surfaceRenderMethod == SurfaceRenderMethod::PbrAltParallaxMapping)
		GeometryTypes |= GeometryTypes::UV;

	if (renderMethod.geometryRenderMethod == GeometryRenderMethod::Skinned)
		GeometryTypes |= GeometryTypes::BoneData;

	return GeometryTypes;
}


// Calculate geometry types required for an entire assimp scene based on its materials. Returns the minimal set of types that
// covers the needs of every material. This is used for Assimp post-processing flags to remove unnecessary data. However, these
// types apply to the whole scene rather than per-mesh so will lose some opportunities for vertex welding when importing.
// Nevertheless, data not needed per-submesh will not be copied over to the final data (vertex buffer).
// Will not consider skinning since it is not a material propety, i.e. it will not return GeometryTypes::BoneData
GeometryTypes SceneGeometryRequirements(const aiScene& scene, std::vector<AssimpMaterial>& materials, std::filesystem::path defaultPath, bool noLighting = false)
{
	GeometryTypes geometryTypes = {};

	// Add input requirements of each mesh in turn
	for (unsigned int i = 0; i < scene.mNumMeshes; ++i)
	{
		// Get render method for this submesh
		auto renderMethod = MaterialRenderMethod(materials[scene.mMeshes[i]->mMaterialIndex], defaultPath, noLighting);
		
		// Combine geometry requirements of this submesh's render method and combine with those we found so far
		geometryTypes |= RenderMethodGeometryRequirements(renderMethod);
	}

	return geometryTypes;
}



//--------------------------------------------------------------------------------------
// Mesh Construction
//--------------------------------------------------------------------------------------

// Pass the name of the mesh file to load. Uses assimp (http://www.assimp.org/) to support many file types
// Will throw a std::runtime_error exception on failure (since constructors can't return errors).
// Optionally pass extra import flags over and above the default. Available flags here are:
//     OptimiseHierarchy, FlattenHierarchyExceptBones, FlattenHierarchy, UVAxisUp, SelectiveDebone and NoLighting
Mesh::Mesh(const std::string& fileName, ImportFlags additionalImportFlags /* = {}*/)
{
	// Assimp provides a huge amount of control over how meshes are imported. All assimp import settings are in
	// variables and constants prefixed by "ai" - hover on any of these, or right-click and "Peek Definition" to see the documention above it

	Assimp::Importer importer;

	// Settings for import
	ImportFlags importFlags = ImportFlags::SimpleUVMapping | ImportFlags::RemoveLinesPoints |
		                      ImportFlags::RetainHierarchy | ImportFlags::RemoveDegenerates |
					          ImportFlags::FixNormals      | ImportFlags::Validate;
	importFlags |= additionalImportFlags;
	unsigned int maxVerticesPerMesh  = 65536;     // No submesh will have more than this many vertices, set to 0 for no limit
	unsigned int maxTrianglesPerMesh = 1000000;   // No submesh will have more than this many triangles, set to 0 for no limit
	unsigned int maxBonesPerVertex   = 4;         // In skinned meshes, no vertex will be affected by more than this number of bones
	unsigned int maxBonesPerMesh     = MAX_BONES; // In skinned meshes, no submesh will have more than this many bones
	float creaseAngle        = 0.0f;  // Recalculating normals - the min angle between two faces that should look sharp, if this value is 0 the original normals will be retained
	float tangentCreaseAngle = 45.0f; // Recalculating tangents and bitangents - same idea as above. Best to leave this value at 45 to recalculate, or 0 to keep original data

	mFilepath = fileName;
	if (mFilepath.is_relative())
	{
		mFilepath = std::filesystem::current_path() / "Media" / mFilepath;
	}
	std::string stem      = mFilepath.stem().string();
	std::string extension = mFilepath.extension().string();


	//-----------------------------------
	// First pass at reading file - an analysis to decide how to properly import it

	unsigned int assimpFlags = {};
		
	// Enable assimp logging - visible in Visual Studio output window
	if (IsSet(importFlags & ImportFlags::Validate)) 
	{
		assimpFlags |= aiProcess_ValidateDataStructure;
		Assimp::DefaultLogger::create("", Assimp::DefaultLogger::VERBOSE);
	}

	// Don't want unused materials affecting final geometry requirements (see next section)
	assimpFlags |= aiProcess_RemoveRedundantMaterials; 

	// First pass file reading
	const aiScene* scene = importer.ReadFile(mFilepath.string(), assimpFlags);

	// Disable logging again
	if (IsSet(importFlags & ImportFlags::Validate)) 
	{
		Assimp::DefaultLogger::kill();
	}

	// Quit if any serious errors
	if (scene == nullptr)
	{
		throw std::runtime_error(importer.GetErrorString());
	}
	else if (IsSet(importFlags & ImportFlags::Validate) && (scene->mFlags & AI_SCENE_FLAGS_VALIDATION_WARNING) != 0)
	{
		throw std::runtime_error("Mesh Import: Validation warning importing " + mFilepath.string());
	}


	//-----------------------------------
	// Read materials that assimp found in the file and identity what geometry data will be needed to support those materials

	// Read materials from assimp scene
	std::vector<AssimpMaterial> materials;
	ReadMaterials(*scene, materials);

	// Get maximal geometry input requirements based on materials to help specify / optimise import
	bool noLighting = IsSet(importFlags & ImportFlags::NoLighting);
	GeometryTypes requiredSceneGeometry = SceneGeometryRequirements(*scene, materials, mFilepath.parent_path(), noLighting);


	//-----------------------------------
	// Based on the analysis of the mesh file, select the final assimp settings for import
	
	int removeComponents = 0;

	// By default ignore animations, lights and cameras in mesh files
	removeComponents |= aiComponent_ANIMATIONS | aiComponent_LIGHTS | aiComponent_CAMERAS;

	// Also ignore textures embedded in the mesh file - not supporting this unusual case. Textures held in files are unaffected by this
	removeComponents |= aiComponent_TEXTURES; 

	// By default convert everything to left-handed, DirectX style meshes
	// Also triangulate and make any non-destructive optimisations.
	assimpFlags |= aiProcess_ConvertToLeftHanded |
	               aiProcess_Triangulate |
	               aiProcess_JoinIdenticalVertices |
	               aiProcess_ImproveCacheLocality |
	               aiProcess_RemoveRedundantMaterials |
	               aiProcess_SortByPType |
	               aiProcess_FindInvalidData |
	               aiProcess_OptimizeMeshes |
	               aiProcess_FindInstances |
	               aiProcess_Debone;
	importer.SetPropertyBool(AI_CONFIG_PP_DB_ALL_OR_NONE, true);


	//////////////////////////
	// Remove unneeded geometry data if not required by materials used by the mesh
	// Also recalculate normals/tangents/bitangents where necessary

	// Are normals required?
	if (IsSet(requiredSceneGeometry & GeometryTypes::Normal) ||
		IsSet(requiredSceneGeometry & GeometryTypes::Tangent) ||
		IsSet(requiredSceneGeometry & GeometryTypes::Bitangent))
	{
		if (creaseAngle > 0) // Recalculate normals if a crease angle is specified
		{
			removeComponents |= aiComponent_NORMALS;
			importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, creaseAngle);
		}
		assimpFlags |= aiProcess_GenSmoothNormals;
	}
	else
	{
		removeComponents |= aiComponent_NORMALS;
	}

	// Are tangents & bitangents required?
	if (IsSet(requiredSceneGeometry & GeometryTypes::Tangent) || IsSet(requiredSceneGeometry & GeometryTypes::Bitangent))
	{
		if (tangentCreaseAngle > 0) // Recalculate normals if a tangent crease angle is specified
		{
			removeComponents |= aiComponent_TANGENTS_AND_BITANGENTS;
			importer.SetPropertyFloat(AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE, tangentCreaseAngle);
		}
		assimpFlags |= aiProcess_CalcTangentSpace;
	}
	else
	{
		removeComponents |= aiComponent_TANGENTS_AND_BITANGENTS;
	}

	// Are texture coordinates required
	if (!IsSet(requiredSceneGeometry & GeometryTypes::UV))
	{
		removeComponents |= aiComponent_TEXCOORDS;
	}

	// Are vertex colours required
	if (!IsSet(requiredSceneGeometry & GeometryTypes::Colour))
	{
		removeComponents |= aiComponent_COLORS;
	}


	//////////////////////////
	// UV manipulations

	if (IsSet(importFlags & ImportFlags::SimpleUVMapping)) // Remove unusual UV mappings (spherical mapping, cylindrical mapping etc.) and UV transforms
	{
		assimpFlags |= aiProcess_GenUVCoords | aiProcess_TransformUVCoords;
	}
	if (IsSet(importFlags & ImportFlags::UVAxisUp)) // Assume the +V axis of texture space goes up rather than the usual down
	{
		assimpFlags &= ~aiProcess_FlipUVs;
	}


	//////////////////////////
	// Node Hierarchy Adjustments

	if ((importFlags & ImportFlags::HierarchyFlags) == ImportFlags::FlattenHierarchy)  // Remove mesh hierarchy, returning a single unanimatable mesh
	{
		assimpFlags |= aiProcess_PreTransformVertices;
		removeComponents |= aiComponent_BONEWEIGHTS;
	}
	else if ((importFlags & ImportFlags::HierarchyFlags) == ImportFlags::FlattenHierarchyExceptBones)  // Remove any nodes that don't have bones
	{
		assimpFlags |= aiProcess_OptimizeGraph;
	}


	//////////////////////////
	// Deal with unwanted / broken geometry

	if (IsSet(importFlags & ImportFlags::RemoveLinesPoints)) // Used when we only want triangles in meshes
	{
		importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
	}
	if (IsSet(importFlags & ImportFlags::RemoveDegenerates)) // Degenerate geometry is removed (e.g. a triangle with two points in the same place)
	{
		assimpFlags |= aiProcess_FindDegenerates;
		if (!IsSet(importFlags & ImportFlags::RemoveLinesPoints))
		{
			importer.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
		}
	}

	if (IsSet(importFlags & ImportFlags::FixNormals)) // Attempt to spot and correct inside-out parts of meshes
	{
		assimpFlags |= aiProcess_FixInfacingNormals;
	}

	if (IsSet(importFlags & ImportFlags::SelectiveDebone)) // Remove bones from submeshes that don't need them. E.g. the eyes in a skinned character
	{
		assimpFlags |= aiProcess_Debone;
		importer.SetPropertyBool(AI_CONFIG_PP_DB_ALL_OR_NONE, false);
	}


	//////////////////////////
	// Set limits for total vertices / triangles in one submesh
	if (maxVerticesPerMesh != 0 || maxTrianglesPerMesh != 0)
	{
		importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT,   maxVerticesPerMesh  != 0 ? maxVerticesPerMesh  : std::numeric_limits<int>::max());
		importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxTrianglesPerMesh != 0 ? maxTrianglesPerMesh : std::numeric_limits<int>::max());
		assimpFlags |= aiProcess_SplitLargeMeshes;
	}


	//////////////////////////
	// Set limits for total bones used by a submesh / vertex
	if (maxBonesPerVertex != 0)
	{
		importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, maxBonesPerVertex);
		assimpFlags |= aiProcess_LimitBoneWeights;
	}
	if (maxBonesPerMesh != 0)
	{
		importer.SetPropertyInteger(AI_CONFIG_PP_SBBC_MAX_BONES, maxBonesPerMesh);
		assimpFlags |= aiProcess_SplitByBoneCount;
	}


	//////////////////////////
	// Ask assimp to remove any geometry data we identified as unnecessary or requiring recalculation
	if (removeComponents != 0)
	{
		importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, removeComponents);
		assimpFlags |= aiProcess_RemoveComponent;
	}


	//-----------------------------------
	// Reprocess the file with the updated settings calculated above

	if (IsSet(importFlags & ImportFlags::Validate))
	{
		Assimp::DefaultLogger::create("", Assimp::DefaultLogger::VERBOSE);
	}
	scene = importer.ApplyPostProcessing(assimpFlags);
	if (IsSet(importFlags & ImportFlags::Validate))
	{
		Assimp::DefaultLogger::kill();
	}

	// Quit on serious errors
	if (scene == nullptr)
	{
		throw std::runtime_error(importer.GetErrorString());
	}
	else if (IsSet(importFlags & ImportFlags::Validate) && (scene->mFlags & AI_SCENE_FLAGS_VALIDATION_WARNING) != 0)
	{
		throw std::runtime_error("Mesh Import: Validation warning importing " + mFilepath.string());
	}


	// Read scene materials again as they may have changed in odd cases (e.g. if a material that was attached only to degenerate geometry is now redundant)
	ReadMaterials(*scene, materials);

	// Create render method for each material. This describes what GPU data / settings are required to render a particular
	// submesh - including what shaders to use, what textures are required, what states need to be set etc.
	// Most of this information comes from the material the submesh uses, however a couple of details are related to the
	// geometry, notably whether the submesh needs a skinning vertex shader.
	// The render methods created here are related to the material only - they will still need the geometry details added
	// later on in the code when processing each submesh.
	std::vector<RenderMethod> materialRenderMethods(materials.size());
	for (unsigned int i = 0; i < materials.size(); ++i)
	{
		materialRenderMethods[i] = MaterialRenderMethod(materials[i], mFilepath.parent_path(), noLighting);
	}


	//-----------------------------------
	// We have now imported an assimp mesh tailored for our requirements. Now convert this mesh to our own structures.
	// This involves identifying the exact needs of each submesh, copying over the data required for that submesh to 
	// GPU buffers and preparing render methods specific for each submesh based on their geometry and material

	// Create the node hierarchy. A simple object might just contain a root node. However, rigid body animated objects
	// or skinned objects will contain a tree of nodes, one for each controllable part of the mesh
	bool filterEmptyNodes = ((importFlags & ImportFlags::HierarchyFlags) == ImportFlags::OptimiseHierarchy); // Can optionally remove nodes with no submeshes, but don't do this if you have dummy nodes
	unsigned int numNodes = CountDescendantsOf(scene->mRootNode, filterEmptyNodes);
	mNodes.resize(numNodes);
	mAbsoluteTransforms.resize(numNodes);
	ReadNodes(scene->mRootNode, filterEmptyNodes);

	// Create collection of all submeshes (each node created above can contain one or more submeshes)
	// Then loop through each submesh and create our structures / GPU data based on what was imported from assimp
	mSubMeshes.resize(scene->mNumMeshes);
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* assimpMesh = scene->mMeshes[i];
		SubMesh& subMesh = mSubMeshes[i];

		// Each submesh is named, either with the name from assimp/original file, or a unique name based on the name of the node that contains it
		subMesh.name = assimpMesh->mName.C_Str();
		if (subMesh.name == "")  subMesh.name = mNodes[subMesh.nodeIndex].name + std::to_string(i);
		subMesh.materialName = materials[assimpMesh->mMaterialIndex].name; // Each submesh uses just one material, so has just one material name


		//-----------------------------------
		// Prepare GPU settings (render state) required by submesh material

		// Get render method required for the material used by this submesh, then update it based on the geometry of the submesh
		// This is now the final render method for this submesh
		RenderMethod& renderMethod = materialRenderMethods[assimpMesh->mMaterialIndex];
		renderMethod.geometryRenderMethod = assimpMesh->HasBones() ? GeometryRenderMethod::Skinned : GeometryRenderMethod::Rigid;

		// Create DirectX objects (shaders, texture objects etc.) to match the render method we have identified
		// Can throw std::runtime_error
		try
		{
			subMesh.renderState = std::make_unique<RenderState>(renderMethod);
		}
		catch (std::runtime_error e)
		{
			throw std::runtime_error("Mesh Import: cannot create render state for mesh / material: " + subMesh.name + " / " + subMesh.materialName + " in " + mFilepath.string() + " - " + std::string(e.what()));
		}


		//-----------------------------------
		// Identify/validate/prepare the geometry needed for this submesh
		
		// Get the exact geometry types required by this render method. Each submesh will only copy over the data that is needed.
		// For example if one submesh in a mesh is not affected by light (e.g. a special effect on an object) then that submesh will
		// not copy over vertex normals, even if the rest of the mesh does
		subMesh.geometryTypes = RenderMethodGeometryRequirements(renderMethod);

		// Set up DirectX structure that specifies the geometry content of this submesh, i.e. the data held with each vertex
		std::vector<D3D11_INPUT_ELEMENT_DESC> vertexElements;
		unsigned int offset = 0;

		// Submesh requires vertex positions (always required in practice)
		unsigned int positionOffset = offset;
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Position))
		{
			if (!assimpMesh->HasPositions())
				throw std::runtime_error("Mesh Import: missing positions for mesh/material: " + subMesh.name + "/" + subMesh.materialName + " in " + mFilepath.string());
			vertexElements.push_back({ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, positionOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 });
			offset += 12;
		}

		// Submesh requires vertex normals
		unsigned int normalOffset = offset;
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Normal))
		{
			if (!assimpMesh->HasNormals())
				throw std::runtime_error("Mesh Import: missing normals for mesh/material: " + subMesh.name + "/" + subMesh.materialName + " in " + mFilepath.string());
			vertexElements.push_back({ "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, normalOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 });
			offset += 12;
		}

		// Submesh requires vertex tangents (used for normal/parallax mapping)
		unsigned int tangentOffset = offset;
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Tangent))
		{
			if (!assimpMesh->HasTangentsAndBitangents())
				throw std::runtime_error("Mesh Import: missing tangents or bitangents for mesh/material: " + subMesh.name + "/" + subMesh.materialName + " in " + mFilepath.string());
			vertexElements.push_back({ "tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, tangentOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 });
			offset += 12;
		}

		// Submesh requires vertex bitangents (used for normal/parallax mapping, but often left out and calculated in the shader)
		unsigned int bitangentOffset = offset;
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Bitangent))
		{
			if (!assimpMesh->HasTangentsAndBitangents())
				throw std::runtime_error("Mesh Import: missing tangents or bitangents for mesh/material: " + subMesh.name + "/" + subMesh.materialName + " in " + mFilepath.string());
			vertexElements.push_back({ "bitangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, tangentOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 });
			offset += 12;
		}

		// Submesh requires texture coordinates (UVs)
		std::string uvName; // Careful - needs to stay in scope until vertexElements is converted to DirectX object below
		unsigned int uvOffset = offset;
		if (IsSet(subMesh.geometryTypes & GeometryTypes::UV))
		{
			unsigned int uvChannel = 0;
			int numUVChannels = assimpMesh->GetNumUVChannels();
			if (numUVChannels > 0)
			{
				for (unsigned int j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j)
				{
					if (assimpMesh->HasTextureCoords(j))
					{
						if (assimpMesh->mNumUVComponents[j] != 2)  throw std::runtime_error("Unsupported number of texture coordinates in " + subMesh.name + "/" + subMesh.materialName + " in " + mFilepath.string());
						uvName = "uv";
						if (numUVChannels > 1)  uvName += std::to_string(uvChannel);
						vertexElements.push_back({ uvName.c_str(), uvChannel, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0});
						++uvChannel;
						offset += 8;
						break; // Only supporting single UV channel
					}
				}
			}
			if (uvChannel == 0) // != assimpMesh->GetNumUVChannels())
				throw std::runtime_error("Mesh Import: missing UVs for mesh/material: " + subMesh.name + "/" + subMesh.materialName + " in " + mFilepath.string());
		}

		// Submesh requires vertex colours (rarely used)
		std::string colourName;  // Careful - needs to stay in scope until vertexElements is converted to DirectX object below
		unsigned int colourOffset = offset;
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Colour))
		{
			unsigned int colourChannel = 0;
			int numColourChannels = assimpMesh->GetNumColorChannels();
			if (numColourChannels > 0)
			{
				for (unsigned int j = 0; j < AI_MAX_NUMBER_OF_COLOR_SETS; ++j)
				{
					if (assimpMesh->HasVertexColors(j))
					{
						colourName = "colour";
						if (numColourChannels > 1)  colourName += std::to_string(colourChannel);
						vertexElements.push_back({ colourName.c_str(), colourChannel, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 });
						++colourChannel;
						offset += 4;
						break; // Only supporting single colour channel
					}
				}
			}
			if (colourChannel == 0) // != assimpMesh->GetNumColorChannels())
				throw std::runtime_error("Mesh Import: missing vertex colours for mesh/material: " + subMesh.name + "/" + subMesh.materialName + " in " + mFilepath.string());
		}

		// Submesh requires bone data for skinning (bone indexes and weights)
		unsigned int bonesOffset = offset;
		if (IsSet(subMesh.geometryTypes & GeometryTypes::BoneData))
		{
			if (!assimpMesh->HasBones())
				throw std::runtime_error("Mesh Import: missing bones for mesh: " + subMesh.name + " in " + mFilepath.string());
			vertexElements.push_back({ "bones"  , 0, DXGI_FORMAT_R8G8B8A8_UINT,      0, bonesOffset,     D3D11_INPUT_PER_VERTEX_DATA, 0 });
			offset += 4;
			vertexElements.push_back({ "weights", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, bonesOffset + 4, D3D11_INPUT_PER_VERTEX_DATA, 0 });
			offset += 16;
		}

		// Final size of a single vertex in this submesh
		subMesh.vertexSize = offset;


		// Create a "vertex layout" to describe to DirectX what data is in each vertex of this submesh
		auto shaderSignature = CreateSignatureForVertexLayout(vertexElements.data(), static_cast<int>(vertexElements.size()));
		HRESULT hr = DX->Device()->CreateInputLayout(vertexElements.data(), static_cast<UINT>(vertexElements.size()),
			                                         shaderSignature->GetBufferPointer(), shaderSignature->GetBufferSize(), &subMesh.vertexLayout);
		if (shaderSignature)  shaderSignature->Release();
		if (FAILED(hr))  throw std::runtime_error("Failure creating input layout for " + mFilepath.string());



		//-----------------------------------
		// The vertex data required for this submesh has been identified and the size of each vertex is now known
		// Create CPU-side arrays to hold the submesh data - since vertex content is flexible, we can't use a structure so we just use an array of bytes
		
		// Note: for large amounts of data a unique_ptr array is better than a vector because vectors default-initialise all the values which is a waste of time.
		subMesh.numVertices = assimpMesh->mNumVertices;
		subMesh.numIndices  = assimpMesh->mNumFaces * 3;
		auto vertices = std::make_unique<unsigned char[]>(subMesh.numVertices * subMesh.vertexSize);
		auto indices  = std::make_unique<unsigned char[]>(subMesh.numIndices * 4); // Always using 32 bit indexes (4 bytes) for each index here


		//-----------------------------------
		// Copy submesh data from assimp to our CPU-side arrays
		// Copying code is low level and intricate because the arrays are just byte-arrays (see comment above)

		// Copy vertex positions if present (in practice they will be)
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Position))
		{
			Vector3* assimpPosition = reinterpret_cast<Vector3*>(assimpMesh->mVertices);
			unsigned char* position = vertices.get() + positionOffset;
			unsigned char* positionEnd = position + subMesh.numVertices * subMesh.vertexSize;
			while (position != positionEnd)
			{
				*(Vector3*)position = *assimpPosition;
				position += subMesh.vertexSize;
				++assimpPosition;
			}
		}

		// Copy vertex normals if present
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Normal))
		{
			Vector3* assimpNormal = reinterpret_cast<Vector3*>(assimpMesh->mNormals);
			unsigned char* normal = vertices.get() + normalOffset;
			unsigned char* normalEnd = normal + subMesh.numVertices * subMesh.vertexSize;
			while (normal != normalEnd)
			{
				*(Vector3*)normal = *assimpNormal;
				normal += subMesh.vertexSize;
				++assimpNormal;
			}
		}

		// Copy vertex tangents if present
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Tangent))
		{
			Vector3* assimpTangent = reinterpret_cast<Vector3*>(assimpMesh->mTangents);
			unsigned char* tangent = vertices.get() + tangentOffset;
			unsigned char* tangentEnd = tangent + subMesh.numVertices * subMesh.vertexSize;
			while (tangent != tangentEnd)
			{
				*(Vector3*)tangent = *assimpTangent;
				tangent += subMesh.vertexSize;
				++assimpTangent;
			}
		}

		// Copy vertex bitangents if present
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Bitangent))
		{
			Vector3* assimpBitangent = reinterpret_cast<Vector3*>(assimpMesh->mBitangents);
			unsigned char* bitangent = vertices.get() + bitangentOffset;
			unsigned char* bitangentEnd = bitangent + subMesh.numVertices * subMesh.vertexSize;
			while (bitangent != bitangentEnd)
			{
				*(Vector3*)bitangent = *assimpBitangent;
				bitangent += subMesh.vertexSize;
				++assimpBitangent;
			}
		}

		// Copy texture coordinates (UVs) if present
		if (IsSet(subMesh.geometryTypes & GeometryTypes::UV))
		{
			if (assimpMesh->GetNumUVChannels() > 0 && assimpMesh->HasTextureCoords(0))
			{
				aiVector3D* assimpUV = assimpMesh->mTextureCoords[0];
				unsigned char* uv = vertices.get() + uvOffset;
				unsigned char* uvEnd = uv + subMesh.numVertices * subMesh.vertexSize;
				while (uv != uvEnd)
				{
					*(Vector2*)uv = { assimpUV->x, assimpUV->y };
					uv += subMesh.vertexSize;
					++assimpUV;
				}
			}
		}

		// Copy vertex colours if present
		if (IsSet(subMesh.geometryTypes & GeometryTypes::Colour))
		{
			if (assimpMesh->GetNumColorChannels() > 0 && assimpMesh->HasVertexColors(0))
			{
				aiColor4D* assimpColour = assimpMesh->mColors[0];
				unsigned char* colour = vertices.get() + colourOffset;
				unsigned char* colourEnd = colour + subMesh.numVertices * subMesh.vertexSize;
				while (colour != colourEnd)
				{
					*(ColourRGBA*)colour = { assimpColour->r, assimpColour->g, assimpColour->b, assimpColour->a };
					colour += subMesh.vertexSize;
					++assimpColour;
				}
			}
		}

		// TODO
		//if (mHasBones)
		//{
		//	if (assimpMesh->HasBones())
		//	{
		//		// Set all bones and weights to 0 to start with
		//		unsigned char* bones = vertices.get() + bonesOffset;
		//		unsigned char* bonesEnd = bones + subMesh.numVertices * subMesh.vertexSize;
		//		while (bones != bonesEnd)
		//		{
		//			memset(bones, 0, 20);
		//			bones += subMesh.vertexSize;
		//		}

		//		for (auto& node : mNodes)
		//		{
		//			node.offsetMatrix = Matrix4x4::Identity;
		//		}

		//		// Go through each assimp bone
		//		bones = vertices.get() + bonesOffset;
		//		for (unsigned int i = 0; i < assimpMesh->mNumBones; ++i)
		//		{
		//			// Get offset matrix for the bone (transform from skinned mesh root to bone root)
		//			aiBone* assimpBone = assimpMesh->mBones[i];
		//			std::string boneName = assimpBone->mName.C_Str();
		//			unsigned int nodeIndex;
		//			for (nodeIndex = 0; nodeIndex < mNodes.size(); ++nodeIndex)
		//			{
		//				if (mNodes[nodeIndex].name == boneName)
		//				{
		//					mNodes[nodeIndex].offsetMatrix = Matrix4x4(&assimpBone->mOffsetMatrix.a1);
		//					mNodes[nodeIndex].offsetMatrix.Transpose(); // Assimp stores matrices differently to this app
		//					break;
		//				}
		//			}
		//			if (nodeIndex == mNodes.size())  throw std::runtime_error("Bone with no matching node in " + mFilepath.string());

		//			// Go through each weight of the bone and update the vertex it influences
		//			// Find the first 0 weight on that vertex and put the new influence / weight there.
		//			// A vertex can only have up to 4 influences
		//			for (unsigned int j = 0; j < assimpBone->mNumWeights; ++j)
		//			{
		//				unsigned int vertexIndex = assimpBone->mWeights[j].mVertexId;
		//				unsigned char* bone = bones + vertexIndex * subMesh.vertexSize;
		//				float* weight = (float*)(bone + 4);
		//				float* lastWeight = weight + 3;
		//				while (*weight != 0.0f && weight != lastWeight)
		//				{
		//					bone++; weight++;
		//				}
		//				if (*weight == 0.0f)
		//				{
		//					*bone = static_cast<uint8_t>(nodeIndex);
		//					*weight = assimpBone->mWeights[j].mWeight;
		//				}
		//			}
		//		}
		//	}
		//	else
		//	{
		//		// In a mesh that uses skinning any sub-meshes that don't contain bones are given bones so the whole mesh can use one shader
		//		unsigned int subMeshNode = 0;
		//		for (unsigned int nodeIndex = 0; nodeIndex < mNodes.size(); ++nodeIndex)
		//		{
		//			for (auto& subMeshIndex : mNodes[nodeIndex].subMeshes)
		//			{
		//				if (subMeshIndex == m)
		//					subMeshNode = nodeIndex;
		//			}
		//		}

		//		unsigned char* bones = vertices.get() + bonesOffset;
		//		unsigned char* bonesEnd = bones + subMesh.numVertices * subMesh.vertexSize;
		//		while (bones != bonesEnd)
		//		{
		//			memset(bones, 0, 20);
		//			bones[0] = static_cast<uint8_t>(subMeshNode);
		//			*(float*)(bones + 4) = 1.0f;
		//			bones += subMesh.vertexSize;
		//		}
		//	}
		//}



		//-----------------------------------
		// With the vertex data copied, we now create index data to indicate how to join up the vertices into trianlges
		// This is called face data in assimp. Assimp will always have face data and we will always use index buffers

		if (!assimpMesh->HasFaces())  throw std::runtime_error("No face data in " + subMesh.name + " in " + mFilepath.string());

		// Copy assimp faces to our index array
		uint32_t* index = reinterpret_cast<uint32_t*>(indices.get()); // Will always use 4 byte indexes here (2-byte indexes are possible for some memory savings)
		for (unsigned int face = 0; face < assimpMesh->mNumFaces; ++face)
		{
			*index++ = assimpMesh->mFaces[face].mIndices[0];
			*index++ = assimpMesh->mFaces[face].mIndices[1];
			*index++ = assimpMesh->mFaces[face].mIndices[2];
		}


		//-----------------------------------
		// Finally, with CPU-side vertex and index arrays complete for this submesh, create a matching DirectX
		// vertex and index buffer GPU-side. Then loop for all other submeshes

		D3D11_BUFFER_DESC bufferDesc    = {};
		D3D11_SUBRESOURCE_DATA initData = {};

		// Create GPU-side vertex buffer and copy the vertices imported by assimp into it
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Indicate it is a vertex buffer
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = subMesh.numVertices * subMesh.vertexSize; // Size of the buffer in bytes
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		initData.pSysMem = vertices.get(); // Fill the new vertex buffer with CPU-side vertex data we created above

		hr = DX->Device()->CreateBuffer(&bufferDesc, &initData, &subMesh.vertexBuffer);
		if (FAILED(hr))  throw std::runtime_error("Failure creating vertex buffer for " + mFilepath.string());


		// Create GPU-side index buffer and copy the vertices imported by assimp into it
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER; // Indicate it is an index buffer
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = subMesh.numIndices * sizeof(uint32_t); // Size of the buffer in bytes, will always use 4 byte indexes here
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		initData.pSysMem = indices.get(); // Fill the new index buffer with CPU-side index data we created above

		hr = DX->Device()->CreateBuffer(&bufferDesc, &initData, &subMesh.indexBuffer);
		if (FAILED(hr))  throw std::runtime_error("Failure creating index buffer for " + mFilepath.string());
	}
}



//--------------------------------------------------------------------------------------
// Special Mesh Construction
//--------------------------------------------------------------------------------------

// Special mesh constructor to creates a grid mesh without needing a file
// Create a grid in the XZ plane from minPt to maxPt with the given number of subdivisions in X and Z. 
// Optionally select whether to create normals (upwards), and/or UVs (0->1 square over the entire grid)
// If UVs requested optionally indicate how many repeats of the texture are required in X and Z (defaults to 1)
Mesh::Mesh(Vector3 minPt, Vector3 maxPt, int subDivX, int subDivZ, bool normals /*= false*/, bool uvs /*= true*/, float uvRepeatX /*= 1*/, float uvRepeatZ /*= 1*/)
	: mNodes{ { "Grid", Matrix4x4::Identity, Matrix4x4::Identity, 0, 1, {0}, {} } },  // Create a single root node, using one submesh and no children
	  mSubMeshes{ 1 }, mMaxNodeDepth{ 1 }, mHasBones{ false }, mFilepath{}
{
	mSubMeshes[0].nodeIndex = 0;
	mSubMeshes[0].name = "Grid0";
	mSubMeshes[0].materialName = "";


	//-----------------------------------
	// Create bespoke RenderMethod/RenderState

	RenderMethod renderMethod;
	renderMethod.geometryRenderMethod = GeometryRenderMethod::Rigid;
	renderMethod.surfaceRenderMethod  = SurfaceRenderMethod::BlinnTexture;
	renderMethod.textures.push_back({ TextureType::Diffuse, "Media/Water_Diffuse.png", {TextureFilter::FilterAnisotropic, TextureAddressingMode::AddressingWrap } });
	renderMethod.constants.diffuseColour = { 0.8f, 0.8f, 0.8f };
	renderMethod.constants.specularColour = { 0.2f, 0.2f, 0.2f };
	renderMethod.constants.specularPower = 25;
	renderMethod.constants.parallaxDepth = 0.05f;

	// Create DirectX objects (shaders, texture objects etc.) to match the render method we have identified
	// Can throw std::runtime_error
	try
	{
		mSubMeshes[0].renderState = std::make_unique<RenderState>(renderMethod);
	}
	catch (std::runtime_error e)
	{
		throw std::runtime_error("Grid Mesh: cannot create render state - " + std::string(e.what()));
	}


	//-----------------------------------
	// Create vertex layout based on requested data
	
	// Determine vertex layout based on parameters
	std::vector<D3D11_INPUT_ELEMENT_DESC> vertexElements;
	unsigned int offset = 0;

	unsigned int positionOffset = offset;
	vertexElements.push_back({ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, positionOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	offset += 12;
	mSubMeshes[0].geometryTypes = GeometryTypes::Position;

	unsigned int normalOffset = offset;
	if (normals)
	{
		vertexElements.push_back({ "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, normalOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		offset += 12;
		mSubMeshes[0].geometryTypes |= GeometryTypes::Normal;
	}

	unsigned int uvOffset = offset;
	if (uvs)
	{
		vertexElements.push_back({ "uv", 0, DXGI_FORMAT_R32G32_FLOAT, 0, uvOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		offset += 8;
		mSubMeshes[0].geometryTypes |= GeometryTypes::UV;
	}

	mSubMeshes[0].vertexSize = offset;

	// Create a vertex layout object from above array - used by DirectX to understand the data in each vertex of this mesh
	auto shaderSignature = CreateSignatureForVertexLayout(vertexElements.data(), static_cast<int>(vertexElements.size()));
	HRESULT hr = DX->Device()->CreateInputLayout(vertexElements.data(), static_cast<UINT>(vertexElements.size()),
		                                         shaderSignature->GetBufferPointer(), shaderSignature->GetBufferSize(),	&mSubMeshes[0].vertexLayout);
	if (shaderSignature)  shaderSignature->Release();
	if (FAILED(hr))  throw std::runtime_error("Failure creating input layout for grid mesh");


	//-----------------------------------
	// Create grid

	// Allocate space to create the grid vertices (CPU-side first)
	mSubMeshes[0].numVertices = (subDivX + 1) * (subDivZ + 1);
	auto vertexData = std::make_unique<char[]>(mSubMeshes[0].numVertices * mSubMeshes[0].vertexSize); // Smart pointer
  
	// Create the grid vertices (CPU-side), to be passed to the GPU afterwards
	float xStep = (maxPt.x - minPt.x) / subDivX; // X-size of a single grid square
	float zStep = (maxPt.z - minPt.z) / subDivZ; // Z-size of a single grid square
	float uStep = uvRepeatX / subDivX;                // U-size of a single grid square
	float vStep = uvRepeatZ / subDivZ;                // V-size of a single grid square
	Vector3 pt  = minPt;                         // Start position at bottom-left of grid (looking down on it)
	Vector3 normal = { 0, 1, 0 };                // All normals will be up (useful to make grid use same data as ordinary models so it can use the same shaders)
	Vector2 uv     = { 0, 1 };                   // UVs also start at bottom-left (V axis is opposite direction to Z)
	// A 2D array of data, only complexity is that some data is optional. So byte-offsets and pointer casting is needed
	auto currVert = vertexData.get();
	for (int z = 0; z <= subDivZ; ++z)
	{
		for (int x = 0; x <= subDivX; ++x)
		{
			*reinterpret_cast<Vector3*>(currVert) = pt;
			currVert += sizeof(Vector3);
			if (normals)
			{
				*reinterpret_cast<Vector3*>(currVert) = normal;
				currVert += sizeof(Vector3);
			}
			if (uvs)
			{
				*reinterpret_cast<Vector2*>(currVert) = uv;
				currVert += sizeof(Vector2);
			}
			pt.x += xStep;
			uv.x += uStep;
		}
		pt.x = minPt.x;
		pt.z += zStep;
		uv.x = 0;
		uv.y -= vStep; // V axis is opposite direction to Z
	}


	// Allocate space to create the grid indices. To keep model rendering code simpler using a triangle
	// list, even though a strip would work nicely here
	mSubMeshes[0].numIndices = subDivX * subDivZ * 6; // Two triangles for each grid square
	auto indexData = std::make_unique<char[]>(mSubMeshes[0].numIndices * 4); // 4 byte integer for each index

	// Create the grid indexes (CPU-side first)
	uint32_t tlIndex = 0;
	auto currIndex = reinterpret_cast<uint32_t*>(indexData.get()); // uint32_t = 4-byte indexes
	for (int z = 0; z < subDivZ; ++z)
	{
		for (int x = 0; x < subDivX; ++x)
		{
			// Bottom-left triangle in grid square (looking down on the grid)
			*currIndex++ = tlIndex;
			*currIndex++ = tlIndex + subDivX + 1;
			*currIndex++ = tlIndex + 1;

			// Top-right triangle in grid square
			*currIndex++ = tlIndex + 1;
			*currIndex++ = tlIndex + subDivX + 1;
			*currIndex++ = tlIndex + subDivX + 2;

			++tlIndex;
		}
		++tlIndex;
	}

  
	//-----------------------------------
	// Create vertex / index buffer

	// Create the vertex buffer and fill it with the loaded vertex data
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT; // Not a dynamic buffer
	bufferDesc.ByteWidth = mSubMeshes[0].numVertices * mSubMeshes[0].vertexSize; // Buffer size
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initData = {}; // Initial data
	initData.pSysMem = vertexData.get();   
	if (FAILED(DX->Device()->CreateBuffer( &bufferDesc, &initData, &mSubMeshes[0].vertexBuffer)))
	{
		throw std::runtime_error("Failure creating vertex buffer for grid mesh");
	}


	// Create the index buffer
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = mSubMeshes[0].numIndices * 4;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	initData.pSysMem = indexData.get();   
	if (FAILED(DX->Device()->CreateBuffer( &bufferDesc, &initData, &mSubMeshes[0].indexBuffer)))
	{
		throw std::runtime_error("Failure creating index buffer for grid mesh");
	}
}



//--------------------------------------------------------------------------------------
// Mesh Rendering
//--------------------------------------------------------------------------------------

// Helper function for Render function - renders the given sub-mesh. Material RenderState mu
void Mesh::RenderSubMesh(const SubMesh& subMesh)
{
	subMesh.renderState->Apply();

	// Set vertex buffer as next data source for GPU
	UINT stride = subMesh.vertexSize;
	UINT offset = 0;
	DX->Context()->IASetVertexBuffers(0, 1, &subMesh.vertexBuffer.p, &stride, &offset);

	// Indicate the layout of vertex buffer
	DX->Context()->IASetInputLayout(subMesh.vertexLayout);

	// Set index buffer as next data source for GPU, indicate it uses 32-bit integers
	DX->Context()->IASetIndexBuffer(subMesh.indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Using triangle lists only in this class
	DX->Context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Render mesh
	DX->Context()->DrawIndexed(subMesh.numIndices, 0, 0);
}

// Calculate the absolute matrix for given node given a set of mesh transforms 
Matrix4x4 Mesh::AbsoluteMatrix(const std::vector<Matrix4x4>& transforms, unsigned int node)
{
	// Mesh transformation matrices in child nodes are stored relative to their parent's matrix (recall animation in 2nd year Graphics)
	mAbsoluteTransforms[0] = transforms[0]; // First matrix for a model is the root matrix, already in world space
	for (unsigned int nodeIndex = 1; nodeIndex < mNodes.size(); ++nodeIndex)
	{
		// Multiply each model matrix by its parent's absolute world matrix
		mAbsoluteTransforms[nodeIndex] = transforms[nodeIndex] * mAbsoluteTransforms[mNodes[nodeIndex].parentIndex];
	}
	return mAbsoluteTransforms[node];
}


// Render the mesh with the given matrices. If no matrices are provided, the mesh default transforms will be used
// May optionally provide a colour, which will be available to the shaders in the per-mesh constant buffer
// Handles rigid body meshes (including single part meshes) as well as skinned meshes
void Mesh::Render(const std::vector<Matrix4x4>& transforms /*= {}*/, ColourRGBA colour /*= { 1, 1, 1, 1 }*/)
{
	gPerMeshConstants.meshColour = colour;

	// No matrices - static mesh
	if (transforms.size() == 0)
	{
		// World matrix is always identity matrix
		gPerMeshConstants.worldMatrix = Matrix4x4::Identity;
		DX->CBuffers()->UpdateCBuffer(gPerMeshConstantBuffer, gPerMeshConstants); // Send to GPU

		// Render submeshes directly, no need to consider nodes
		for (auto& subMesh : mSubMeshes)
			RenderSubMesh(subMesh);
		
		return;
	}

	// Mesh transformation matrices in child nodes are stored relative to their parent's matrix (recall animation in 2nd year Graphics)
	// This is good for animation but we need absolute world space matrices to render - so calculate all the absolute matrices first
	mAbsoluteTransforms[0] = transforms[0]; // First matrix for a model is the root matrix, already in world space
	for (unsigned int nodeIndex = 1; nodeIndex < mNodes.size(); ++nodeIndex)
	{
		// Multiply each model matrix by its parent's absolute world matrix
		mAbsoluteTransforms[nodeIndex] = transforms[nodeIndex] * mAbsoluteTransforms[mNodes[nodeIndex].parentIndex];
	}

	if (mHasBones) // Render a mesh that uses skinning
	{
		// Advanced point: the above loop will get the absolute world matrices **of the bones**. However, they are
		// not actually rendered, they merely influence the skinned mesh, which has its origin at a particular node.
		// So for each bone there is a fixed offset (transform) between where that bone is and where the root of the
		// skinned mesh is. We need to apply that offset to each of the bone matrices calculated in the last loop to make
		// the bone influences work on the skinned mesh.
		// These offset matrices are fixed for the model and were already calculated when the mesh was imported
		for (unsigned int nodeIndex = 0; nodeIndex < mNodes.size(); ++nodeIndex)
			mAbsoluteTransforms[nodeIndex] = mNodes[nodeIndex].offsetMatrix * mAbsoluteTransforms[nodeIndex];

		// Send all matrices over to the GPU for skinning via a constant buffer - each matrix can represent a bone which influences nearby vertices
		for (unsigned int nodeIndex = 0; nodeIndex < mNodes.size(); ++nodeIndex)
			gPerMeshConstants.boneMatrices[nodeIndex] = mAbsoluteTransforms[nodeIndex];

		DX->CBuffers()->UpdateCBuffer(gPerMeshConstantBuffer, gPerMeshConstants); // Send to GPU

		// All matrices for the entire mesh have been sent over to the GPU which makes this loop simple - we can
		// render all submeshes directly and do not need to iterate through the nodes (unlike non-skinning code below)
		for (auto& subMesh : mSubMeshes)
			RenderSubMesh(subMesh);
	}
	else
	{
		// Render a mesh without skinning. First iterate through each node
		for (unsigned int nodeIndex = 0; nodeIndex < mNodes.size(); ++nodeIndex)
		{
			// Send this node's matrix to the GPU via a constant buffer
			gPerMeshConstants.worldMatrix = mAbsoluteTransforms[nodeIndex];
			DX->CBuffers()->UpdateCBuffer(gPerMeshConstantBuffer, gPerMeshConstants); // Send to GPU

			// Render the sub-meshes attached to this node (no bones - rigid movement)
			for (auto& subMeshIndex : mNodes[nodeIndex].subMeshes)
				RenderSubMesh(mSubMeshes[subMeshIndex]);
		}
	}
}


//--------------------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------------------

// Read assimp node and its children and place in mNodes vector at position nodeIndex. Optionally filter out nodes with no submeshes (filterEmpty)
// Recursive function, first call only needs first two parameters. Returns first nodeIndex into mNodes after the nodes inserted
unsigned int Mesh::ReadNodes(aiNode* assimpNode, bool filterEmpty, unsigned int nodeIndex /*= 0*/, unsigned int parentIndex /*= 0*/,
	                         unsigned int depth /*= 1*/, Matrix4x4 filteredTransform /*= Matrix4x4::Identity*/)
{
	if (depth > mMaxNodeDepth)  mMaxNodeDepth = depth;

	if (assimpNode->mNumMeshes == 0 && depth > 1 && filterEmpty) // Root node cannot be removed even if empty
	{
		// Found a node with no meshes, if filtering these out then pass its transform to its children without creating a new node.
		filteredTransform *= Matrix4x4(&assimpNode->mTransformation.a1).Transpose();

		for (unsigned int child = 0; child < assimpNode->mNumChildren; ++child)
			nodeIndex = ReadNodes(assimpNode->mChildren[child], filterEmpty, nodeIndex, parentIndex, depth, filteredTransform);
	}
	else
	{
		auto& node = mNodes[nodeIndex];
		unsigned int thisIndex = nodeIndex;
		++nodeIndex;

		node.name = assimpNode->mName.C_Str(); // Note: in UTF-8 format
		node.parentIndex = parentIndex;
		node.depth = depth;

		// Include any transforms passed in from filtered parent nodes
		node.transform = filteredTransform * Matrix4x4(&assimpNode->mTransformation.a1).Transpose();

		// Reference meshes controlled by this node
		node.subMeshes.resize(assimpNode->mNumMeshes);
		for (unsigned int i = 0; i < assimpNode->mNumMeshes; ++i)
		{
			node.subMeshes[i] = assimpNode->mMeshes[i];
		}

		node.children.resize(assimpNode->mNumChildren);
		for (unsigned int i = 0; i < assimpNode->mNumChildren; ++i)
		{
			node.children[i] = nodeIndex;
			nodeIndex = ReadNodes(assimpNode->mChildren[i], filterEmpty, nodeIndex, thisIndex, depth + 1, Matrix4x4::Identity);
		}
	}
	return nodeIndex;
}
