///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	bool bReturn = false;
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/d6z6f7n-d0432750-413e-43dd-9c71-94851bd5de38.jpg", "floor");

	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/dark_brown_leather_texture__tileable___2048x2048__by_fabooguy_d7aect6-pre.jpg", "kickstand"
	);
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/Beveled_top_edge_red_textured_cast_finish_dusty_grubby_rough_flecked_metal_sheet_surface_texture.jpg", "kickbag"
	);

	//Create dumbell_steel texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/dumbell_steel_texture.jpg", "dumbell_steel"
	);
	//Create dumbell_orange texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/orange_dumbell_texture.jpg", "dumbell_orange"
	);

	//Create dumbell_pink texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/pink_dumbell_texture.jpg", "dumbell_pink"
	);

	//Create dumbell_grey texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/grey_dumbell_texture.jpg", "dumbell_grey"
	);


	//Create wall texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/texture2.jpg", "wall"
	);

	//Create window texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/glass_window_texture.jpg", "window"
	);

	//Create window wall texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/window_wall_texture.jpg", "window_wall"
	);

	//Create tv texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/TV_Texture.jpg", "tv"
	);

	//Create dumbell_steel_white texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/dumbell_steel_white_texture.jpg", "dumbell_steel_white"
	);

	//Create doors texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/door_texture.jpg", "door"
	);

	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/double_door_texture.jpg", "double_doors"
	);
	//Create weight_mats texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/texture1.jpg", "weight_mats"
	);
	//Create wall_mat texture
	bReturn = CreateGLTexture(
		"C:/Users/coope/Downloads/wall_mat_texture.jpg", "wall_mat"
	);

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}


/*********************************************
* DefineObjectMaterials()
*
This method is used to define the objects material
for lighting, and how said objects texture would reflect
or look when lit
**********************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL redKickMaterial;
	redKickMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f); //Low ambient color
	redKickMaterial.ambientStrength = 0.4f; //low ambient strength
	redKickMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f); //Low diffuse color
	redKickMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f); //High specular color
	redKickMaterial.shininess = 22.0; //Lower shininess
	redKickMaterial.tag = "redKick";
	m_objectMaterials.push_back(redKickMaterial);

	OBJECT_MATERIAL KickStandMaterial;
	KickStandMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f); //Low ambient color
	KickStandMaterial.ambientStrength = 0.4f; //low ambient strength
	KickStandMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f); //Low diffuse color
	KickStandMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f); //High specular color
	KickStandMaterial.shininess = 22.0; //Lower shininess
	KickStandMaterial.tag = "KickStand";
	m_objectMaterials.push_back(KickStandMaterial);

	OBJECT_MATERIAL MetalDumbbellMaterial;
	MetalDumbbellMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Low ambient color
	MetalDumbbellMaterial.ambientStrength = 0.2f; // Low ambient strength
	MetalDumbbellMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f); // Low diffuse color
	MetalDumbbellMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f); // High specular color
	MetalDumbbellMaterial.shininess = 32.0; // Moderate shininess
	MetalDumbbellMaterial.tag = "darkMetallicDumbbell";
	m_objectMaterials.push_back(MetalDumbbellMaterial);

	OBJECT_MATERIAL TVScreenMaterial;
	TVScreenMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f); // No ambient light contribution
	TVScreenMaterial.ambientStrength = 0.0f; // Ambient strength set to 0
	TVScreenMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f); // No diffuse reflection
	TVScreenMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); // No specular highlights
	TVScreenMaterial.shininess = 0.0; // No shininess
	TVScreenMaterial.tag = "blackTVScreen";
	m_objectMaterials.push_back(TVScreenMaterial);

	OBJECT_MATERIAL MirrorMaterial;
	MirrorMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f); // No ambient light contribution
	MirrorMaterial.ambientStrength = 0.0f; // Ambient strength set to 0
	MirrorMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f); // No diffuse reflection
	MirrorMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // Full white specular reflection
	MirrorMaterial.shininess = 100.0; // High shininess for sharp reflections
	MirrorMaterial.tag = "glassMirror";
	m_objectMaterials.push_back(MirrorMaterial);

	OBJECT_MATERIAL WhiteWallMaterial;
	WhiteWallMaterial.ambientColor = glm::vec3(0.8f, 0.8f, 0.8f); // Ambient color for soft ambient light
	WhiteWallMaterial.ambientStrength = 0.2f; // Low ambient strength for subtle ambient light
	WhiteWallMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // Diffuse color for light reflection
	WhiteWallMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); // No specular highlights
	WhiteWallMaterial.shininess = 0.0; // No shininess
	WhiteWallMaterial.tag = "whiteWall";
	m_objectMaterials.push_back(WhiteWallMaterial);

	OBJECT_MATERIAL WhiteHardwoodMaterial;
	WhiteHardwoodMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f); // Ambient color for soft ambient light
	WhiteHardwoodMaterial.ambientStrength = 0.3f; // Low ambient strength for subtle ambient light
	WhiteHardwoodMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // Diffuse color for light reflection
	WhiteHardwoodMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Specular color for sheen effect
	WhiteHardwoodMaterial.shininess = 25.0; // Medium shininess for a slight reflective sheen
	WhiteHardwoodMaterial.tag = "whiteHardwoodFloor";
	m_objectMaterials.push_back(WhiteHardwoodMaterial);

	OBJECT_MATERIAL RubberDumbbellMaterial;
	RubberDumbbellMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Ambient color for soft ambient light
	RubberDumbbellMaterial.ambientStrength = 0.2f; // Low ambient strength for subtle ambient light
	RubberDumbbellMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); // Diffuse color for light reflection
	RubberDumbbellMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); // No specular highlights
	RubberDumbbellMaterial.shininess = 0.0; // No shininess
	RubberDumbbellMaterial.tag = "rubberDumbbell";
	m_objectMaterials.push_back(RubberDumbbellMaterial);

	OBJECT_MATERIAL FloorMatMaterial;
	FloorMatMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Ambient color for soft ambient light
	FloorMatMaterial.ambientStrength = 0.2f; // Low ambient strength for subtle ambient light
	FloorMatMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f); // Diffuse color for light reflection
	FloorMatMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); // No specular highlights
	FloorMatMaterial.shininess = 0.0; // No shininess
	FloorMatMaterial.tag = "floorMat";
	m_objectMaterials.push_back(FloorMatMaterial);

	OBJECT_MATERIAL WhiteDoorMaterial;
	WhiteDoorMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // Ambient color for soft ambient light
	WhiteDoorMaterial.ambientStrength = 0.3f; // Low ambient strength for subtle ambient light
	WhiteDoorMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // Diffuse color for light reflection
	WhiteDoorMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Specular color for slight sheen effect
	WhiteDoorMaterial.shininess = 10.0; // Medium shininess for being slight reflective
	WhiteDoorMaterial.tag = "whiteDoor";
	m_objectMaterials.push_back(WhiteDoorMaterial);

	OBJECT_MATERIAL WindowMaterial;
	WindowMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // Ambient color for soft ambient light
	WindowMaterial.ambientStrength = 0.1f; // Low ambient strength for subtle ambient light
	WindowMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // Diffuse color for light reflection
	WindowMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); // No specular highlights
	WindowMaterial.shininess = 0.0; // No shininess
	WindowMaterial.tag = "Window";
	m_objectMaterials.push_back(WindowMaterial);
}


/*********************************************
* SetupSceneLights()
* 
This method is used to setup the lights
in the scene
**********************************************/

