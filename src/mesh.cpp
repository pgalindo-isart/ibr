
#include <cstdint>
#include <cstdio>

#include "maths.h"

#include "mesh.h"

using namespace Mesh;

struct base_vertex
{
    v3 Position;
    v3 Normal;
    v2 UV;
};

void* ConvertVertices(void* VerticesDst, const vertex_descriptor& Descriptor, base_vertex* VerticesSrc, int Count)
{
    uint8_t* Buffer = (uint8_t*)VerticesDst;

    for (int i = 0; i < Count; ++i)
    {
        base_vertex& VertexSrc = VerticesSrc[i];
        uint8_t* VertexStart = Buffer + i * Descriptor.Stride;

        v3* PositionDst = (v3*)(VertexStart + Descriptor.PositionOffset);
        *PositionDst = VertexSrc.Position;

        if (Descriptor.HasNormal)
        {
            v3* NormalDst = (v3*)(VertexStart + Descriptor.NormalOffset);
            *NormalDst = VertexSrc.Normal;
        }

        if (Descriptor.HasUV)
        {
            v2* UVDst = (v2*)(VertexStart + Descriptor.UVOffset);
            *UVDst = VertexSrc.UV;
        }
    }

    return Buffer + Descriptor.Stride * Count;
}

int GetVertexCount(void* Vertices, void* End, const vertex_descriptor& Descriptor)
{
    int SizeInBytes = (int)((uint8_t*)End - (uint8_t*)Vertices);
    return SizeInBytes / Descriptor.Stride;
}

void* Mesh::Transform(void* Vertices, void* End, const vertex_descriptor& Descriptor, const mat4& Transform)
{
    uint8_t* Buffer = (uint8_t*)Vertices;
    int Count = GetVertexCount(Vertices, End, Descriptor);

    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(Transform));
    for (int i = 0; i < Count; ++i)
    {
        uint8_t* VertexStart = Buffer + i * Descriptor.Stride;
        
        v3* Position = (v3*)(VertexStart + Descriptor.PositionOffset);
        v4 TransformedPosition = Transform * Vec4::vec4(*Position, 1.f);
        *Position = TransformedPosition.xyz / TransformedPosition.w; // normalized homogeneous coordinate

        if (Descriptor.HasNormal)
        {
            v3* Normal           = (v3*)(VertexStart + Descriptor.NormalOffset);
            v4 TransformedNormal = NormalMatrix * Vec4::vec4(*Normal, 0.f);
            *Normal              = Vec3::Normalize(TransformedNormal.xyz);
        }
    }
    return Buffer + Descriptor.Stride * Count;
}

void* Mesh::BuildQuad(void* Vertices, void* End, const vertex_descriptor& Descriptor)
{
    if (GetVertexCount(Vertices, End, Descriptor) < 6)
    {
        fprintf(stderr, "Not enough vertices to create quad\n");
        return Vertices;
    }

    v3 Normal = { 0.f, 0.f, 1.f };

    base_vertex TopLeft     = { { -0.5f, 0.5f, 0.f }, Normal, { 0.f, 1.f } };
    base_vertex TopRight    = { {  0.5f, 0.5f, 0.f }, Normal, { 1.f, 1.f } };
    base_vertex BottomLeft  = { { -0.5f,-0.5f, 0.f }, Normal, { 0.f, 0.f } };
    base_vertex BottomRight = { {  0.5f,-0.5f, 0.f }, Normal, { 1.f, 0.f } };

    base_vertex QuadVertices[] = {
        TopLeft,
        BottomLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        TopRight
    };

    return ConvertVertices(Vertices, Descriptor, QuadVertices, 6);
}

