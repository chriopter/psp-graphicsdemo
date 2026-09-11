/*
 * The hint row: the pad's own symbols, drawn with the GE, and what each one
 * does in the scene on screen. Shapes and labels go down in two passes so the
 * texture unit is switched once instead of once per chip.
 */
#include "demo.h"

typedef struct { unsigned int color; float x, y, z; } ShapeVertex;
#define SHAPE_FORMAT (GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D)

/* the colours the symbols carry on the pad itself */
#define COLOR_SQUARE   0xffd884ff
#define COLOR_TRIANGLE 0xff88dc88
#define COLOR_CIRCLE   0xff6464ff
#define COLOR_CROSS    0xffffb464
#define COLOR_PAD      0xffdcdcdc

#define GLYPH_SIZE 16
#define PILL_PAD   4
#define CHIP_GAP   13
#define LABEL_GAP  5

/* A bar of width w between two points: the stroke every outline is built from. */
static void bar(float x0, float y0, float x1, float y1, float w, unsigned int color)
{
	float dx = x1 - x0, dy = y1 - y0;
	float len = sqrtf(dx*dx + dy*dy), nx, ny;
	ShapeVertex* v;
	int i;
	if (len < 0.001f)
		return;
	nx = (-dy / len) * (w * 0.5f);
	ny = ( dx / len) * (w * 0.5f);
	v = (ShapeVertex*)sceGuGetMemory(sizeof(ShapeVertex) * 6);
	v[0].x = x0 + nx; v[0].y = y0 + ny;
	v[1].x = x1 + nx; v[1].y = y1 + ny;
	v[2].x = x1 - nx; v[2].y = y1 - ny;
	v[3].x = x0 + nx; v[3].y = y0 + ny;
	v[4].x = x1 - nx; v[4].y = y1 - ny;
	v[5].x = x0 - nx; v[5].y = y0 - ny;
	for (i = 0; i < 6; ++i) {
		v[i].color = color;
		v[i].z = 0.0f;
	}
	sceGuDrawArray(GU_TRIANGLES, SHAPE_FORMAT, 6, 0, v);
}

static void ring(float cx, float cy, float inner, float outer, unsigned int color)
{
	enum { STEPS = 16 };
	ShapeVertex* v = (ShapeVertex*)sceGuGetMemory(sizeof(ShapeVertex) * (STEPS + 1) * 2);
	int i;
	for (i = 0; i <= STEPS; ++i) {
		float a = (2.0f * GU_PI * i) / STEPS;
		float cs = cosf(a), sn = sinf(a);
		v[i*2+0].color = color; v[i*2+0].x = cx + cs * inner; v[i*2+0].y = cy + sn * inner; v[i*2+0].z = 0.0f;
		v[i*2+1].color = color; v[i*2+1].x = cx + cs * outer; v[i*2+1].y = cy + sn * outer; v[i*2+1].z = 0.0f;
	}
	sceGuDrawArray(GU_TRIANGLE_STRIP, SHAPE_FORMAT, (STEPS + 1) * 2, 0, v);
}

static void wedge(float ax, float ay, float bx, float by, float cx, float cy, unsigned int color)
{
	ShapeVertex* v = (ShapeVertex*)sceGuGetMemory(sizeof(ShapeVertex) * 3);
	v[0].color = color; v[0].x = ax; v[0].y = ay; v[0].z = 0.0f;
	v[1].color = color; v[1].x = bx; v[1].y = by; v[1].z = 0.0f;
	v[2].color = color; v[2].x = cx; v[2].y = cy; v[2].z = 0.0f;
	sceGuDrawArray(GU_TRIANGLES, SHAPE_FORMAT, 3, 0, v);
}

static void frame(float x, float y, float w, float h, float t, unsigned int color)
{
	bar(x, y + t*0.5f, x + w, y + t*0.5f, t, color);
	bar(x, y + h - t*0.5f, x + w, y + h - t*0.5f, t, color);
	bar(x + t*0.5f, y, x + t*0.5f, y + h, t, color);
	bar(x + w - t*0.5f, y, x + w - t*0.5f, y + h, t, color);
}

static const char* pill_label(int glyph, int half)
{
	if (glyph == GLYPH_SHOULDERS)
		return half ? "R" : "L";
	if (glyph == GLYPH_SELECT)
		return "SELECT";
	if (glyph == GLYPH_START)
		return "START";
	return 0;
}

static int pill_width(const char* text)
{
	return demo_string_width(text, 0) + 2 * PILL_PAD;
}

