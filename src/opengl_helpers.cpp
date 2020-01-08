
#include <cstdio>

#include "platform.h"

#include "opengl_helpers.h"

using namespace GL;

const char* PhongLightingStr = R"GLSL(
struct Light
{
    bool enabled;
    vec4 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float attenuation[3];
};

Light gDefaultLight = Light(
    true,
    vec4(1.0, 2.5, 0.0, 1.0),
    vec3(0.2, 0.2, 0.2),
    vec3(0.8, 0.8, 0.8),
    vec3(0.9, 0.9, 0.9),
    64.0,
    float[3](1.0, 0.0, 0.0));

vec3 shade(Light light, vec3 position, vec3 normal)
{
    vec3 lightDir;
    float dist;
    float attenuation = 1.0;
    if (light.position.w > 0.0)
    {
        // Point light
        vec3 lightPosFromVertexPos = (light.position.xyz / light.position.w) - position;
        lightDir = normalize(lightPosFromVertexPos);
        dist = length(lightPosFromVertexPos);
        attenuation = 1.0 / (light.attenuation[0] + light.attenuation[1]*dist + light.attenuation[2]*light.attenuation[2]*dist);
    }
    else
    {
        // Directional light
        lightDir = normalize(light.position.xyz);
    }

    if (attenuation < 0.001)
        return vec3(0.0, 0.0, 0.0);

    vec3 viewDir  = normalize(-position);
    vec3 halfDir  = normalize(lightDir + viewDir);
    float specAngle = max(dot(halfDir, normal), 0.f);

    vec3 ambient  = light.ambient;
    vec3 diffuse  = light.diffuse  * max(dot(normal, lightDir), 0.f);
    vec3 specular = light.specular * pow(specAngle, light.shininess);
    
    //return clamp(ambient + diffuse + specular, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f});
    return attenuation * (ambient + diffuse + specular);
}
)GLSL";

GLuint GL::CompileShader(GLenum ShaderType, const char* ShaderStr)
{
    GLuint Shader = glCreateShader(ShaderType);

    const char* Sources[] = {
        "#version 330 core",
        PhongLightingStr,
        ShaderStr,
    };

    glShaderSource(Shader, ARRAY_SIZE(Sources), Sources, nullptr);
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