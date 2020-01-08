
#include <cstdio>

#include "platform.h"

#include "opengl_helpers.h"

using namespace GL;

GLuint GL::CompileShader(GLenum ShaderType, const char* ShaderStr)
{
    GLuint Shader = glCreateShader(ShaderType);

    glShaderSource(Shader, 1, &ShaderStr, nullptr);
    glCompileShader(Shader);

    GLint CompileStatus;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &CompileStatus);
    if (CompileStatus == GL_FALSE)
    {
        char Infolog[1024];
        glGetShaderInfoLog(Shader, ARRAY_SIZE(Infolog), nullptr, Infolog);
        fprintf(stderr, "Shader error: %s\n", Infolog);
    }

    return Shader;
}

GLuint GL::CreateProgram(const char* VSString, const char* FSString)
{
    GLuint Program = glCreateProgram();

    GLuint VertexShader   = GL::CompileShader(GL_VERTEX_SHADER, VSString);
    GLuint FragmentShader = GL::CompileShader(GL_FRAGMENT_SHADER, FSString);

    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);

    glLinkProgram(Program);

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    return Program;
}