void SceneManager::SetupSceneLights()
{


	// Light 1
	m_pShaderManager->setVec3Value("lightSources[0].position", -14.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.5f, 0.7f, 0.7f); 
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.5f, 0.7f, 0.7f); 
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.5f, 0.7f, 0.7f); 
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 18.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Light 2
	m_pShaderManager->setVec3Value("lightSources[1].position", 14.0f, 5.0f, 14.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 18.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);


	// Adjusting for cool white lighting
	const float coolWhiteIntensity = 0.8f; // Intensity for cool white lighting

	// Adjusting light color temperature towards cooler end of the spectrum (blue-white)
	const float coolWhiteR = 0.7f; // Red component
	const float coolWhiteG = 0.75f; // Green component
	const float coolWhiteB = 0.85f; // Blue component

	// Applying cool white lighting to both light sources
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", coolWhiteR, coolWhiteG, coolWhiteB);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", coolWhiteR, coolWhiteG, coolWhiteB);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", coolWhiteR, coolWhiteG, coolWhiteB);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", coolWhiteIntensity);

	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", coolWhiteR, coolWhiteG, coolWhiteB);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", coolWhiteR, coolWhiteG, coolWhiteB);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", coolWhiteR, coolWhiteG, coolWhiteB);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", coolWhiteIntensity);


	m_pShaderManager->setBoolValue("bUseLighting", true);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	SetTextureUVScale(2.0f, 2.0f);

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	//Floors, walls, and ceilings

	//Ceiling
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 15.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Tilted Ceiling
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 1.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 15.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 1, 0.5, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Floor
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("floor");
	SetShaderMaterial("whiteHardwoodFloor");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	//Wall #1

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.0f, 14.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 7.5f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	//Wall #2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.0f, 7.5f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


