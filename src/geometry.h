/* Mesh generators, taken from pspsdk src/samples/gu/common/geometry (BSD). */
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <psptypes.h>

typedef struct {
	ScePspFVector2 texture;
	u32 color;
	ScePspFVector3 normal;
	ScePspFVector3 position;
} TCNPVertex;
#define TCNP_VERTEX_FORMAT (GU_TEXTURE_32BITF|GU_COLOR_8888|GU_NORMAL_32BITF|GU_VERTEX_32BITF)

typedef struct {
	ScePspFVector2 texture;
	u32 color;
	ScePspFVector3 position;
} TCPVertex;
#define TCP_VERTEX_FORMAT (GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF)

typedef struct {
	ScePspFVector2 texture;
	ScePspFVector3 position;
} TPVertex;
#define TP_VERTEX_FORMAT (GU_TEXTURE_32BITF|GU_VERTEX_32BITF)

typedef struct {
	ScePspFVector3 normal;
	ScePspFVector3 position;
} NPVertex;
#define NP_VERTEX_FORMAT (GU_NORMAL_32BITF|GU_VERTEX_32BITF)

/* torus: slices*rows vertices, slices*rows*6 indices */
void generateTorusTCNP(unsigned int slices, unsigned int rows, float radius, float thickness, TCNPVertex* vertices, unsigned short* indices);
void generateTorusTCP(unsigned int slices, unsigned int rows, float radius, float thickness, TCPVertex* vertices, unsigned short* indices);
void generateTorusNP(unsigned int slices, unsigned int rows, float radius, float thickness, NPVertex* vertices, unsigned short* indices);

/* grid: columns*rows vertices, (columns-1)*(rows-1)*6 indices */
void generateGridTCNP(unsigned int columns, unsigned int rows, float width, float depth, TCNPVertex* vertices, unsigned short* indices);
void generateGridNP(unsigned int columns, unsigned int rows, float width, float depth, NPVertex* vertices, unsigned short* indices);

#endif
