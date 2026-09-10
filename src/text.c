/*
 * 2D text and rectangles for the overlay. The font sprite sheet and the
 * width table come from the pspsdk "text" sample by McZonk (BSD).
 */
#include "demo.h"

static const unsigned char fontwidth[128] = {
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10,  6,  8, 10, 10, 10, 10,  6, 10, 10, 10, 10,  6, 10,  6, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  6,  6, 10, 10, 10, 10,
	16, 10, 10, 10, 10, 10, 10, 10, 10,  6,  8, 10,  8, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 12, 10, 10, 10, 10, 10, 10,  8, 10,
	 6,  8,  8,  8,  8,  8,  6,  8,  8,  6,  6,  8,  6, 10,  8,  8,
	 8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8, 10,  8, 10,  8, 12
};

typedef struct {
	float s, t;
	unsigned int c;
	float x, y, z;
} TextVertex;

typedef struct {
	unsigned int c;
	float x, y, z;
} RectVertex;

void demo_text_begin(void)
{
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, 256, 128, 256, font_start);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexScale(1.0f / 256.0f, 1.0f / 128.0f);
	sceGuTexOffset(0.0f, 0.0f);
	sceGuTexWrap(GU_REPEAT, GU_REPEAT);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
}

static int char_width(unsigned char c, int fw)
{
	if (fw) return fw;
	if (c >= 128) c = 0;
	return fontwidth[c];
}

int demo_string_width(const char* text, int fw)
{
	int w = 0;
	while (*text)
		w += char_width((unsigned char)*text++, fw);
	return w;
}

void demo_draw_string(const char* text, int x, int y, unsigned int color, int fw)
{
	int len = (int)strlen(text), i;
	TextVertex* v;
	if (!len)
		return;
	v = sceGuGetMemory(sizeof(TextVertex) * 2 * len);
	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char)text[i];
		int w, inset, tx, ty;
		if (c < 32 || c >= 128) c = 0;
		w = char_width(c, fw);
		inset = (16 - w) >> 1;
		tx = (c & 0x0F) << 4;
		ty = (c & 0xF0);
		v[i*2+0].s = (float)(tx + inset);
		v[i*2+0].t = (float)ty;
		v[i*2+0].c = color;
		v[i*2+0].x = (float)x;
		v[i*2+0].y = (float)y;
		v[i*2+0].z = 0.0f;
		v[i*2+1].s = (float)(tx + 16 - inset);
		v[i*2+1].t = (float)(ty + 16);
		v[i*2+1].c = color;
		v[i*2+1].x = (float)(x + w);
		v[i*2+1].y = (float)(y + 16);
		v[i*2+1].z = 0.0f;
		x += w;
	}
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, len * 2, 0, v);
}

void demo_draw_rect(int x, int y, int w, int h, unsigned int color)
{
	RectVertex* v = sceGuGetMemory(sizeof(RectVertex) * 2);
	if (w <= 0 || h <= 0)
		return;
	v[0].c = color; v[0].x = (float)x;       v[0].y = (float)y;       v[0].z = 0.0f;
	v[1].c = color; v[1].x = (float)(x + w); v[1].y = (float)(y + h); v[1].z = 0.0f;
	sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, v);
}
