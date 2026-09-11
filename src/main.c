/*
 * PSP Graphics Demo - main loop, GU setup, overlay and controls.
 */
#include "demo.h"
#include <pspctrl.h>
#include <pspiofilemgr.h>

PSP_MODULE_INFO("PSP Graphics Demo", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define SCENE_FRAMES 480   /* eight seconds per scene at 60 Hz */
#define MAX_SCENES 32

static unsigned int __attribute__((aligned(16))) list[262144];
void* demo_draw_buffer;
char demo_status[64];
static volatile int exit_request;

/*
 * Recording mode, for the video in the README. If ms0:/PSP/GRAPHICSDEMO.REC
 * exists and holds "<frames> <step> <offset>", every <step>th frame from
 * <offset> on is appended raw (512x272, 8888) to ms0:/PSP/GRAPHICSDEMO.RAW
 * and the demo exits after <frames> frames. tools/make-video.sh sets it up.
 */
static SceUID rec_fd = -1;
static int rec_frames, rec_step = 1, rec_offset;

static void record_open(void)
{
	char text[64];
	int n;
	SceUID fd = sceIoOpen("ms0:/PSP/GRAPHICSDEMO.REC", PSP_O_RDONLY, 0);
	if (fd < 0)
		return;
	n = sceIoRead(fd, text, sizeof(text) - 1);
	sceIoClose(fd);
	if (n <= 0)
		return;
	text[n] = 0;
	if (sscanf(text, "%d %d %d", &rec_frames, &rec_step, &rec_offset) < 1 || rec_frames <= 0)
		return;
	if (rec_step < 1)
		rec_step = 1;
	rec_fd = sceIoOpen("ms0:/PSP/GRAPHICSDEMO.RAW", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
}

/* Called once the GE has finished the frame, before the swap. Returns 1 when done. */
static int record_frame(int total)
{
	if (rec_fd < 0)
		return 0;
	if (total >= rec_offset && ((total - rec_offset) % rec_step) == 0)
		sceIoWrite(rec_fd, sceGeEdramGetAddr() + (unsigned int)demo_draw_buffer, FRAME_SIZE);
	if (total + 1 >= rec_frames) {
		sceIoClose(rec_fd);
		rec_fd = -1;
		return 1;
	}
	return 0;
}

static int exit_callback(int arg1, int arg2, void* common)
{
	exit_request = 1;
	return 0;
}

static int callback_thread(SceSize args, void* argp)
{
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

static void setup_callbacks(void)
{
	int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, 0);
}

/* Stick to -1..1; the centre of a worn stick wanders, so the middle quarter is cut out. */
static float stick_axis(unsigned char raw)
{
	float a = ((int)raw - 128) / 127.0f;
	if (a > -0.25f && a < 0.25f)
		return 0.0f;
	return clampf((a > 0.0f ? a - 0.25f : a + 0.25f) / 0.75f, -1.0f, 1.0f);
}

static void read_input(DemoInput* in)
{
	static DemoInput last;
	static unsigned char hold[32];
	SceCtrlData pad;
	int b;

	/* Peek can come back empty; then the pad is as it was, with nothing new pressed. */
	memset(&pad, 0, sizeof(pad));
	if (sceCtrlPeekBufferPositive(&pad, 1) <= 0) {
		*in = last;
		in->pressed = in->repeat = 0;
		return;
	}
	in->x = stick_axis(pad.Lx);
	in->y = stick_axis(pad.Ly);
	in->pressed = pad.Buttons & ~last.held;
	in->held = pad.Buttons;
	in->repeat = 0;
	/* a held button repeats after a third of a second, twelve times a second */
	for (b = 0; b < 32; ++b) {
		if (!(pad.Buttons & (1u << b))) {
			hold[b] = 0;
			continue;
		}
		hold[b] = hold[b] < 250 ? hold[b] + 1 : 20;   /* keep the cadence, never wrap past it */
		if (hold[b] == 1 || (hold[b] >= 20 && (hold[b] - 20) % 5 == 0))
			in->repeat |= 1u << b;
	}
	last = *in;
}

void demo_turn_reset(DemoTurn* turn)
{
	turn->yaw = turn->pitch = 0.0f;
	turn->zoom = 1.0f;
}

void demo_zoom_update(float* zoom, const DemoInput* in)
{
	if (in->held & PSP_CTRL_UP)
		*zoom *= 0.99f;
	if (in->held & PSP_CTRL_DOWN)
		*zoom *= 1.01f;
	*zoom = clampf(*zoom, 0.6f, 3.0f);
}

void demo_turn_update(DemoTurn* turn, const DemoInput* in)
{
	turn->yaw += in->x * deg(3.0f);
	turn->pitch += in->y * deg(3.0f);
	demo_zoom_update(&turn->zoom, in);
}

void demo_turn_apply(const DemoTurn* turn)
{
	ScePspFVector3 rot = { turn->pitch, turn->yaw, 0.0f };
	sceGumRotateXYZ(&rot);
}

/* Every scene starts from the same state and only switches on what it needs. */
void demo_reset_state(void)
{
	static ScePspFMatrix4 identity;
	gumLoadIdentity(&identity);

	sceGuDrawBufferList(GU_PSM_8888, demo_draw_buffer, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);

	sceGuDepthRange(0xc350, 0x2710);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuDepthMask(GU_FALSE);

	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);

	sceGuDisable(GU_TEXTURE_2D);
	sceGuDisable(GU_LIGHTING);
	sceGuDisable(GU_LIGHT0);
	sceGuDisable(GU_LIGHT1);
	sceGuDisable(GU_LIGHT2);
	sceGuDisable(GU_LIGHT3);
	sceGuLightMode(GU_SINGLE_COLOR);
	sceGuDisable(GU_BLEND);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_STENCIL_TEST);
	sceGuDisable(GU_DITHER);
	sceGuDisable(GU_FOG);
	sceGuPixelMask(0);

	sceGuTexMapMode(GU_TEXTURE_COORDS, 0, 0);
	sceGuTexProjMapMode(GU_UV);
	sceGuTexScale(1.0f, 1.0f);
	sceGuTexOffset(0.0f, 0.0f);
	sceGuTexWrap(GU_REPEAT, GU_REPEAT);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuSetMatrix(GU_TEXTURE, &identity);

	sceGuColor(0xffffffff);
	sceGuAmbientColor(0xffffffff);
	sceGuAmbient(0x00000000);
}

