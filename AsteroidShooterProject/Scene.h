#pragma once
// assimp include files. These three are usually needed.
#include "assimp.h"
#include "aiPostProcess.h"
#include "aiScene.h"
#include "GL/glut.h"
#include <IL/il.h>
#include <map>

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)
#define TRUE                1
#define FALSE               0

class Scene
{
private:
	const struct aiScene *scene = NULL;
	const struct aiNode* rootNode = NULL;
	struct aiVector3D scene_min, scene_max, scene_center;
	// images / texture
	std::map<std::string, GLuint*> textureIdMap;	// map image filenames to textureIds
	GLuint*		textureIds;	// pointer to texture Array
	const std::string basepath = "..\\AsteroidShooterProject\\Resources\\";
	int nodeId = 0;

	int loadasset(const char* path);
	void get_bounding_box(struct aiVector3D* min, struct aiVector3D* max);
	void get_bounding_box_for_node(const struct aiNode* nd,
		struct aiVector3D* min,
		struct aiVector3D* max,
		struct aiMatrix4x4* trafo);
	void apply_material(const struct aiMaterial *mtl);
	void Color4f(const struct aiColor4D *color);
	void set_float4(float f[4], float a, float b, float c, float d);
	void color4_to_float4(const struct aiColor4D *c, float f[4]);
	
public:
	Scene();
	Scene(const char* path);
	~Scene();
	int LoadGLTextures();
	void recursive_render(const struct aiNode* nd, float scale);
	bool inside(float try_x, float try_y, float try_z);
	const aiNode* getRootNode();
	aiVector3D getCenter();
	aiVector3D getMin();
	aiVector3D getMax();
};