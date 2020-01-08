
#include "opengl_helpers.h"

using namespace GL;

GLuint GL::CompileShader(GLenum ShaderType, const char* ShaderStr)
{
    GLuint Shader = glCreateShader(ShaderType);

    // TODO: Check shader compilation
    glShaderSource(Shader, 1, &ShaderStr, nullptr);
    glCompileShader(Shader);

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