/* Palette cycling - after the pspsdk "clut" sample. */
#include "demo.h"

typedef struct { float u, v; float x, y, z; } Vertex;

static const char* const style_names[] = { "RGB", "FIRE", "BANDS", "GREY" };

static unsigned int __attribute__((aligned(16))) clut256[256];
static unsigned char __attribute__((aligned(16))) tex256[256*256];

static float pan_u, pan_v, zoom;
static int speed, style, nearest, phase, last_frame;

static void init(void)
{
	unsigned int i, j;
	for (j = 0; j < 256; ++j)
		for (i = 0; i < 256; ++i)
			tex256[i + j * 256] = j ^ i;
}

static void reset(void)
{
	pan_u = pan_v = 0.0f;
	zoom = 1.0f;
	speed = 2;
	style = 0;
	nearest = 0;
	phase = 0;
	last_frame = 0;
}

static unsigned int palette_entry(unsigned int j)
{
	int r, g, b, ramp;
	switch (style) {
	case 1:   /* fire, up and back down so the cycle stays seamless */
		ramp = (j < 128 ? (int)j : 255 - (int)j) * 6;
		r = ramp > 255 ? 255 : ramp;
		g = ramp < 255 ? 0 : (ramp - 255 > 255 ? 255 : ramp - 255);
		b = ramp < 510 ? 0 : ramp - 510;
		break;
	case 2:   /* bands: the bits of the XOR pattern, black and white */
		r = g = b = (j & 32) ? 255 : 0;
		break;
	case 3:
		r = g = b = (int)j;
		break;
	default:
		r = (int)j;
		g = (int)(j * 3) & 0xff;
		b = 255 - (int)j;
		break;
	}
	return 0xff000000 | (b << 16) | (g << 8) | r;
}

static void draw(int frame, const DemoInput* in)
{
	unsigned int i;
	float span;
	/* write the palette uncached, the GE reads it right after */
	unsigned int* clut = (unsigned int*)(((unsigned int)clut256) | 0x40000000);
	Vertex* v;

	/* the clock stops while the scene is frozen, and so does the cycling */
	if (frame != last_frame) {
		phase += speed;
		last_frame = frame;
	}
	demo_zoom_update(&zoom, in);
	pan_u += in->x * 4.0f;
	pan_v += in->y * 4.0f;
	if ((in->repeat & PSP_CTRL_RIGHT) && speed < 8)
		speed++;
	if ((in->repeat & PSP_CTRL_LEFT) && speed > -8)
		speed--;
	if (in->pressed & PSP_CTRL_SQUARE)
		style = (style + 1) % COUNT(style_names);
	if (in->pressed & PSP_CTRL_CIRCLE)
		nearest ^= 1;
	snprintf(demo_status, sizeof(demo_status), "speed %d  %s  %s",
		speed, style_names[style], nearest ? "NEAREST" : "LINEAR");

	for (i = 0; i < 256; ++i)
		*(clut++) = palette_entry((i + phase) & 0xff);

	sceGuClearColor(0xff000000);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuDisable(GU_DEPTH_TEST);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuClutMode(GU_PSM_8888, 0, 0xff, 0);
	sceGuClutLoad(256/8, clut256);
	sceGuTexMode(GU_PSM_T8, 0, 0, 0);
	sceGuTexImage(0, 256, 256, 256, tex256);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
	sceGuTexFilter(nearest ? GU_NEAREST : GU_LINEAR, nearest ? GU_NEAREST : GU_LINEAR);

	span = 256.0f * zoom;
	v = (Vertex*)sceGuGetMemory(2 * sizeof(Vertex));
	v[0].u = pan_u;        v[0].v = pan_v;        v[0].x = 0;         v[0].y = 0;          v[0].z = 0;
	v[1].u = pan_u + span; v[1].v = pan_v + span; v[1].x = SCR_WIDTH; v[1].y = SCR_HEIGHT; v[1].z = 0;
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, v);
	sceGuEnable(GU_DEPTH_TEST);
}

static const DemoHint hints[] = {
	{ GLYPH_STICK,      "pan" },
	{ GLYPH_UPDOWN,     "zoom" },
	{ GLYPH_LEFTRIGHT,  "speed" },
	{ GLYPH_SQUARE,     "palette" },
	{ GLYPH_CIRCLE,     "filter" },
};

const Scene scene_clut = { "PALETTE CYCLING", "8-bit XOR texture, 256-entry CLUT cycled",
	hints, COUNT(hints), init, reset, draw };