//This is the little space between the 2 doors and wall
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.0f, 7.5f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Wall #3
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 7.5f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Slanted part of right wall
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -50.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 7.5f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	//Wall #4
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.6f, 0.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.86f, 7.5f, 7.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	/********************************************
	*****************Window**********************
	*********************************************
	*********************************************/

	//Window Pane
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 0.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.99f, 7.5f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("window");
	SetShaderMaterial("Window");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	//Window Frame Bottom
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 8.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.8f, 3.5f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("window_wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//Window Frame Top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 8.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.8f, 11.5f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("window_wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//Window Frame Middle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 8.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.8f, 7.5f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("window_wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//Window Frame Left
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 8.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.8f, 7.5f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("window_wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//Window Frame Right
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 8.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.8f, 7.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("window_wall");
	SetShaderMaterial("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	/*******************************************
	********************************************
	********************************************/

	//Closet Doors
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.0f, 6.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.0f, 6.5f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("double_doors");
	SetShaderMaterial("whiteDoor");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Small bit of wall above closet doors
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.0f, 14.8f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderTexture("whiteWall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	//weight mats #1
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 1.0f, 2.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.7f, 0.1f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.4, 1, 1);
	SetShaderTexture("weight_mats");
	SetShaderMaterial("floorMat");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	//weight mats #2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, 1.0f, 4.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.1f, -0.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.4, 1, 1);
	SetShaderTexture("weight_mats");
	SetShaderMaterial("floorMat");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	//weight mats #3
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.2f, 1.0f, 10.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.1f, -7.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.4, 1, 1);
	SetShaderTexture("weight_mats");
	SetShaderMaterial("floorMat");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/


	/*******************************************************************/
	/*This is for the kickboxing stand*/
	/*******************************************************************/
	//Base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 0, 1, 1);
	SetShaderTexture("kickstand");
	SetShaderMaterial("KickStand");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	
	
	//Cone

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 2.0f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 1.6f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0, 0, 1);
	SetShaderTexture("kickstand");
	SetShaderMaterial("KickStand");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	
	
	//Cyllander for kickbox stand

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 5.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 2.6f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 1, 0, 1);
	SetShaderTexture("kickbag");
	SetShaderMaterial("redKick");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/**********************************************
	***********************************************
	* This part is for one off things like the TV
	* And mirrors
	***********************************************/

	//Mirror #1

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 1.0f, 4.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 5.0f, -9.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 1, 1, 1);
	SetShaderTexture("window");
	SetShaderMaterial("glassMirror");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Mirror #2

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.2f, 1.0f, 4.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.9f, 5.0f, -4.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 1, 1, 1);
	SetShaderTexture("window");
	SetShaderMaterial("glassMirror");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	//TV
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 3.3f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.6f, 8.5f, -8.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("tv");
	SetShaderMaterial("blackTVScreen");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//Wall Mat
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 10.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.8f, 6.0f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("wall_mat");
	SetShaderMaterial("floorMat");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	/*************************************
	* ************Corner Weights**********
	* ************************************
	* ************************************
	* */
	//Biggest Weight
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.0f, 0.2f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.7, 0.7, 0.7, 1);
	SetShaderTexture("dumbell_steel_white");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	//Middle Weight
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.8f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.0f, 0.4f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1);
	SetShaderTexture("dumbell_steel_white");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	//Smallest Weight
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 0.6f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.0f, 0.6f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.3, 0.3, 1);
	SetShaderTexture("dumbell_steel_white");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	//Middle part of Smallest Weight
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.3f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.0f, 0.6f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.3, 0.3, 1);
	SetShaderTexture("dumbell_steel_white");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();



	/*************************************
	***************Kettlebells************************
	**************************************
	**************************************/

	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.4f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 1.0f, -0.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	//Weight
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.4f, -0.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("kettlebell_blue");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();


	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.4f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 1.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	//Weight
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.4f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("kettlebell_blue");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();


	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.4f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 1.0f, -3.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	//Weight
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.4f, -3.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("kettlebell_blue");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.4f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 1.0f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();


	//Weight
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.4f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("kettlebell_blue");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*************************************
	***************Dumbells************************
	**************************************
	**************************************/
	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.3f, 0.4f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.3f, 0.4f, 7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	
	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.3f, 0.4f, 9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.3f, 0.4f, 8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();






	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 0.4f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.3f, 0.4f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();




	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.3f, 0.4f, 3.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.3f, 0.4f, 3.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.2f, 0.4f, 3.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.3f, 0.4f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.3f, 0.4f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.2f, 0.4f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.3f, 0.4f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_pink");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.3f, 0.4f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_pink");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.2f, 0.4f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_pink");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.3f, 0.4f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_grey");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.3f, 0.4f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_grey");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.2f, 0.4f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_grey");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();




	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();




	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();




	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();





	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();




	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_steel");
	SetShaderMaterial("darkMetallicDumbbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	//Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Weights
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.3f, 0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.6f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.3f, 0.4f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);
	SetShaderTexture("dumbell_orange");
	SetShaderMaterial("rubberDumbell");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}
