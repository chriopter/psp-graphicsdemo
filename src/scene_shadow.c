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

/* how far the lamp hangs from the torus */
static const float distances[] = { LIGHT_DISTANCE, 2.0f, 4.5f, 6.0f };

typedef struct { float u, v; float x, y, z; } FlatVertex;

static float light_yaw, elevation, camera, zoom;
static int distance, show_map;

static void reset(void)
{
	light_yaw = 0.0f;
	elevation = deg(60.0f);
	camera = 0.0f;
	zoom = 1.0f;
	distance = 0;
	show_map = 0;
}

/* The shadow map itself, in the corner, so it is clear what gets projected. */
static void draw_map(void)
{
	FlatVertex* v = sceGuGetMemory(sizeof(FlatVertex) * 2);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDisable(GU_LIGHTING);
	sceGuTexMapMode(GU_TEXTURE_COORDS, 0, 0);
	sceGuTexProjMapMode(GU_UV);
	sceGuSetMatrix(GU_TEXTURE, &identity);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	v[0].u = 0;       v[0].v = 0;       v[0].x = 8;   v[0].y = 34;  v[0].z = 0;
	v[1].u = RT_SIZE; v[1].v = RT_SIZE; v[1].x = 104; v[1].y = 130; v[1].z = 0;
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, v);
	sceGuEnable(GU_DEPTH_TEST);
}

static void init(void)
{
	generateGridNP(GRID_COLUMNS, GRID_ROWS, GRID_SIZE, GRID_SIZE, grid_vertices, grid_indices);
	generateTorusNP(TORUS_ROWS, TORUS_SLICES, 1.0f, 0.5f, torus_vertices, torus_indices);

	gumLoadIdentity(&identity);
	gumLoadIdentity(&projection);
	gumPerspective(&projection, 75.0f, 16.0f/9.0f, 0.5f, 1000.0f);
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

static void draw(int frame, const DemoInput* in)
{
	ScePspFMatrix4 gridWorld, torusWorld, lightMatrix, lightView, shadowProj;

	/* the stick carries the lamp around the torus and raises it */
	demo_zoom_update(&zoom, in);
	light_yaw += in->x * deg(3.0f);
	elevation = clampf(elevation - in->y * deg(1.5f), deg(15.0f), deg(88.0f));
	if (in->held & PSP_CTRL_RIGHT)
		camera += deg(2.0f);
	if (in->held & PSP_CTRL_LEFT)
		camera -= deg(2.0f);
	if (in->pressed & PSP_CTRL_SQUARE)
		distance = (distance + 1) % COUNT(distances);
	if (in->pressed & PSP_CTRL_CIRCLE)
		show_map ^= 1;
	snprintf(demo_status, sizeof(demo_status), "light %d  dist %.1f%s",
		(int)(elevation / deg(1.0f)), distances[distance], show_map ? "  MAP" : "");

	{
		ScePspFVector3 pos = { 0, 0, -5.0f * zoom };
		ScePspFVector3 rot = { 0, camera, 0 };
		gumLoadIdentity(&view);
		gumTranslate(&view, &pos);
		gumRotateXYZ(&view, &rot);
	}
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
		ScePspFVector3 rot1 = { 0, deg(frame * 0.79f) + light_yaw, 0 };
		ScePspFVector3 rot2 = { -elevation, 0, 0 };
		ScePspFVector3 pos = { 0, 0, distances[distance] };
		gumLoadIdentity(&lightMatrix);
		gumTranslate(&lightMatrix, &lookAt);
		gumRotateXYZ(&lightMatrix, &rot1);
		gumRotateXYZ(&lightMatrix, &rot2);
		gumTranslate(&lightMatrix, &pos);
	}
	gumFastInverse(&lightView, &lightMatrix);

	/* pass 1: the torus as seen from the light, black on white */
	demo_target_begin(RT_SIZE);
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

	if (show_map)
		draw_map();

	/* leave gum's cached matrices dirty, the next scene reloads them */
	sceGumMatrixMode(GU_PROJECTION); sceGumLoadIdentity();
	sceGumMatrixMode(GU_VIEW); sceGumLoadIdentity();
	sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity();
}

static const DemoHint hints[] = {
	{ GLYPH_STICK,      "lamp" },
	{ GLYPH_UPDOWN,     "zoom" },
	{ GLYPH_LEFTRIGHT,  "orbit" },
	{ GLYPH_SQUARE,     "distance" },
	{ GLYPH_CIRCLE,     "show map" },
};

const Scene scene_shadow = { "PROJECTED SHADOW", "shadow map projected via GU_TEXTURE_MATRIX",
	hints, COUNT(hints), init, reset, draw };
