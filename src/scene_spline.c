/* Spline surface - after the pspsdk "splinesurface" sample. */
#include "demo.h"
#include <stdlib.h>

#define GRID_WIDTH 18
#define GRID_HEIGHT 18

typedef struct { unsigned int color; ScePspFVector3 normal; ScePspFVector3 position; } Vertex;

static Vertex __attribute__((aligned(16))) vertices[GRID_WIDTH * GRID_HEIGHT];
static unsigned short __attribute__((aligned(16))) indices[GRID_WIDTH * GRID_HEIGHT * 6];
static int params[8] = { 0, 2, 2, 2, 2, 2, 2, 2 };
static int last_change = -1;

static float hue2rgb(float m1, float m2, float h)
{
	h += h < 0.0f ? 1.0f : h > 1.0f ? -1.0f : 0.0f;
	if ((h*6.0f) < 1.0f) return m1 + (m2-m1) * h * 6.0f;
	if ((h*2.0f) < 1.0f) return m2;
	if ((h*3.0f) < 2.0f) return m1 + (m2-m1) * ((2.0f/3.0f)-h) * 6.0f;
	return m1;
}

static unsigned int hsl2rgb(float h, float s, float l)
{
	float m2 = l < 0.5f ? l * (s+1.0f) : (l+s) - (l*s);
	float m1 = l*2.0f-m2;
	int r = (int)(255.0f * hue2rgb(m1, m2, h+(1.0f/3.0f)));
	int g = (int)(255.0f * hue2rgb(m1, m2, h));
	int b = (int)(255.0f * hue2rgb(m1, m2, h-(1.0f/3.0f)));
	return (0xff << 24) | (b << 16) | (g << 8) | r;
}

/* spherical harmonics as a control net */
static void evalSH(float theta, float phi, const int* m, ScePspFVector3* p)
{
	float r = 0;
	r += powf(sinf(m[0]*phi), (float)m[1]);
	r += powf(cosf(m[2]*phi), (float)m[3]);
	r += powf(sinf(m[4]*theta), (float)m[5]);
	r += powf(cosf(m[6]*theta), (float)m[7]);
	p->x = r * sinf(phi) * cosf(theta);
	p->y = r * cosf(phi);
	p->z = r * sinf(phi) * sinf(theta);
}

static void init(void)
{
	unsigned short i, j;
	float dh = 1.0f / GRID_WIDTH;
	for (i = 0; i < GRID_WIDTH; ++i) {
		for (j = 0; j < GRID_HEIGHT; ++j) {
			unsigned short* curr = &indices[(j + (i * GRID_HEIGHT)) * 6];
			unsigned short i1 = (i+1) % GRID_WIDTH, j1 = (j+1) % GRID_HEIGHT;
			vertices[j + i * GRID_HEIGHT].color = hsl2rgb(i * dh, 1.0f, 0.5f);
			*curr++ = j + i * GRID_HEIGHT;
			*curr++ = j1 + i * GRID_HEIGHT;
			*curr++ = j + i1 * GRID_HEIGHT;
			*curr++ = j1 + i * GRID_HEIGHT;
			*curr++ = j1 + i1 * GRID_HEIGHT;
			*curr++ = j + i1 * GRID_HEIGHT;
		}
	}
}

static void update_net(const int* m)
{
	float du = (GU_PI*2) / (GRID_WIDTH-1);
	float dv = GU_PI / (GRID_HEIGHT-1);
	unsigned int i, j;
	Vertex* currvtx = vertices;
	for (i = 0; i < GRID_WIDTH; ++i) {
		float u = fmodf(i * du, GU_PI*2);
		for (j = 0; j < GRID_HEIGHT; ++j) {
			evalSH(u, j * dv, m, &(currvtx->position));
			currvtx++;
		}
	}
	for (i = 0; i < GRID_WIDTH; ++i) {
		for (j = 0; j < GRID_HEIGHT; ++j) {
			ScePspFVector3 l1, l2;
			unsigned short* curr = &indices[(j + (i * GRID_HEIGHT)) * 6];
			ScePspFVector3* normal = &vertices[curr[0]].normal;
			l1.x = vertices[curr[1]].position.x - vertices[curr[0]].position.x;
			l1.y = vertices[curr[1]].position.y - vertices[curr[0]].position.y;
			l1.z = vertices[curr[1]].position.z - vertices[curr[0]].position.z;
			l2.x = vertices[curr[2]].position.x - vertices[curr[0]].position.x;
			l2.y = vertices[curr[2]].position.y - vertices[curr[0]].position.y;
			l2.z = vertices[curr[2]].position.z - vertices[curr[0]].position.z;
			gumCrossProduct(normal, &l1, &l2);
			gumNormalize(normal);
		}
	}
	sceKernelDcacheWritebackAll();
}

static const struct { ScePspFVector3 position; unsigned int diffuse, specular; } lights[3] = {
	{ {-1,-1,-1}, 0xffffffff, 0xffffffff },  /* key */
	{ { 1, 0,-1}, 0xff202020, 0xff000000 },  /* fill */
	{ { 0,-1, 1}, 0xff808080, 0xff000000 },  /* back */
};

static void draw(int frame)
{
	unsigned int i;

	/* a new harmonic every four seconds; the first one is the classic */
	if (frame == 0)
		last_change = -1;
	if (frame >= 240 && (frame / 240) != last_change) {
		last_change = frame / 240;
		for (i = 0; i < 8; ++i)
			params[i] = (int)((rand() / ((float)RAND_MAX)) * 6.0f);
	}
	if (frame == 0) {
		static const int classic[8] = { 0, 2, 2, 2, 2, 2, 2, 2 };
		memcpy(params, classic, sizeof(params));
	}

	sceGuClearColor(0xff000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuDisable(GU_CULL_FACE);

	sceGuEnable(GU_LIGHTING);
	sceGuLightMode(GU_SEPARATE_SPECULAR_COLOR);
	for (i = 0; i < 3; ++i) {
		sceGuEnable(GU_LIGHT0 + i);
		sceGuLight(i, GU_DIRECTIONAL, GU_DIFFUSE_AND_SPECULAR, &lights[i].position);
		sceGuLightColor(i, GU_DIFFUSE, lights[i].diffuse);
		sceGuLightColor(i, GU_SPECULAR, lights[i].specular);
		sceGuLightAtt(i, 0.0f, 1.0f, 0.0f);
	}
	sceGuSpecular(12.0f);
	sceGuAmbient(0x000000);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0, 0, -5.0f };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
	}
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	{
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f), deg(frame * 1.32f) };
		sceGumRotateXYZ(&rot);
	}

	update_net(params);
	sceGumDrawSpline(GU_NORMAL_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF, GRID_HEIGHT, GRID_WIDTH, 3, 3, indices, vertices);
}

const Scene scene_spline = { "SPLINE SURFACE", "sceGumDrawSpline over an 18x18 control net", init, draw };
