
#include <cstdio>
#include <vector>

#include "imgui.h"
#include "stb_image.h"
#include "platform.h"

#include "opengl_helpers.h"

using namespace GL;

// Light shader function
static const char* PhongLightingStr = R"GLSL(
// =================================
// PHONG SHADER START ===============
// Light structure
struct light
{
    vec4 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float attenuation[3];
};

// Default light
light gDefaultLight = light(
    vec4(1.0, 2.5, 0.0, 1.0),
    vec3(0.2, 0.2, 0.2),
    vec3(0.8, 0.8, 0.8),
    vec3(0.9, 0.9, 0.9),
    64.0,
    float[3](1.0, 0.0, 0.0));

// Phong shading function
vec3 light_shade(light light, vec3 position, vec3 normal)
{
    vec3 lightDir;
    float attenuation = 1.0;
    if (light.position.w > 0.0)
    {
        // Point light
        vec3 lightPosFromVertexPos = (light.position.xyz / light.position.w) - position;
        lightDir = normalize(lightPosFromVertexPos);
        float dist = length(lightPosFromVertexPos);
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
// PHONG SHADER STOP ===============
// =================================
)GLSL";

GLuint GL::CompileShader(GLenum ShaderType, const char* ShaderStr, bool InjectLightShading)
{
    GLuint Shader = glCreateShader(ShaderType);

    const char* Sources[] = {
        "#version 330 core",
		InjectLightShading ? PhongLightingStr : "",
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

GLuint GL::CreateProgram(const char* VSString, const char* FSString, bool InjectLightShading)
{
    GLuint Program = glCreateProgram();

    GLuint VertexShader         = GL::CompileShader(GL_VERTEX_SHADER, VSString);
    GLuint FragmentShader       = GL::CompileShader(GL_FRAGMENT_SHADER, FSString, InjectLightShading);

    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);

    glLinkProgram(Program);

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    return Program;
}

void GL::UploadTexture(const char* Filename, int ImageFlags, int* WidthOut, int* HeightOut)
{
    // Flip
    stbi_set_flip_vertically_on_load((ImageFlags & IMG_FLIP) ? 1 : 0);

    // Desired channels
    int DesiredChannels = 0;
    if (ImageFlags & IMG_FORCE_RGB)
        DesiredChannels = STBI_rgb;
    if (ImageFlags & IMG_FORCE_RGBA)
        DesiredChannels = STBI_rgb_alpha;

    // Loading
    int Width, Height, Channels;
    uint8_t* Image = stbi_load(Filename, &Width, &Height, &Channels, DesiredChannels);
    if (Image == nullptr)
    {
        fprintf(stderr, "Image loading failed on '%s'\n", Filename);
        return;
    }

    GLint Format = (Channels == 3) ? GL_RGB : GL_RGBA;
    
    // Uploading
    glTexImage2D(GL_TEXTURE_2D, 0, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Image);
    stbi_image_free(Image);

    // Mipmaps
    if (ImageFlags & IMG_GEN_MIPMAPS)
        glGenerateMipmap(GL_TEXTURE_2D);

    if (WidthOut)
        *WidthOut = Width;

    if (HeightOut)
        *HeightOut = Height;

    stbi_set_flip_vertically_on_load(0); // Always reset to default value
}

void GL::UploadCheckerboardTexture(int Width, int Height, int SquareSize)
{
	std::vector<v4> Texels(Width * Height);

	for (int y = 0; y < Height; ++y)
	{
		for (int x = 0; x < Width; ++x)
		{
			int PixelIndex = x + y * Width;
			int TileX = x / SquareSize;
			int TileY = y / SquareSize;
			Texels[PixelIndex] = ((TileX + TileY) % 2) ? v4{ 0.1f, 0.1f, 0.1f, 1.f } : v4{ 0.7f, 0.7f, 0.7f, 1.f };
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Width, 0, GL_RGBA, GL_FLOAT, &Texels[0]);
}

static void DebugGLBoolText(const char* Name, GLint value, bool color = true)
{
	ImGui::Text("%s:", Name);
	ImGui::SameLine();
	if (color) ImGui::PushStyleColor(ImGuiCol_Text, value ? ImVec4(0.0f, 1.f, 0.f, 1.f) : ImVec4(1.0f, 0.f, 0.f, 1.f));
	ImGui::Text("%s", value ? "GL_TRUE" : "GL_FALSE");
	if (color) ImGui::PopStyleColor();
}

static void DebugShader(GLuint program, GLuint shader)
{
	static GLuint shaderEdited = 0;
	static GLuint prevShaderEdited = -1;
	static int shaderSourceBufferSize = 10 * 1024;
	static char* shaderSourceBuffer = (char*)malloc(shaderSourceBufferSize); // Memory leak here

	GLint shaderType;
	glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);

	const char* shaderTypeStr = "";
	switch (shaderType)
	{
	case GL_VERTEX_SHADER:   shaderTypeStr = "GL_VERTEX_SHADER"; break;
	case GL_FRAGMENT_SHADER: shaderTypeStr = "GL_FRAGMENT_SHADER"; break;
	case GL_GEOMETRY_SHADER: shaderTypeStr = "GL_GEOMETRY_SHADER"; break;
	default: shaderTypeStr = "UNKNOWN_SHADER_TYPE";
	}

	if (ImGui::TreeNode((void*)(intptr_t)shader, "%s (id = %d)", shaderTypeStr, shader))
	{
		if (ImGui::Button("Edit"))
			shaderEdited = shader;

		if (ImGui::TreeNode("Status"))
		{
			GLint deleteStatus;
			glGetShaderiv(shader, GL_DELETE_STATUS, &deleteStatus);
			DebugGLBoolText("Delete status", deleteStatus, false);

			GLint compileStatus;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
			DebugGLBoolText("Compile status", compileStatus);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Info log"))
		{
			const GLuint infoLogMaxLength = 1024;
			static GLchar infoLogBuffer[infoLogMaxLength];

			GLint infoLogLength;
			glGetShaderInfoLog(shader, infoLogMaxLength, &infoLogLength, infoLogBuffer);
			if (infoLogLength)
				ImGui::InputTextMultiline("Log", (char*)infoLogBuffer, infoLogLength, ImVec2(-20.f, -40.f), ImGuiInputTextFlags_ReadOnly);
			else
				ImGui::Text("(empty)");

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	if (shader == shaderEdited)
	{
		if (shaderEdited != prevShaderEdited)
		{
			GLint shaderSourceLength;
			glGetShaderiv(shaderEdited, GL_SHADER_SOURCE_LENGTH, &shaderSourceLength);
			ImGui::Text("Shader len: %d", shaderSourceLength);

			if (shaderSourceBufferSize < shaderSourceLength) {
				shaderSourceBuffer = (char*)realloc(shaderSourceBuffer, shaderSourceLength);
				shaderSourceBufferSize = shaderSourceLength;
			}

			glGetShaderSource(shaderEdited, shaderSourceBufferSize, &shaderSourceLength, shaderSourceBuffer);
			prevShaderEdited = shaderEdited;
		}

		ImGui::Begin("Shader");
		{
			ImGui::InputTextMultiline("Source", shaderSourceBuffer, shaderSourceBufferSize, ImVec2(600, 600));

			if (ImGui::Button("Recompile"))
			{
				glShaderSource(shaderEdited, 1, (const GLchar* const*)&shaderSourceBuffer, nullptr);
				glCompileShader(shaderEdited);
				GLint success = 0;
				glGetShaderiv(shaderEdited, GL_COMPILE_STATUS, &success);
				if (success > 0)
					glLinkProgram(program);
			}
			ImGui::SameLine();
			if (ImGui::Button("Close"))
			{
				shaderEdited = 0;
			}
		}
		ImGui::End();
	}
}

void GLImGui::InspectProgram(GLuint program)
{
	GLint previousProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
	glUseProgram(program);

	if (ImGui::TreeNode("Status"))
	{
		GLint deleteStatus;
		glGetProgramiv(program, GL_DELETE_STATUS, &deleteStatus);
		DebugGLBoolText("Delete status", deleteStatus, false);

		GLint linkStatus;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		DebugGLBoolText("Link status", linkStatus);

		GLint validateStatus;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &validateStatus);
		DebugGLBoolText("Validate status", validateStatus);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Info log"))
	{
		const GLuint infoLogMaxLength = 1024;
		static GLchar infoLogBuffer[infoLogMaxLength];

		GLint infoLogLength;
		glGetProgramInfoLog(program, infoLogMaxLength, &infoLogLength, infoLogBuffer);
		if (infoLogLength)
			ImGui::InputTextMultiline("Log", (char*)infoLogBuffer, infoLogLength, ImVec2(-20.f, -40.f), ImGuiInputTextFlags_ReadOnly);
		else
			ImGui::Text("(empty)");

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Attributes"))
	{
		GLint activeAttributes;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &activeAttributes);

		for (int index = 0; index < activeAttributes; ++index)
		{
			GLsizei length;
			GLint size;
			GLenum type;
			GLchar name[1024];

			glGetActiveAttrib(program, index, 1024, &length, &size, &type, name);
			if (length == 0)
				break;

			GLint location = glGetAttribLocation(program, name);

			ImGui::Text("[%d:%s], size=%d, type=0x%x", location, name, size, type);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Uniform blocks"))
	{
		GLint activeUniformBlocks;
		glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &activeUniformBlocks);

		for (int blockIndex = 0; blockIndex < activeUniformBlocks; ++blockIndex)
		{
			ImGui::PushID(blockIndex);

			char blockName[256];
			glGetActiveUniformBlockName(program, blockIndex, 256, nullptr, blockName);

			GLint dataSize;
			glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);

			GLuint bufferBinding;
			glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, blockIndex, (GLint*)&bufferBinding);

			if (ImGui::TreeNode("", "[%d:%s], bufferBinding=%d, size=%d", blockIndex, blockName, bufferBinding, dataSize))
			{
				GLint blockBinding, nameLength, activeUniforms;
				glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_BINDING, &blockBinding);
				glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLength);
				glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &activeUniforms);

				ImGui::Text(" - blockBinding=%d", blockBinding);
				ImGui::Text(" - nameLength=%d", nameLength);
				if (ImGui::TreeNode("ACTIVE_UNIFORMS", "Active uniforms (count=%d)", activeUniforms))
				{
					if (activeUniforms < 256)
					{
						GLint activeUniformIndices[256];
						glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, activeUniformIndices);
						for (int i = 0; i < activeUniforms; ++i)
						{
							ImGui::PushID(i);
							GLint uniformIndex = activeUniformIndices[i];

							char uniformName[256];
							glGetActiveUniformName(program, uniformIndex, 256, nullptr, uniformName);
							if (ImGui::TreeNode("", "[%d:%s]", uniformIndex, uniformName))
							{

								ImGui::TreePop();
							}
							ImGui::PopID();
						}
					}
					else
					{
						ImGui::Text("[Too many uniforms to display]");
					}
					ImGui::TreePop();
				}

				if (bufferBinding == 0)
				{
					if (ImGui::Button("Bind empty buffer"))
					{
						char* data = (char*)calloc(dataSize, 1);
						glGenBuffers(1, &bufferBinding);
						glBindBuffer(GL_UNIFORM_BUFFER, bufferBinding);
						glBufferData(GL_UNIFORM_BUFFER, dataSize, data, GL_STATIC_DRAW);
						glBindBuffer(GL_UNIFORM_BUFFER, 0);
						free(data);

						glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, bufferBinding);
					}
				}
				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Uniforms"))
	{
		GLint activeUniforms;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniforms);

		static bool AsColor = true;
		ImGui::Separator();
		ImGui::Checkbox("View vec3 as color", &AsColor);
		ImGui::Separator();

		for (GLuint index = 0; index < (GLuint)activeUniforms; ++index)
		{
			ImGui::PushID(index);

			GLsizei length;
			GLint size;
			GLenum type;
			GLchar name[1024];

			glGetActiveUniform(program, index, 1024, &length, &size, &type, name);
			if (length == 0)
				break;

			GLuint bufferBinding = 0;
			GLint blockIndex;
			GLint blockOffset = 0;
			GLint blockSize = 0;
			glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_BLOCK_INDEX, &blockIndex);
			if (blockIndex != -1)
			{
				glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_OFFSET, &blockOffset);
				glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_SIZE, &blockSize);
				glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, blockIndex, (GLint*)&bufferBinding);
			}

			GLint uniformSize;
			glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_SIZE, &uniformSize);

			GLchar arrayName[1024];
			if (uniformSize > 1)
			{
				strcpy(arrayName, name);
				arrayName[strlen(arrayName) - 3] = '\0';
			}

			// ===========================
			// TODO: Remove this crazyness (especially uniform array access)
			// ===========================
			for (int uniformArrayIndex = 0; uniformArrayIndex < uniformSize; ++uniformArrayIndex)
			{
				if (uniformSize > 1)
				{
					sprintf(name, "%s[%d]", arrayName, uniformArrayIndex);
				}

				GLint location = -1;
				location = glGetUniformLocation(program, name);
				ImGui::Text("[%d:%s] location=%d, size=%d, type=0x%x, blockIndex=%d, bufferBinding=%d", index, name, location, size, type, blockIndex, bufferBinding);

				if (location != -1)
				{
					if (type == GL_BOOL)
					{
						GLint currentValue;
						glGetUniformiv(program, location, &currentValue);
						if (ImGui::Checkbox(name, (bool*)&currentValue))
							glUniform1i(location, currentValue);
					}
					else if (type == GL_FLOAT)
					{
						GLfloat currentValue;
						glGetUniformfv(program, location, &currentValue);
						if (ImGui::DragFloat(name, &currentValue, 0.001f))
							glUniform1f(location, currentValue);
					}
					else if (type == GL_FLOAT_VEC2)
					{
						GLfloat currentValue[2];
						glGetUniformfv(program, location, currentValue);
						if (ImGui::DragFloat2(name, currentValue, 0.001f))
							glUniform2f(location, currentValue[0], currentValue[1]);
					}
					else if (type == GL_FLOAT_VEC3)
					{
						GLfloat currentValue[3];
						glGetUniformfv(program, location, currentValue);
						if ((!AsColor && ImGui::DragFloat3(name, currentValue, 0.001f)) ||
							(AsColor && ImGui::ColorEdit3(name, currentValue)))
							glUniform3f(location, currentValue[0], currentValue[1], currentValue[2]);
					}
					else if (type == GL_FLOAT_VEC4)
					{
						GLfloat currentValue[4];
						glGetUniformfv(program, location, currentValue);
						if (ImGui::DragFloat4(name, currentValue, 0.001f))
							glUniform4f(location, currentValue[0], currentValue[1], currentValue[2], currentValue[3]);
					}
				}
				else if (bufferBinding != 0)
				{
					glBindBuffer(GL_UNIFORM_BUFFER, bufferBinding);
					if (type == GL_FLOAT)
					{
						GLfloat currentValue;
						glGetBufferSubData(GL_UNIFORM_BUFFER, blockOffset + uniformArrayIndex * sizeof(GLfloat), sizeof(GLfloat), &currentValue);
						if (ImGui::DragFloat(name, &currentValue, 0.001f))
							glBufferSubData(GL_UNIFORM_BUFFER, blockOffset + uniformArrayIndex * sizeof(GLfloat), sizeof(GLfloat), &currentValue);
					}
					else if (type == GL_FLOAT_VEC2)
					{
						GLfloat currentValue[2];
						glGetBufferSubData(GL_UNIFORM_BUFFER, blockOffset + uniformArrayIndex * sizeof(GLfloat), 2 * sizeof(GLfloat), &currentValue);
						if (ImGui::DragFloat3(name, currentValue, 0.001f))
							glBufferSubData(GL_UNIFORM_BUFFER, blockOffset + uniformArrayIndex * sizeof(GLfloat), 2 * sizeof(GLfloat), &currentValue);
					}
					else if (type == GL_FLOAT_VEC3)
					{
						GLfloat currentValue[3];
						glGetBufferSubData(GL_UNIFORM_BUFFER, blockOffset + uniformArrayIndex * sizeof(GLfloat), 3 * sizeof(GLfloat), &currentValue);
						if ((!AsColor && ImGui::DragFloat3(name, currentValue, 0.001f)) ||
							(AsColor && ImGui::ColorEdit3(name, currentValue)))
							glBufferSubData(GL_UNIFORM_BUFFER, blockOffset + uniformArrayIndex * sizeof(GLfloat), 3 * sizeof(GLfloat), &currentValue);
					}
					else if (type == GL_FLOAT_VEC4)
					{
						GLfloat currentValue[4];
						glGetBufferSubData(GL_UNIFORM_BUFFER, blockOffset + uniformArrayIndex * sizeof(GLfloat), 4 * sizeof(GLfloat), &currentValue);
						if (ImGui::DragFloat3(name, currentValue, 0.001f))
							glBufferSubData(GL_UNIFORM_BUFFER, blockOffset + uniformArrayIndex * sizeof(GLfloat), 4 * sizeof(GLfloat), &currentValue);
					}
					glBindBuffer(GL_UNIFORM_BUFFER, 0);
				}
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Shaders"))
	{
		GLsizei count;
		GLuint  shaders[4];

		glGetAttachedShaders(program, 4, &count, shaders);
		for (int i = 0; i < count; ++i)
		{
			DebugShader(program, shaders[i]);
		}

		ImGui::TreePop();
	}

	glUseProgram(previousProgram);
}