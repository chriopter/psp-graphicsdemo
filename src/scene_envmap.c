/* Environment mapping - after the pspsdk "envmap" sample. */
#include "demo.h"
#include "geometry.h"

#define TORUS_SLICES 48
#define TORUS_ROWS 48

static NPVertex __attribute__((aligned(16))) torus_vertices[TORUS_SLICES*TORUS_ROWS];
static unsigned short __attribute__((aligned(16))) torus_indices[TORUS_SLICES*TORUS_ROWS*6];

static void init(void)
{
	generateTorusNP(TORUS_ROWS, TORUS_SLICES, 1.0f, 0.5f, torus_vertices, torus_indices);
}

static void draw(int frame)
{
	float angle, cs, sn;
	ScePspFVector3 dir = { 0, 0, 1 };
	ScePspFVector3 columns[2];

	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuDepthRange(65535, 0);

	sceGuEnable(GU_LIGHTING);
	sceGuEnable(GU_LIGHT0);
	sceGuLight(0, GU_DIRECTIONAL, GU_DIFFUSE, &dir);
	sceGuLightColor(0, GU_DIFFUSE, 0x00ff4040);
	sceGuLightAtt(0, 1.0f, 0.0f, 0.0f);
	sceGuAmbient(0x00202020);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 1.0f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_4444, 0, 0, 0);
	sceGuTexImage(0, 64, 64, 64, env0_start);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);

	/* The 2x2 envmap matrix rides in light slots 2 and 3, one column each. */
	angle = deg(-2.0f * frame);
	cs = cosf(angle);
	sn = sinf(angle);
	columns[0].x = cs;  columns[0].y = sn; columns[0].z = 0.0f;
	columns[1].x = -sn; columns[1].y = cs; columns[1].z = 0.0f;
	sceGuLight(2, GU_DIRECTIONAL, GU_DIFFUSE, &columns[0]);
	sceGuLight(3, GU_DIRECTIONAL, GU_DIFFUSE, &columns[1]);
	sceGuTexMapMode(GU_ENVIRONMENT_MAP, 2, 3);

	sceGumMatrixMode(GU_MODEL);
	{
		ScePspFVector3 pos = { 0, 0, -2.5f };
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f), deg(frame * 1.32f) };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rot);
	}
	sceGuColor(0xffffff);
	sceGumDrawArray(GU_TRIANGLES, NP_VERTEX_FORMAT | GU_INDEX_16BIT | GU_TRANSFORM_3D, sizeof(torus_indices)/sizeof(unsigned short), torus_indices, torus_vertices);
}

const Scene scene_envmap = { "ENVIRONMENT MAP", "GU_ENVIRONMENT_MAP, matrix in light slots 2+3", init, draw };
