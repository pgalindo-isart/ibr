#pragma once

#include "opengl_headers.h"
#include "types.h"

enum image_flags
{
    IMG_FLIP        = 1 << 0,
    IMG_FORCE_RGB   = 1 << 1,
    IMG_FORCE_RGBA  = 1 << 2,
    IMG_GEN_MIPMAPS = 1 << 3,
};

namespace GL
{
    // Same memory layout than 'struct light' in glsl shader
    struct alignas(16) light
    {
        int Enabled; float _Pad0[3];
        v4 ViewPosition; // (must be in view-space/eye-coordinates)
        v3 Ambient;  float _Pad1;
        v3 Diffuse;  float _Pad2;
        v3 Specular;
        v3 Attenuation;
    };

    // Same memory layout than 'struct material' in glsl shader
    struct alignas(16) material
    {
        v3 Ambient;  float _Pad0;
        v3 Diffuse;  float _Pad1;
        v3 Specular; float _Pad2;
        v3 Emission;
        float Shininess;
    };

    class cache
    {
    public:
        cache();
        ~cache();
        GLuint LoadObj(const char* Filename, float Scale, int* VertexCountOut);
        GLuint LoadTexture(const char* Filename, int ImageFlags = 0, int* WidthOut = nullptr, int* HeightOut = nullptr);
    private:
        struct data;
        data* Data = nullptr;
    };

    class debug
    {
    public:
        debug();
        ~debug();
        // Provide wireframe renderer information on how the positions are stored inside the MeshVBO
        void WireframePrepare(GLuint MeshVBO, GLsizei PositionStride, GLsizei PositionOffset, int VertexCount);
        // Draw previously prepared buffer (must be triangles)
        void WireframeDrawArray(GLint First, GLsizei Count, const mat4& MVP);

    private:
        struct data;
        data* Data = nullptr;
    };

    void UniformLight(GLuint Program, const char* LightUniformName, const light& Light);
    void UniformMaterial(GLuint Program, const char* MaterialUniformName, const material& Material);
    GLuint CompileShader(GLenum ShaderType, const char* ShaderStr, bool InjectLightShading = false);
    GLuint CompileShaderEx(GLenum ShaderType, int ShaderStrsCount, const char** ShaderStrs, bool InjectLightShading = false);
    GLuint CreateProgram(const char* VSString, const char* FSString, bool InjectLightShading = false);
    GLuint CreateProgramEx(int VSStringsCount, const char** VSStrings, int FSStringCount, const char** FSString, bool InjectLightShading = false);
    void UploadTexture(const char* Filename, int ImageFlags = 0, int* WidthOut = nullptr, int* HeightOut = nullptr);
    void UploadCheckerboardTexture(int Width, int Height, int SquareSize);
}

namespace GLImGui
{
    void InspectProgram(GLuint program);
}