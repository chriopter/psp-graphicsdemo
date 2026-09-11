/* Render to texture - after the pspsdk "rendertarget" sample. */
#include "demo.h"
#include "geometry.h"

#define TORUS_SLICES 48
#define TORUS_ROWS 48

typedef struct { float u, v; unsigned int color; float x, y, z; } Vertex;

static Vertex __attribute__((aligned(16))) cube_vertices[12*3] = {
	{0, 0, 0xff7f0000,-1,-1, 1}, {1, 0, 0xff7f0000,-1, 1, 1}, {1, 1, 0xff7f0000, 1, 1, 1},
	{0, 0, 0xff7f0000,-1,-1, 1}, {1, 1, 0xff7f0000, 1, 1, 1}, {0, 1, 0xff7f0000, 1,-1, 1},
	{0, 0, 0xff7f0000,-1,-1,-1}, {1, 0, 0xff7f0000, 1,-1,-1}, {1, 1, 0xff7f0000, 1, 1,-1},
	{0, 0, 0xff7f0000,-1,-1,-1}, {1, 1, 0xff7f0000, 1, 1,-1}, {0, 1, 0xff7f0000,-1, 1,-1},
	{0, 0, 0xff007f00, 1,-1,-1}, {1, 0, 0xff007f00, 1,-1, 1}, {1, 1, 0xff007f00, 1, 1, 1},
	{0, 0, 0xff007f00, 1,-1,-1}, {1, 1, 0xff007f00, 1, 1, 1}, {0, 1, 0xff007f00, 1, 1,-1},
	{0, 0, 0xff007f00,-1,-1,-1}, {1, 0, 0xff007f00,-1, 1,-1}, {1, 1, 0xff007f00,-1, 1, 1},
	{0, 0, 0xff007f00,-1,-1,-1}, {1, 1, 0xff007f00,-1, 1, 1}, {0, 1, 0xff007f00,-1,-1, 1},
	{0, 0, 0xff00007f,-1, 1,-1}, {1, 0, 0xff00007f, 1, 1,-1}, {1, 1, 0xff00007f, 1, 1, 1},
	{0, 0, 0xff00007f,-1, 1,-1}, {1, 1, 0xff00007f, 1, 1, 1}, {0, 1, 0xff00007f,-1, 1, 1},
	{0, 0, 0xff00007f,-1,-1,-1}, {1, 0, 0xff00007f,-1,-1, 1}, {1, 1, 0xff00007f, 1,-1, 1},
	{0, 0, 0xff00007f,-1,-1,-1}, {1, 1, 0xff00007f, 1,-1, 1}, {0, 1, 0xff00007f, 1,-1,-1},
};

static NPVertex __attribute__((aligned(16))) torus_vertices[TORUS_SLICES*TORUS_ROWS];
static unsigned short __attribute__((aligned(16))) torus_indices[TORUS_SLICES*TORUS_ROWS*6];

/* the target is square and its side is a power of two, so it can be a texture */
static const int sizes[] = { 128, 64, 32, 16 };

static DemoTurn turn;
static float spin;
static int size, nearest;

static void init(void)
{
	generateTorusNP(TORUS_ROWS, TORUS_SLICES, 1.0f, 0.5f, torus_vertices, torus_indices);
}

static void reset(void)
{
	demo_turn_reset(&turn);
	spin = 0.0f;
	size = 0;
	nearest = 0;
}

static void draw_torus(int frame)
{
	ScePspFVector3 dir = { 0, 0, 1 };
	sceGuEnable(GU_LIGHTING);
	sceGuEnable(GU_LIGHT0);
	sceGuLight(0, GU_DIRECTIONAL, GU_DIFFUSE, &dir);
	sceGuLightColor(0, GU_DIFFUSE, 0x00ff4040);
	sceGuLightAtt(0, 1.0f, 0.0f, 0.0f);
	sceGuAmbient(0x00202020);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 1.0f, 0.5f, 1000.0f);   /* the target is square */
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0.0f, 0.0f, -2.5f };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
	}
	sceGumMatrixMode(GU_MODEL);
	{
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f) + spin, deg(frame * 1.32f) };
		sceGumLoadIdentity();
		sceGumRotateXYZ(&rot);
	}
	sceGuColor(0xffffff);
	sceGumDrawArray(GU_TRIANGLES, NP_VERTEX_FORMAT | GU_INDEX_16BIT | GU_TRANSFORM_3D, sizeof(torus_indices)/sizeof(unsigned short), torus_indices, torus_vertices);
	sceGuDisable(GU_LIGHTING);
	sceGuDisable(GU_LIGHT0);
}

static void draw_cube(int frame)
{
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0.0f, 0.0f, -3.0f * turn.zoom };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
	}
	sceGumMatrixMode(GU_MODEL);
	{
		ScePspFVector3 rot = { deg(frame * 0.263f), deg(frame * 0.32f), deg(frame * 0.44f) };
		sceGumLoadIdentity();
		demo_turn_apply(&turn);
		sceGumRotateXYZ(&rot);
	}
	/* the offscreen buffer, read back as a plain 8888 texture */
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	/* the drawn corner of the target; the stride stays the full 128 */
	sceGuTexImage(0, sizes[size], sizes[size], RT_SIZE, sceGeEdramGetAddr() + VRAM_RT);
	sceGuTexFunc(GU_TFX_ADD, GU_TCC_RGB);
	sceGuTexFilter(nearest ? GU_NEAREST : GU_LINEAR, nearest ? GU_NEAREST : GU_LINEAR);
	sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 12*3, 0, cube_vertices);
	sceGuDisable(GU_TEXTURE_2D);
}

static void draw(int frame, const DemoInput* in)
{
	demo_turn_update(&turn, in);
	if (in->held & PSP_CTRL_RIGHT)
		spin += deg(2.0f);
	if (in->held & PSP_CTRL_LEFT)
		spin -= deg(2.0f);
	if (in->pressed & PSP_CTRL_SQUARE)
		size = (size + 1) % COUNT(sizes);
	if (in->pressed & PSP_CTRL_CIRCLE)
		nearest ^= 1;
	snprintf(demo_status, sizeof(demo_status), "target %dx%d  %s",
		sizes[size], sizes[size], nearest ? "NEAREST" : "LINEAR");

	/* pass 1: torus into the target */
	demo_target_begin(sizes[size]);
	sceGuClearColor(0xffffffff);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	draw_torus(frame);

	/* pass 2: the frame, with the target wrapped around a cube */
	demo_target_end();
	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	draw_cube(frame);
}

const Scene scene_rtt = { "RENDER TO TEXTURE", "torus into a 128x128 VRAM target, then a cube",
	"stick turn  ^v zoom  <> spin torus  [] size  O filter", init, reset, draw };
