#pragma once

#include <vector>

#include "types.h"

// Descriptor for interleaved vertex formats
struct vertex_descriptor
{
	int Stride;
	int PositionOffset;
	bool HasNormal;
	int NormalOffset;
	bool HasUV;
	int UVOffset;
};

struct vertex_full
{
	v3 Position;
	v3 Normal;
	v2 UV;
};

namespace Mesh
{

void* Transform(void* Vertices, void* End, const vertex_descriptor& Descriptor, const mat4& Transform);
void* BuildQuad(void* Vertices, void* End, const vertex_descriptor& Descriptor);
void* BuildCube(void* Vertices, void* End, const vertex_descriptor& Descriptor);
void* BuildInvertedCube(void* Vertices, void* End, const vertex_descriptor& Descriptor);
void* BuildSphere(void* Vertices, void* End, const vertex_descriptor& Descriptor, int Lon, int Lat);
void* LoadObj(void* Vertices, void* End, const vertex_descriptor& Descriptor, const char* Filename, float Scale);
bool LoadObjNoConvertion(std::vector<vertex_full>& Mesh, const char* Filename, float Scale);
}