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
    struct Light
    {
        v4 Position;
        v3 Ambient;
        v3 Diffuse;
        v3 Specular;
        float Shininess;
        float Attenuation[3];
    };

    GLuint CompileShader(GLenum ShaderType, const char* ShaderStr);
    GLuint CreateProgram(const char* VSString, const char* FSString);
    void UploadTexture(const char* Filename, int ImageFlags, int* WidthOut = nullptr, int* HeightOut = nullptr);
}

namespace GLImGui
{
    void InspectProgram(GLuint program);
}