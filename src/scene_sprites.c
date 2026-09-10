/* Sprite cloud - after the pspsdk "sprite" sample. */
#include "demo.h"

#define NUM_SLICES 128
#define NUM_ROWS 128
#define RING_SIZE 2.0f
#define RING_RADIUS 1.0f
#define SPRITE_SIZE 0.025f

typedef struct { float x, y, z; } InputVertex;
typedef struct { float u, v; unsigned int color; float x, y, z; } Vertex;

static const unsigned int colors[8] = {
	0xffff0000, 0xffff00ff, 0xff0000ff, 0xff00ffff, 0xff00ff00, 0xffffff00, 0xffffffff, 0xff101010
};

static InputVertex torus_vertices[NUM_SLICES * NUM_ROWS];

static void init(void)
{
	unsigned int i, j;
	for (i = 0; i < NUM_SLICES; ++i) {
		for (j = 0; j < NUM_ROWS; ++j) {
			float s = i + 0.5f, t = j;
			InputVertex* v = &torus_vertices[j + i * NUM_ROWS];
			v->x = (RING_SIZE + RING_RADIUS * cosf(s * ((GU_PI*2)/NUM_SLICES))) * cosf(t * ((GU_PI*2)/NUM_ROWS));
			v->y = (RING_SIZE + RING_RADIUS * cosf(s * ((GU_PI*2)/NUM_SLICES))) * sinf(t * ((GU_PI*2)/NUM_ROWS));
			v->z = RING_RADIUS * sinf(s * ((GU_PI*2)/NUM_SLICES));
		}
	}
}

/* Two corners per sprite, offset along the world matrix's x+y so the
   quads face the camera whatever the rotation. */
static void billboards(Vertex* vertices, const float* world)
{
	unsigned int i, j;
	float sx = SPRITE_SIZE * world[0] + SPRITE_SIZE * world[1];
	float sy = SPRITE_SIZE * world[4] + SPRITE_SIZE * world[5];
	float sz = SPRITE_SIZE * world[8] + SPRITE_SIZE * world[9];
	for (i = 0; i < NUM_SLICES; ++i) {
		Vertex* row = &vertices[i * NUM_ROWS * 2];
		InputVertex* inrow = &torus_vertices[i * NUM_ROWS];
		for (j = 0; j < NUM_ROWS; ++j) {
			Vertex* curr = &row[j << 1];
			InputVertex* in = &inrow[j];
			curr[0].u = 0; curr[0].v = 0; curr[0].color = colors[(i+j)&7];
			curr[0].x = in->x - sx; curr[0].y = in->y - sy; curr[0].z = in->z - sz;
			curr[1].u = 1; curr[1].v = 1; curr[1].color = colors[(i+j)&7];
			curr[1].x = in->x + sx; curr[1].y = in->y + sy; curr[1].z = in->z + sz;
		}
	}
}

static void draw(int frame)
{
	ScePspFMatrix4 world;
	Vertex* vertices;
	float val = frame * 0.6f;

	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	sceGuAlphaFunc(GU_GREATER, 0, 0xff);
	sceGuEnable(GU_ALPHA_TEST);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_5551, 0, 0, 0);
	sceGuTexImage(0, 32, 32, 32, ball_start);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0.0f, 0.0f, -3.5f };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
	}
	sceGumMatrixMode(GU_MODEL);
	{
		ScePspFVector3 rot = { deg(val * 0.3f), deg(val * 0.7f), deg(val * 1.3f) };
		sceGumLoadIdentity();
		sceGumRotateXYZ(&rot);
	}
	sceGumStoreMatrix(&world);

	vertices = sceGuGetMemory(NUM_SLICES * NUM_ROWS * 2 * sizeof(Vertex));
	billboards(vertices, (float*)&world);
	sceGumDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, NUM_SLICES*NUM_ROWS*2, 0, vertices);
}

const Scene scene_sprites = { "SPRITE CLOUD", "16384 GU_SPRITES billboards with alpha test", init, draw };
