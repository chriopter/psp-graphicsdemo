/* The running order. */
#include "demo.h"

extern const Scene scene_cube, scene_lights, scene_envmap, scene_cel, scene_morph,
	scene_skin, scene_spline, scene_rtt, scene_mirror, scene_shadow, scene_fog,
	scene_sprites, scene_clut;

const Scene* const demo_scenes[] = {
	&scene_cube,
	&scene_lights,
	&scene_envmap,
	&scene_cel,
	&scene_morph,
	&scene_skin,
	&scene_spline,
	&scene_rtt,
	&scene_mirror,
	&scene_shadow,
	&scene_fog,
	&scene_sprites,
	&scene_clut,
};

const int demo_scene_count = sizeof(demo_scenes) / sizeof(demo_scenes[0]);
