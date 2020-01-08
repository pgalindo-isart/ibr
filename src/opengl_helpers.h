#include "opengl_headers.h"

namespace GL
{
    GLuint CompileShader(GLenum ShaderType, const char* ShaderStr);
    GLuint CreateProgram(const char* VSString, const char* FSString);
}