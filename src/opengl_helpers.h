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
    // Same data format than the shader light structure
    struct light
    {
        v4 Position;
        v3 Ambient; float _Pad0;
        v3 Diffuse; float _Pad1;
        v3 Specular;
        float Shininess;
        float Attenuation[3];
    };

    GLuint CompileShader(GLenum ShaderType, const char* ShaderStr, bool InjectLightShading = false);
    GLuint CreateProgram(const char* VSString, const char* FSString, bool InjectLightShading = false);
    void UploadTexture(const char* Filename, int ImageFlags = 0, int* WidthOut = nullptr, int* HeightOut = nullptr);
    void UploadCheckerboardTexture(int Width, int Height, int SquareSize);
}

namespace GLImGui
{
    void InspectProgram(GLuint program);
}