void demo_target_begin(int size)
{
	sceGuDrawBufferList(GU_PSM_8888, (void*)VRAM_RT, RT_SIZE);
	sceGuOffset(2048 - (size / 2), 2048 - (size / 2));
	sceGuViewport(2048, 2048, size, size);
	sceGuScissor(0, 0, size, size);
}

void demo_target_end(void)
{
	sceGuDrawBufferList(GU_PSM_8888, demo_draw_buffer, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

/* Autoplay shows the technique; once the player takes over, the band grows
   a line for the scene's buttons and the top right shows its settings. */
static void draw_overlay(int scene, int frame, int autoplay, int frozen)
{
	const Scene* s = demo_scenes[scene];
	int band = autoplay ? 26 : 44;
	char counter[16], status[80];

	demo_reset_state();
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDisable(GU_CULL_FACE);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	/* bands behind the text, and the progress line for the auto advance */
	demo_draw_rect(0, 0, SCR_WIDTH, 26, 0x90000000);
	demo_draw_rect(0, SCR_HEIGHT - band, SCR_WIDTH, band, 0x90000000);
	if (autoplay)
		demo_draw_rect(0, SCR_HEIGHT - 2, (SCR_WIDTH * frame) / SCENE_FRAMES, 2, 0xffe0b060);

	demo_text_begin();
	sprintf(counter, "%02d/%02d", scene + 1, demo_scene_count);
	demo_draw_string(counter, 8, 5, 0xffe0b060, 0);
	demo_draw_string(s->name, 64, 5, 0xffffffff, 0);
	if (autoplay) {
		const char* hint = "L/R  X hold  START";
		demo_draw_string(s->detail, 8, SCR_HEIGHT - 21, 0xffd8d8d8, 0);
		demo_draw_string(hint, SCR_WIDTH - 8 - demo_string_width(hint, 0), 5, 0xff909090, 0);
		return;
	}
	demo_draw_string(s->detail, 8, SCR_HEIGHT - 39, 0xffd8d8d8, 0);
	demo_draw_string(s->controls, 8, SCR_HEIGHT - 21, 0xffe0b060, 0);
	snprintf(status, sizeof(status), "%s%s", frozen ? "||  " : "", demo_status);
	demo_draw_string(status, SCR_WIDTH - 8 - demo_string_width(status, 0), 5, 0xffffffff, 0);
}

int main(int argc, char* argv[])
{
	/* frame counts toward the auto advance; each scene keeps its own clock and settings */
	int i, scene = 0, frame = 0, total = 0, autoplay = 1;
	static int clock[MAX_SCENES];
	static unsigned char frozen[MAX_SCENES];

	setup_callbacks();
	record_open();

	for (i = 0; i < demo_scene_count && i < MAX_SCENES; ++i) {
		demo_scenes[i]->init();
		demo_scenes[i]->reset();
	}
	sceKernelDcacheWritebackAll();

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	demo_draw_buffer = (void*)VRAM_FB0;

	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, (void*)VRAM_FB0, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)VRAM_FB1, BUF_WIDTH);
	sceGuDepthBuffer((void*)VRAM_ZBUF, BUF_WIDTH);
	demo_reset_state();
	sceGuFinish();
	sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	while (!exit_request)
	{
		DemoInput in;
		const Scene* s;

		read_input(&in);
		if (in.pressed & PSP_CTRL_START)
			break;
		if ((in.held & SCENE_BUTTONS) || in.x != 0.0f || in.y != 0.0f)
			autoplay = 0;
		if (in.pressed & PSP_CTRL_CROSS)
			autoplay ^= 1;
		if (in.pressed & PSP_CTRL_RTRIGGER) {
			scene = (scene + 1) % demo_scene_count;
			frame = 0;
		}
		if (in.pressed & PSP_CTRL_LTRIGGER) {
			scene = (scene + demo_scene_count - 1) % demo_scene_count;
			frame = 0;
		}
		s = demo_scenes[scene];
		if (in.pressed & PSP_CTRL_TRIANGLE)
			frozen[scene] ^= 1;
		if (in.pressed & PSP_CTRL_SELECT) {
			s->reset();
			clock[scene] = 0;
			frozen[scene] = 0;
		}

		demo_status[0] = 0;
		sceGuStart(GU_DIRECT, list);
		demo_reset_state();
		s->draw(clock[scene], &in);
		draw_overlay(scene, frame, autoplay, frozen[scene]);
		sceGuFinish();
		sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

		if (record_frame(total))
			break;

		sceDisplayWaitVblankStart();
		demo_draw_buffer = sceGuSwapBuffers();

		frame++;
		total++;
		if (!frozen[scene])
			clock[scene]++;
		if (autoplay && frame >= SCENE_FRAMES) {
			scene = (scene + 1) % demo_scene_count;
			frame = 0;
		}
	}

	sceGuTerm();
	sceKernelExitGame();
	return 0;
}
