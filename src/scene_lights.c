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

/* the three light types the GE knows, cycled with O */
static const struct { int type; const char* name; } types[] = {
	{ GU_POINTLIGHT,   "POINT" },
	{ GU_SPOTLIGHT,    "SPOT" },
	{ GU_DIRECTIONAL,  "DIRECT" },
};

/* how shiny, cycled with left/right */
static const float shine[] = { 1.0f, 3.0f, 6.0f, 12.0f, 24.0f, 48.0f, 96.0f };

typedef struct { unsigned int color; float x, y, z; } MarkVertex;

static DemoTurn turn;
static int type, shininess, count, touched;

static void init(void)
{
	generateGridNP(GRID_COLUMNS, GRID_ROWS, GRID_SIZE, GRID_SIZE, grid_vertices, grid_indices);
	generateTorusNP(TORUS_SLICES, TORUS_ROWS, 1.0f, 0.5f, torus_vertices, torus_indices);
}

static void reset(void)
{
	demo_turn_reset(&turn);
	type = 0;
	shininess = 3;
	count = 4;
	touched = 0;
}

/* A flat quad per light, in its own colour, so the ring can be seen while it moves. */
static void draw_markers(const ScePspFVector3* pos, int n)
{
	MarkVertex* v = sceGuGetMemory(sizeof(MarkVertex) * 2 * n);
	int i;
	sceGuDisable(GU_LIGHTING);
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	for (i = 0; i < n; ++i) {
		v[i*2+0].color = colors[i];
		v[i*2+0].x = pos[i].x - 0.09f;
		v[i*2+0].y = pos[i].y - 0.09f;
		v[i*2+0].z = pos[i].z;
		v[i*2+1].color = colors[i];
		v[i*2+1].x = pos[i].x + 0.09f;
		v[i*2+1].y = pos[i].y + 0.09f;
		v[i*2+1].z = pos[i].z;
	}
	sceGumDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 2 * n, 0, v);
	sceGuEnable(GU_LIGHTING);
}

static void draw(int frame, const DemoInput* in)
{
	ScePspFVector3 light[4];
	int i;

	/* the stick spins the ring of lights and lifts it */
	if (in->x != 0.0f || in->y != 0.0f || (in->held & SCENE_BUTTONS))
		touched = 1;
	demo_turn_update(&turn, in);
	turn.pitch = clampf(turn.pitch, deg(-90.0f), deg(150.0f));
	if (in->pressed & PSP_CTRL_SQUARE)
		count = (count % 4) + 1;
	if (in->pressed & PSP_CTRL_CIRCLE)
		type = (type + 1) % COUNT(types);
	if ((in->repeat & PSP_CTRL_RIGHT) && shininess < COUNT(shine) - 1)
		shininess++;
	if ((in->repeat & PSP_CTRL_LEFT) && shininess > 0)
		shininess--;
	snprintf(demo_status, sizeof(demo_status), "%d %s  shine %d",
		count, types[type].name, (int)shine[shininess]);

	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	sceGuEnable(GU_LIGHTING);
	for (i = 0; i < count; ++i) {
		float angle = i * (GU_PI/2) + deg(frame) + turn.yaw;
		light[i].x = cosf(angle) * LIGHT_DISTANCE;
		light[i].y = -turn.pitch * 2.0f;
		light[i].z = sinf(angle) * LIGHT_DISTANCE;
		sceGuEnable(GU_LIGHT0 + i);
		sceGuLight(i, types[type].type, GU_DIFFUSE_AND_SPECULAR, &light[i]);
		if (types[type].type == GU_SPOTLIGHT) {
			/* a spot has to be aimed; these look down at the torus in the middle */
			ScePspFVector3 dir = { -light[i].x, -light[i].y, -light[i].z };
			gumNormalize(&dir);
			sceGuLightSpot(i, &dir, 6.0f, 0.7f);
		}
		sceGuLightColor(i, GU_DIFFUSE, colors[i]);
		sceGuLightColor(i, GU_SPECULAR, 0xffffffff);
		sceGuLightAtt(i, 0.0f, 1.0f, 0.0f);
	}
	sceGuSpecular(shine[shininess]);
	sceGuAmbient(0x00222222);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 1.0f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0, 0, -3.5f * turn.zoom };
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

	/* the markers are for the player moving the lights, not for the demo reel */
	if (touched)
		draw_markers(light, count);
}

const Scene scene_lights = { "HARDWARE LIGHTS", "four point lights, diffuse + specular per vertex",
	"stick lights  ^v zoom  <> shine  [] count  O type", init, reset, draw };