void* Mesh::BuildCube(void* Vertices, void* End, const vertex_descriptor& Descriptor)
{
    if (GetVertexCount(Vertices, End, Descriptor) < (6 * 6))
    {
        fprintf(stderr, "Not enough vertices to create cube\n");
        return Vertices;
    }

    void* V = Vertices;

    // Back face
    V = Mesh::Transform(V, BuildQuad(V, End, Descriptor), Descriptor,
        Mat4::Translate({ 0.0f, 0.0f,-0.5f }) * Mat4::RotateY(-1.f, 0.f));
    // Front face
    V = Mesh::Transform(V, BuildQuad(V, End, Descriptor), Descriptor,
        Mat4::Translate({ 0.0f, 0.0f, 0.5f }) * Mat4::RotateY( 1.f, 0.f));
    // Left face
    V = Mesh::Transform(V, BuildQuad(V, End, Descriptor), Descriptor,
        Mat4::Translate({-0.5f, 0.0f, 0.0f }) * Mat4::RotateY( 0.f, 1.f));
    // Right face
    V = Mesh::Transform(V, BuildQuad(V, End, Descriptor), Descriptor,
        Mat4::Translate({ 0.5f, 0.0f, 0.0f }) * Mat4::RotateY( 0.f,-1.f));
    // Top face
    V = Mesh::Transform(V, BuildQuad(V, End, Descriptor), Descriptor,
        Mat4::Translate({ 0.0f, 0.5f, 0.0f }) * Mat4::RotateX( 0.f,-1.f));
    // Bottom face
    V = Mesh::Transform(V, BuildQuad(V, End, Descriptor), Descriptor,
        Mat4::Translate({ 0.0f,-0.5f, 0.0f }) * Mat4::RotateX( 0.f, 1.f));

    return V;
}

void* Mesh::BuildSphere(void* Vertices, void* End, const vertex_descriptor& Descriptor, int Lon, int Lat)
{
    if (GetVertexCount(Vertices, End, Descriptor) < (Lon * Lat * 6))
    {
        fprintf(stderr, "Not enough vertices to create sphere\n");
        return Vertices;
    }

    uint8_t* Cur = (uint8_t*)Vertices;
    for (int i = 0; i < Lat; ++i)
    {
        // Theta varies from 0 to 180
        float Theta        = Math::Pi() * (float)(i+0) / Lat;
        float ThetaCos     = Math::Cos(Theta);
        float ThetaSin     = Math::Sin(Theta);
        
        float ThetaNext    = Math::Pi() * (float)(i+1) / Lat;
        float ThetaNextCos = Math::Cos(ThetaNext);
        float ThetaNextSin = Math::Sin(ThetaNext);

        for (int j = 0; j < Lon; ++j)
        {
            // Phi varies from 0 to 360
            float Phi        = Math::TwoPi() * (float)(j+0) / Lon;
            float PhiCos     = Math::Cos(Phi);
            float PhiSin     = Math::Sin(Phi);
            
            float PhiNext    = Math::TwoPi() * (float)(j+1) / Lon;
            float PhiNextCos = Math::Cos(PhiNext);
            float PhiNextSin = Math::Sin(PhiNext);

            // Compute positions
            v3 P0 = {     ThetaSin * PhiCos,         ThetaCos,     ThetaSin * PhiSin     };
            v3 P1 = {     ThetaSin * PhiNextCos,     ThetaCos,     ThetaSin * PhiNextSin };
            v3 P2 = { ThetaNextSin * PhiCos,     ThetaNextCos, ThetaNextSin * PhiSin     };
            v3 P3 = { ThetaNextSin * PhiNextCos, ThetaNextCos, ThetaNextSin * PhiNextSin };

            base_vertex Quad[6];

            Quad[0].Position = Quad[0].Normal = P0;
            Quad[1].Position = Quad[1].Normal = P1;
            Quad[2].Position = Quad[2].Normal = P2;
            Quad[3].Position = Quad[3].Normal = P2;
            Quad[4].Position = Quad[4].Normal = P1;
            Quad[5].Position = Quad[5].Normal = P3;

            // Set uv
            float U0 = 1.f - (j + 0.f) / (float)Lon;
            float U1 = 1.f - (j + 1.f) / (float)Lon;
            float V0 = 1.f - (i + 0.f) / (float)Lat;
            float V1 = 1.f - (i + 1.f) / (float)Lat;

            Quad[0].UV = { U0, V0 };
            Quad[1].UV = { U1, V0 };
            Quad[2].UV = { U0, V1 };
            Quad[3].UV = Quad[2].UV;
            Quad[4].UV = Quad[1].UV;
            Quad[5].UV = { U1, V1 };

            Cur = (uint8_t*)ConvertVertices(Cur, Descriptor, Quad, 6);
        }
    }

    // Transform pos from [-1;+1] to [-0.5;+0.5] to make a unit sphere
    return Mesh::Transform(Vertices, Cur, Descriptor, Mat4::Scale({ 0.5f, 0.5f, 0.5f }));
}