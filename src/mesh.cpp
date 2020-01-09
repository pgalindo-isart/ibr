
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <vector>
#include <string>

#include "tiny_obj_loader.h"

#include "maths.h"

#include "mesh.h"

using namespace Mesh;

struct base_vertex
{
    v3 Position;
    v3 Normal;
    v2 UV;
};

static void* ConvertVertices(void* VerticesDst, const vertex_descriptor& Descriptor, base_vertex* VerticesSrc, int Count)
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

static int GetVertexCount(void* Vertices, void* End, const vertex_descriptor& Descriptor)
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

void* Mesh::BuildInvertedCube(void* Vertices, void* End, const vertex_descriptor& Descriptor)
{
    void* V = Vertices;

    // FRONT FACE
    V = Mesh::Transform(
        V, Mesh::BuildQuad(V, End, Descriptor),
        Descriptor, Mat4::Translate({ 0.f, 0.f, -0.5f }));
    // BACK FACE
    V = Mesh::Transform(
        V, Mesh::BuildQuad(V, End, Descriptor),
        Descriptor, Mat4::RotateY(Math::Pi()) * Mat4::Translate({ 0.f, 0.f, -0.5f }));
    // RIGHT FACE
    V = Mesh::Transform(
        V, Mesh::BuildQuad(V, End, Descriptor),
        Descriptor, Mat4::RotateY(Math::HalfPi()) * Mat4::Translate({ 0.f, 0.f, -0.5f }));
    // LEFT FACE
    V = Mesh::Transform(
        V, Mesh::BuildQuad(V, End, Descriptor),
        Descriptor, Mat4::RotateY(-Math::HalfPi()) * Mat4::Translate({ 0.f, 0.f, -0.5f }));
    // TOP FACE
    V = Mesh::Transform(
        V, Mesh::BuildQuad(V, End, Descriptor),
        Descriptor, Mat4::RotateY(-Math::HalfPi()) * Mat4::RotateX(Math::HalfPi()) * Mat4::Translate({ 0.f, 0.f, -0.5f }));
    // BOTTOM FACE
    V = Mesh::Transform(
        V, Mesh::BuildQuad(V, End, Descriptor),
        Descriptor, Mat4::RotateY(-Math::HalfPi()) * Mat4::RotateX(-Math::HalfPi()) * Mat4::Translate({ 0.f, 0.f, -0.5f }));

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

void* Mesh::LoadObj(void* Vertices, void* End, const vertex_descriptor& Descriptor, const char* Filename, float Scale)
{
    bool HasNormals = false;
    bool HasTexCoords = false;

    std::vector<base_vertex> Mesh;

    {
        std::string Warn;
        std::string Err;
        tinyobj::attrib_t Attrib;
        std::vector<tinyobj::shape_t> Shapes;

        tinyobj::LoadObj(&Attrib, &Shapes, nullptr, &Warn, &Err, Filename, "media/", true);
        if (!Err.empty())
        {
            fprintf(stderr, "Warning loading obj: %s\n", Err.c_str());
        }
        if (!Err.empty())
        {
            fprintf(stderr, "Error loading obj: %s\n", Err.c_str());
            return Vertices;
        }

        HasNormals = !Attrib.normals.empty();
        HasTexCoords = !Attrib.texcoords.empty();

        // Build all meshes
        for (int MeshId = 0; MeshId < (int)Shapes.size(); ++MeshId)
        {
            const tinyobj::mesh_t& MeshDef = Shapes[MeshId].mesh;
            
            int IndexId = 0;
            for (int FaceId = 0; FaceId < (int)MeshDef.num_face_vertices.size(); ++FaceId)
            {
                int FaceVertices = MeshDef.num_face_vertices[FaceId];
                assert(FaceVertices == 3);
        
                for (int j = 0; j < FaceVertices; ++j)
                {
                    const tinyobj::index_t& Index = MeshDef.indices[IndexId];
                    base_vertex V;
                    V.Position = {
                        Scale * Attrib.vertices[Index.vertex_index * 3 + 0],
                        Scale * Attrib.vertices[Index.vertex_index * 3 + 1],
                        Scale * Attrib.vertices[Index.vertex_index * 3 + 2]
                    };

                    if (HasNormals)
                    {
                        V.Normal = {
                            Attrib.normals[Index.normal_index * 3 + 0],
                            Attrib.normals[Index.normal_index * 3 + 1],
                            Attrib.normals[Index.normal_index * 3 + 2]
                        };
                    }

                    if (HasTexCoords)
                    {
                        V.UV = {
                            Attrib.texcoords[Index.texcoord_index * 2 + 0],
                            Attrib.texcoords[Index.texcoord_index * 2 + 1]
                        };
                    }
                    
                    Mesh.push_back(V);

                    IndexId++;
                }
            }
        }
    }

    // Build normals if missing
    if (!HasNormals)
    {
        for (int i = 0; i < (int)Mesh.size(); i += 3)
        {
            base_vertex& V0 = Mesh[i+0];
            base_vertex& V1 = Mesh[i+1];
            base_vertex& V2 = Mesh[i+2];

            v3 Normal = Vec3::Cross((V1.Position - V0.Position), (V2.Position - V0.Position));
            V0.Normal = V1.Normal = V2.Normal = Normal;
        }
    }
    
    // Build UVs if missing
    if (!HasTexCoords)
    {
        // TODO: Maybe triplanar texturing can make best results
        for (int i = 0; i < (int)Mesh.size(); ++i)
        {
            base_vertex& V = Mesh[i];

            float Length = Vec3::Length(V.Position);
            if (Length != 0.f)
            {
                v3 Pos = V.Position / Length;
                V.UV.x = 0.5f + Math::Atan2(Pos.z, Pos.x);
                V.UV.y = Pos.y;
            }
        }
    }

    int MeshSize = (int)Mesh.size();
    int SizeAvailable = GetVertexCount(Vertices, End, Descriptor);
    if (MeshSize > SizeAvailable)
    {
        fprintf(stderr, "Mesh '%s' does not fit inside vertex buffer (%d needed, %d available)\n", Filename, MeshSize, SizeAvailable);
        MeshSize = SizeAvailable;
    }

    return ConvertVertices(Vertices, Descriptor, &Mesh[0], MeshSize);
}
