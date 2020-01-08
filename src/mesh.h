#pragma once

#include "types.h"

// Descriptor for interleaved vertex formats
struct vertex_descriptor
{
	int PositionOffset;
	bool HasNormal;
	int NormalOffset;
	bool HasUV;
	int UVOffset;
	int Stride;
};

namespace Mesh
{
void* Transform(void* Vertices, void* End, const vertex_descriptor& Descriptor, const mat4& Transform);
void* BuildQuad(void* Vertices, void* End, const vertex_descriptor& Descriptor);
void* BuildCube(void* Vertices, void* End, const vertex_descriptor& Descriptor);
void* BuildSphere(void* Vertices, void* End, const vertex_descriptor& Descriptor, int Lon, int Lat);
}