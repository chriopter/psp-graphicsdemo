/* Projected shadow map - after the pspsdk "shadowprojection" sample. */
#include "demo.h"
#include "geometry.h"

#define GRID_COLUMNS 32
#define GRID_ROWS 32
#define GRID_SIZE 10.0f
#define TORUS_SLICES 48
#define TORUS_ROWS 48
#define LIGHT_DISTANCE 3.0f

static NPVertex __attribute__((aligned(16))) grid_vertices[GRID_COLUMNS*GRID_ROWS];
static unsigned short __attribute__((aligned(16))) grid_indices[(GRID_COLUMNS-1)*(GRID_ROWS-1)*6];
static NPVertex __attribute__((aligned(16))) torus_vertices[TORUS_SLICES*TORUS_ROWS];
static unsigned short __attribute__((aligned(16))) torus_indices[TORUS_SLICES*TORUS_ROWS*6];

static ScePspFMatrix4 identity, projection, view, textureProjScaleTrans, lightProjection, lightProjectionInf;

#define VFMT (NP_VERTEX_FORMAT | GU_INDEX_16BIT | GU_TRANSFORM_3D)

static void init(void)
{
	generateGridNP(GRID_COLUMNS, GRID_ROWS, GRID_SIZE, GRID_SIZE, grid_vertices, grid_indices);
	generateTorusNP(TORUS_ROWS, TORUS_SLICES, 1.0f, 0.5f, torus_vertices, torus_indices);

	gumLoadIdentity(&identity);
	gumLoadIdentity(&projection);
	gumPerspective(&projection, 75.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	{
		ScePspFVector3 pos = { 0, 0, -5.0f };
		gumLoadIdentity(&view);
		gumTranslate(&view, &pos);
	}
	/* clip space -> texture space */
	gumLoadIdentity(&textureProjScaleTrans);
	textureProjScaleTrans.x.x = 0.5f;
	textureProjScaleTrans.y.y = -0.5f;
	textureProjScaleTrans.w.x = 0.5f;
	textureProjScaleTrans.w.y = 0.5f;
	gumLoadIdentity(&lightProjection);
	gumPerspective(&lightProjection, 75.0f, 1.0f, 0.1f, 1000.0f);
	gumLoadIdentity(&lightProjectionInf);
	gumPerspective(&lightProjectionInf, 75.0f, 1.0f, 0.0f, 1000.0f);
}

static void draw(int frame)
{
	ScePspFMatrix4 gridWorld, torusWorld, lightMatrix, lightView, shadowProj;

	{
		ScePspFVector3 pos = { 0, -1.5f, 0 };
		gumLoadIdentity(&gridWorld);
		gumTranslate(&gridWorld, &pos);
	}
	{
		ScePspFVector3 pos = { 0, 0.5f, 0.0f };
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f), deg(frame * 1.32f) };
		gumLoadIdentity(&torusWorld);
		gumTranslate(&torusWorld, &pos);
		gumRotateXYZ(&torusWorld, &rot);
	}
	{
		/* the light circles the torus and looks down at it */
		ScePspFVector3 lookAt = { torusWorld.w.x, torusWorld.w.y, torusWorld.w.z };
		ScePspFVector3 rot1 = { 0, deg(frame * 0.79f), 0 };
		ScePspFVector3 rot2 = { -deg(60.0f), 0, 0 };
		ScePspFVector3 pos = { 0, 0, LIGHT_DISTANCE };
		gumLoadIdentity(&lightMatrix);
		gumTranslate(&lightMatrix, &lookAt);
		gumRotateXYZ(&lightMatrix, &rot1);
		gumRotateXYZ(&lightMatrix, &rot2);
		gumTranslate(&lightMatrix, &pos);
	}
	gumFastInverse(&lightView, &lightMatrix);

	/* pass 1: the torus as seen from the light, black on white */
	demo_target_begin();
	sceGuClearColor(0xffffffff);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuSetMatrix(GU_PROJECTION, &lightProjection);
	sceGuSetMatrix(GU_VIEW, &lightView);
	sceGuSetMatrix(GU_MODEL, &torusWorld);
	sceGuColor(0x00000000);
	sceGuDrawArray(GU_TRIANGLES, VFMT, sizeof(torus_indices)/sizeof(unsigned short), torus_indices, torus_vertices);

	/* pass 2: the scene, the floor modulated by the projected map */
	demo_target_end();
	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuEnable(GU_DITHER);
	sceGuSetMatrix(GU_PROJECTION, &projection);
	sceGuSetMatrix(GU_VIEW, &view);
	{
		ScePspFVector3 lightPos = { lightMatrix.w.x, lightMatrix.w.y, lightMatrix.w.z };
		ScePspFVector3 lightDir = { lightMatrix.z.x, lightMatrix.z.y, lightMatrix.z.z };
		sceGuLight(0, GU_SPOTLIGHT, GU_DIFFUSE, &lightPos);
		sceGuLightSpot(0, &lightDir, 5.0f, 0.6f);
		sceGuLightColor(0, GU_DIFFUSE, 0x00ff4040);
		sceGuLightAtt(0, 1.0f, 0.0f, 0.0f);
		sceGuAmbient(0x00202020);
		sceGuEnable(GU_LIGHTING);
		sceGuEnable(GU_LIGHT0);
	}
	sceGuSetMatrix(GU_MODEL, &torusWorld);
	sceGuColor(0xffffff);
	sceGuDrawArray(GU_TRIANGLES, VFMT, sizeof(torus_indices)/sizeof(unsigned short), torus_indices, torus_vertices);

	sceGuTexMapMode(GU_TEXTURE_MATRIX, 0, 0);
	sceGuTexProjMapMode(GU_POSITION);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, RT_SIZE, RT_SIZE, RT_SIZE, sceGeEdramGetAddr() + VRAM_RT);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	sceGuEnable(GU_TEXTURE_2D);

	gumMultMatrix(&shadowProj, &lightProjectionInf, &lightView);
	gumMultMatrix(&shadowProj, &textureProjScaleTrans, &shadowProj);
	gumMultMatrix(&shadowProj, &shadowProj, &gridWorld);
	sceGuSetMatrix(GU_MODEL, &gridWorld);
	sceGuSetMatrix(GU_TEXTURE, &shadowProj);
	sceGuColor(0xff7777);
	sceGuDrawArray(GU_TRIANGLES, VFMT, sizeof(grid_indices)/sizeof(unsigned short), grid_indices, grid_vertices);

	/* leave gum's cached matrices dirty, the next scene reloads them */
	sceGumMatrixMode(GU_PROJECTION); sceGumLoadIdentity();
	sceGumMatrixMode(GU_VIEW); sceGumLoadIdentity();
	sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity();
}

const Scene scene_shadow = { "PROJECTED SHADOW", "shadow map projected via GU_TEXTURE_MATRIX", init, draw };
