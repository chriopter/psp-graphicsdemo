TARGET = graphicsdemo
OBJS = src/main.o src/text.o src/scenes.o src/geometry.o \
	src/scene_cube.o src/scene_lights.o src/scene_envmap.o src/scene_cel.o \
	src/scene_morph.o src/scene_skin.o src/scene_spline.o src/scene_rtt.o \
	src/scene_mirror.o src/scene_shadow.o src/scene_fog.o src/scene_sprites.o \
	src/scene_clut.o \
	assets/logo.o assets/env0.o assets/lightmap.o assets/ball.o assets/font.o

INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lpspgum -lpspgu -lpspge -lpspdisplay -lpspctrl -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP Graphics Demo
PSP_EBOOT_ICON = assets/ICON0.PNG

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

assets/%.o: assets/%.raw
	bin2o -i $< $@ $(notdir $*)
