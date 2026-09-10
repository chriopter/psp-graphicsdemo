/* Four hardware point lights - after the pspsdk "lights" sample. */
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

static const unsigned int colors[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffff00ff };

static void init(void)
{
	generateGridNP(GRID_COLUMNS, GRID_ROWS, GRID_SIZE, GRID_SIZE, grid_vertices, grid_indices);
	generateTorusNP(TORUS_SLICES, TORUS_ROWS, 1.0f, 0.5f, torus_vertices, torus_indices);
}

static void draw(int frame)
{
	int i;
	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	sceGuEnable(GU_LIGHTING);
	for (i = 0; i < 4; ++i) {
		ScePspFVector3 pos = { cosf(i*(GU_PI/2) + deg(frame)) * LIGHT_DISTANCE, 0, sinf(i*(GU_PI/2) + deg(frame)) * LIGHT_DISTANCE };
		sceGuEnable(GU_LIGHT0 + i);
		sceGuLight(i, GU_POINTLIGHT, GU_DIFFUSE_AND_SPECULAR, &pos);
		sceGuLightColor(i, GU_DIFFUSE, colors[i]);
		sceGuLightColor(i, GU_SPECULAR, 0xffffffff);
		sceGuLightAtt(i, 0.0f, 1.0f, 0.0f);
	}
	sceGuSpecular(12.0f);
	sceGuAmbient(0x00222222);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 1.0f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0, 0, -3.5f };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
	}

	sceGumMatrixMode(GU_MODEL);
	{
		ScePspFVector3 pos = { 0, -1.5f, 0 };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
	}
	sceGuColor(0xff7777);
	sceGumDrawArray(GU_TRIANGLES, NP_VERTEX_FORMAT | GU_INDEX_16BIT | GU_TRANSFORM_3D, sizeof(grid_indices)/sizeof(unsigned short), grid_indices, grid_vertices);

	{
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f), deg(frame * 1.32f) };
		sceGumLoadIdentity();
		sceGumRotateXYZ(&rot);
	}
	sceGuColor(0xffffff);
	sceGumDrawArray(GU_TRIANGLES, NP_VERTEX_FORMAT | GU_INDEX_16BIT | GU_TRANSFORM_3D, sizeof(torus_indices)/sizeof(unsigned short), torus_indices, torus_vertices);
}

const Scene scene_lights = { "HARDWARE LIGHTS", "four point lights, diffuse + specular per vertex", init, draw };
