/*
 * PSP Graphics Demo - main loop, GU setup, overlay and controls.
 */
#include "demo.h"
#include <pspctrl.h>

PSP_MODULE_INFO("PSP Graphics Demo", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define SCENE_FRAMES 480   /* eight seconds per scene at 60 Hz */

static unsigned int __attribute__((aligned(16))) list[262144];
void* demo_draw_buffer;
static volatile int exit_request;

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

void demo_target_begin(void)
{
	sceGuDrawBufferList(GU_PSM_8888, (void*)VRAM_RT, RT_SIZE);
	sceGuOffset(2048 - (RT_SIZE / 2), 2048 - (RT_SIZE / 2));
	sceGuViewport(2048, 2048, RT_SIZE, RT_SIZE);
	sceGuScissor(0, 0, RT_SIZE, RT_SIZE);
}

void demo_target_end(void)
{
	sceGuDrawBufferList(GU_PSM_8888, demo_draw_buffer, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

static void draw_overlay(int scene, int frame, int autoplay)
{
	const Scene* s = demo_scenes[scene];
	char counter[16];

	demo_reset_state();
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDisable(GU_CULL_FACE);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	/* bands behind the text, and the progress line for the auto advance */
	demo_draw_rect(0, 0, SCR_WIDTH, 26, 0x90000000);
	demo_draw_rect(0, SCR_HEIGHT - 26, SCR_WIDTH, 26, 0x90000000);
	if (autoplay)
		demo_draw_rect(0, SCR_HEIGHT - 2, (SCR_WIDTH * frame) / SCENE_FRAMES, 2, 0xffe0b060);

	demo_text_begin();
	sprintf(counter, "%02d/%02d", scene + 1, demo_scene_count);
	demo_draw_string(counter, 8, 5, 0xffe0b060, 0);
	demo_draw_string(s->name, 64, 5, 0xffffffff, 0);
	demo_draw_string(s->detail, 8, SCR_HEIGHT - 21, 0xffd8d8d8, 0);
	{
		const char* hint = autoplay ? "L/R  X hold  START" : "L/R  X play  START";
		demo_draw_string(hint, SCR_WIDTH - 8 - demo_string_width(hint, 0), 5, 0xff909090, 0);
	}
}

int main(int argc, char* argv[])
{
	int i, scene = 0, frame = 0, autoplay = 1;
	unsigned int old_buttons = 0;

	setup_callbacks();

	for (i = 0; i < demo_scene_count; ++i)
		demo_scenes[i]->init();
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
		SceCtrlData pad;
		unsigned int pressed;

		sceCtrlPeekBufferPositive(&pad, 1);
		pressed = pad.Buttons & ~old_buttons;
		old_buttons = pad.Buttons;

		if (pressed & PSP_CTRL_START)
			break;
		if (pressed & PSP_CTRL_CROSS)
			autoplay ^= 1;
		if (pressed & (PSP_CTRL_RTRIGGER | PSP_CTRL_RIGHT)) {
			scene = (scene + 1) % demo_scene_count;
			frame = 0;
		}
		if (pressed & (PSP_CTRL_LTRIGGER | PSP_CTRL_LEFT)) {
			scene = (scene + demo_scene_count - 1) % demo_scene_count;
			frame = 0;
		}

		sceGuStart(GU_DIRECT, list);
		demo_reset_state();
		demo_scenes[scene]->draw(frame);
		draw_overlay(scene, frame, autoplay);
		sceGuFinish();
		sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

		sceDisplayWaitVblankStart();
		demo_draw_buffer = sceGuSwapBuffers();

		frame++;
		if (autoplay && frame >= SCENE_FRAMES) {
			scene = (scene + 1) % demo_scene_count;
			frame = 0;
		}
	}

	sceGuTerm();
	sceKernelExitGame();
	return 0;
}
