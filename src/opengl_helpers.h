#include "opengl_headers.h"
#include "types.h"


namespace GL
{
    GLuint CompileShader(GLenum ShaderType, const char* ShaderStr);
    GLuint CreateProgram(const char* VSString, const char* FSString);
    
    struct Light
    {
        v4 position;
        v3 ambient;
        v3 diffuse;
        v3 specular;
        float shininess;
        float attenuation[3];
    };
}