int demo_glyph_width(int glyph)
{
	switch (glyph) {
	case GLYPH_SHOULDERS: return pill_width("L") + 3 + pill_width("R");
	case GLYPH_SELECT:    return pill_width("SELECT");
	case GLYPH_START:     return pill_width("START");
	default:              return GLYPH_SIZE;
	}
}

static void draw_glyph(int glyph, int x, int y)
{
	float cx = x + GLYPH_SIZE * 0.5f, cy = y + GLYPH_SIZE * 0.5f;

	switch (glyph) {
	case GLYPH_STICK:
		ring(cx, cy, 5.0f, 7.0f, COLOR_PAD);
		ring(cx, cy, 0.0f, 2.5f, COLOR_PAD);
		break;
	case GLYPH_UPDOWN:
		wedge(cx, cy - 8.0f, cx - 4.5f, cy - 2.0f, cx + 4.5f, cy - 2.0f, COLOR_PAD);
		wedge(cx, cy + 8.0f, cx + 4.5f, cy + 2.0f, cx - 4.5f, cy + 2.0f, COLOR_PAD);
		break;
	case GLYPH_LEFTRIGHT:
		wedge(cx - 8.0f, cy, cx - 2.0f, cy + 4.5f, cx - 2.0f, cy - 4.5f, COLOR_PAD);
		wedge(cx + 8.0f, cy, cx + 2.0f, cy - 4.5f, cx + 2.0f, cy + 4.5f, COLOR_PAD);
		break;
	case GLYPH_SQUARE:
		frame(cx - 6.0f, cy - 6.0f, 12.0f, 12.0f, 2.0f, COLOR_SQUARE);
		break;
	case GLYPH_CIRCLE:
		ring(cx, cy, 4.5f, 6.5f, COLOR_CIRCLE);
		break;
	case GLYPH_TRIANGLE:
		bar(cx, cy - 6.5f, cx + 6.5f, cy + 5.5f, 2.0f, COLOR_TRIANGLE);
		bar(cx + 6.5f, cy + 5.5f, cx - 6.5f, cy + 5.5f, 2.0f, COLOR_TRIANGLE);
		bar(cx - 6.5f, cy + 5.5f, cx, cy - 6.5f, 2.0f, COLOR_TRIANGLE);
		break;
	case GLYPH_CROSS:
		bar(cx - 5.0f, cy - 5.0f, cx + 5.0f, cy + 5.0f, 2.2f, COLOR_CROSS);
		bar(cx + 5.0f, cy - 5.0f, cx - 5.0f, cy + 5.0f, 2.2f, COLOR_CROSS);
		break;
	case GLYPH_SHOULDERS: {
		int w = pill_width("L");
		frame((float)x, (float)y, (float)w, (float)GLYPH_SIZE + 2.0f, 1.0f, COLOR_PAD);
		frame((float)(x + w + 3), (float)y, (float)pill_width("R"), (float)GLYPH_SIZE + 2.0f, 1.0f, COLOR_PAD);
		break;
	}
	case GLYPH_SELECT:
	case GLYPH_START:
		frame((float)x, (float)y, (float)pill_width(pill_label(glyph, 0)), (float)GLYPH_SIZE + 2.0f, 1.0f, COLOR_PAD);
		break;
	}
}

static int chip_width(const DemoHint* hint)
{
	return demo_glyph_width(hint->glyph) + LABEL_GAP + demo_string_width(hint->label, 0) + CHIP_GAP;
}

int demo_hints_width(const DemoHint* hints, int count)
{
	int i, w = 0;
	for (i = 0; i < count; ++i)
		w += chip_width(&hints[i]);
	return w - CHIP_GAP;
}

/* Glyphs first with the texture off, then every label in one textured pass. */
void demo_draw_hints(const DemoHint* hints, int count, int x, int y, unsigned int color)
{
	int i, at;

	sceGuDisable(GU_TEXTURE_2D);
	for (i = 0, at = x; i < count; ++i) {
		draw_glyph(hints[i].glyph, at, y);
		at += chip_width(&hints[i]);
	}

	demo_text_begin();
	for (i = 0, at = x; i < count; ++i) {
		int glyph = hints[i].glyph;
		const char* first = pill_label(glyph, 0);
		if (first) {
			demo_draw_string(first, at + PILL_PAD, y + 1, color, 0);
			if (glyph == GLYPH_SHOULDERS) {
				const char* second = pill_label(glyph, 1);
				demo_draw_string(second, at + pill_width(first) + 3 + PILL_PAD, y + 1, color, 0);
			}
		}
		demo_draw_string(hints[i].label, at + demo_glyph_width(glyph) + LABEL_GAP, y + 1, color, 0);
		at += chip_width(&hints[i]);
	}
}
