
#include <cassert>
#include <vector>
#include <string>
#include <map>

#include "stb_image.h"
#include "platform.h"
#include "mesh.h"

#include "opengl_helpers.h"

using namespace GL;

// Light shader function
static const char* PhongLightingStr = R"GLSL(
// =================================
// PHONG SHADER START ===============
// Light structure
struct light
{
	bool enabled;
    vec4 viewPosition;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float attenuation[3];
};

struct material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
	float shininess;
};

// Default light
light gDefaultLight = light(
	true,
    vec4(1.0, 2.5, 0.0, 1.0),
    vec3(0.2, 0.2, 0.2),
    vec3(0.8, 0.8, 0.8),
    vec3(0.9, 0.9, 0.9),
    float[3](1.0, 0.0, 0.0));

// Default material
uniform material gDefaultMaterial = material(
    vec3(0.2, 0.2, 0.2),
    vec3(0.8, 0.8, 0.8),
    vec3(0.0, 0.0, 0.0),
    vec3(0.0, 0.0, 0.0),
    0.0);

// Phong shading function
// viewPosition : fragment position in view-space
//   viewNormal : fragment normal in view-space
vec3 light_shade(light light, material material, vec3 viewPosition, vec3 viewNormal)
{
	if (!light.enabled)
		return vec3(0.0);

    vec3 lightDir;
    float lightAttenuation = 1.0;
    if (light.viewPosition.w > 0.0)
    {
        // Point light
        vec3 lightPosFromVertexPos = (light.viewPosition.xyz / light.viewPosition.w) - viewPosition;
        lightDir = normalize(lightPosFromVertexPos);
        float dist = length(lightPosFromVertexPos);
        lightAttenuation = 1.0 / (light.attenuation[0] + light.attenuation[1]*dist + light.attenuation[2]*light.attenuation[2]*dist);
    }
    else
    {
        // Directional light
        lightDir = normalize(light.viewPosition.xyz);
    }

    if (lightAttenuation < 0.001)
        return vec3(0.0);

    vec3 viewDir  = normalize(-viewPosition);
	vec3 reflectDir = reflect(-lightDir, viewNormal);
	float specAngle = max(dot(reflectDir, viewDir), 0.0);

    vec3 ambient  = lightAttenuation * material.ambient  * light.ambient;
    vec3 diffuse  = lightAttenuation * material.diffuse  * light.diffuse  * max(dot(viewNormal, lightDir), 0.0);
    vec3 specular = lightAttenuation * material.specular * light.specular * (pow(specAngle, material.shininess / 4.0));
	specular = clamp(specular, 0.0, 1.0);
    
	return ambient + diffuse + specular;
}
// PHONG SHADER STOP ===============
// =================================
)GLSL";

void GL::UniformLight(GLuint Program, const char* LightUniformName, const light& Light)
{
	glUseProgram(Program);
	char UniformMemberName[255];

	sprintf(UniformMemberName, "%s.enabled", LightUniformName);
	glUniform1i(glGetUniformLocation(Program, UniformMemberName), Light.Enabled);

	sprintf(UniformMemberName, "%s.viewPosition", LightUniformName);
	glUniform4fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.ViewPosition.e);

	sprintf(UniformMemberName, "%s.ambient", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Ambient.e);

	sprintf(UniformMemberName, "%s.diffuse", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Diffuse.e);

	sprintf(UniformMemberName, "%s.specular", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Specular.e);

	sprintf(UniformMemberName, "%s.attenuation", LightUniformName);
	glUniform1fv(glGetUniformLocation(Program, UniformMemberName), 3, Light.Attenuation.e);
}

void UniformMaterial(GLuint Program, const char* MaterialUniformName, const material& Material)
{
	glUseProgram(Program);
	char UniformMemberName[255];

	sprintf(UniformMemberName, "%s.ambient", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Ambient.e);

	sprintf(UniformMemberName, "%s.diffuse", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Diffuse.e);

	sprintf(UniformMemberName, "%s.specular", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Specular.e);

	sprintf(UniformMemberName, "%s.emission", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Emission.e);

	sprintf(UniformMemberName, "%s.shininess", MaterialUniformName);
	glUniform1f(glGetUniformLocation(Program, UniformMemberName), Material.Shininess);
}

GLuint GL::CompileShaderEx(GLenum ShaderType, int ShaderStrsCount, const char** ShaderStrs, bool InjectLightShading)
{
	GLuint Shader = glCreateShader(ShaderType);

	std::vector<const char*> Sources;
	Sources.reserve(4);
	Sources.push_back("#version 330 core");

	if (InjectLightShading)
		Sources.push_back(PhongLightingStr);

	for (int i = 0; i < ShaderStrsCount; ++i)
		Sources.push_back(ShaderStrs[i]);

	glShaderSource(Shader, (GLsizei)Sources.size(), &Sources[0], nullptr);
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

GLuint GL::CompileShader(GLenum ShaderType, const char* ShaderStr, bool InjectLightShading)
{
	return GL::CompileShaderEx(ShaderType, 1, &ShaderStr, InjectLightShading);
}

GLuint GL::CreateProgramEx(int VSStringsCount, const char** VSStrings, int FSStringsCount, const char** FSStrings, bool InjectLightShading)
{
	GLuint Program = glCreateProgram();

	GLuint VertexShader = GL::CompileShaderEx(GL_VERTEX_SHADER, VSStringsCount, VSStrings);
	GLuint FragmentShader = GL::CompileShaderEx(GL_FRAGMENT_SHADER, FSStringsCount, FSStrings, InjectLightShading);

	glAttachShader(Program, VertexShader);
	glAttachShader(Program, FragmentShader);

	glLinkProgram(Program);

	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);

	return Program;
}

GLuint GL::CreateProgram(const char* VSString, const char* FSString, bool InjectLightShading)
{
	return GL::CreateProgramEx(1, &VSString, 1, &FSString, InjectLightShading);
}

void GL::UploadTexture(const char* Filename, int ImageFlags, int* WidthOut, int* HeightOut)
{
    // Flip
    stbi_set_flip_vertically_on_load((ImageFlags & IMG_FLIP) ? 1 : 0);

    // Desired channels
    int DesiredChannels = 0;
	int Channels = 0;
	if (ImageFlags & IMG_FORCE_RGB)
	{
		DesiredChannels = STBI_rgb;
		Channels = 3;
	}
	if (ImageFlags & IMG_FORCE_RGBA)
	{
		DesiredChannels = STBI_rgb_alpha;
		Channels = 4;
	}

    // Loading
    int Width, Height;
    uint8_t* Image = stbi_load(Filename, &Width, &Height, (DesiredChannels == 0) ? &Channels : nullptr, DesiredChannels);
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

struct GL::cache::data
{
	struct mesh
	{
		GLuint VertexBuffer;
		int Size;
	};

	struct texture_identifier
	{
		std::string Filename;
		int ImageFlags;

		bool operator<(const texture_identifier& Other) const
		{
			return Filename < Other.Filename;
		}
	};

	struct texture
	{
		GLuint TextureID;
		int Width;
		int Height;
	};

	std::vector<vertex_full> TmpBuffer;
	std::map<std::string, mesh> VertexBufferMap;
	std::map<texture_identifier, texture> TextureMap;
};

GL::cache::cache()
{
	Data = new cache::data();
}

GL::cache::~cache()
{
	for (const auto& KeyValue : Data->TextureMap)
		glDeleteTextures(1, &KeyValue.second.TextureID);

	for (const auto& KeyValue : Data->VertexBufferMap)
		glDeleteBuffers(1, &KeyValue.second.VertexBuffer);

	delete Data;
}

GLuint GL::cache::LoadObj(const char* Filename, float Scale, int* VertexCountOut)
{
	assert(Data != nullptr);

	auto Found = Data->VertexBufferMap.find(Filename);
	if (Found != Data->VertexBufferMap.end())
	{
		if (VertexCountOut)
			*VertexCountOut = Found->second.Size;
		return Found->second.VertexBuffer;
	}

	Data->TmpBuffer.clear();
	Mesh::LoadObjNoConvertion(Data->TmpBuffer, Filename, Scale);

	// Upload mesh to gpu
	GLuint MeshBuffer = 0;
	glGenBuffers(1, &MeshBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, MeshBuffer);
	glBufferData(GL_ARRAY_BUFFER, Data->TmpBuffer.size() * sizeof(vertex_full), &Data->TmpBuffer[0], GL_STATIC_DRAW);

	if (VertexCountOut)
		*VertexCountOut = (int)Data->TmpBuffer.size();
	
	Data->VertexBufferMap[Filename] = { MeshBuffer, (int)Data->TmpBuffer.size() };

	return MeshBuffer;
}

GLuint GL::cache::LoadTexture(const char* Filename, int ImageFlags, int* WidthOut, int* HeightOut)
{
	assert(Data != nullptr);

	GL::cache::data::texture_identifier TextureIdentifier = { Filename, ImageFlags };
	
	auto Found = Data->TextureMap.find(TextureIdentifier);
	if (Found != Data->TextureMap.end())
	{
		if (WidthOut)  *WidthOut  = Found->second.Width;
		if (HeightOut) *HeightOut = Found->second.Height;
		return Found->second.TextureID;
	}

	GLuint Texture;
	glGenTextures(1, &Texture);
	glBindTexture(GL_TEXTURE_2D, Texture);
	int Width, Height;
	GL::UploadTexture(Filename, ImageFlags, &Width, &Height);

	if (WidthOut)  *WidthOut  = Width;
	if (HeightOut) *HeightOut = Height;

	Data->TextureMap[TextureIdentifier] = { Texture, Width, Height };

	return Texture;
}

struct GL::debug::data
{
	GLuint WireframeProgram;
	GLuint WireframeVAO;
	std::vector<v3> BaryBufferData;
	GLuint BaryBuffer;
};

static const char* gWireframeVertexShaderStr = R"GLSL(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aBC;
uniform mat4 uModelViewProj;
out vec3 vBC;

void main()
{
    vBC = aBC;
    gl_Position = uModelViewProj * vec4(aPosition, 1.0);
})GLSL";

static const char* gWireframeFragmentShaderStr = R"GLSL(
in vec3 vBC;
out vec4 oColor;
uniform float uLineWidth = 1.25;
uniform vec4 uLineColor = vec4(1.0, 1.0, 1.0, 0.25);

float edgeFactor()
{
    vec3 d = fwidth(vBC);
    vec3 a3 = smoothstep(vec3(0.0), d * uLineWidth, vBC);
    return min(min(a3.x, a3.y), a3.z);
}

void main()
{
    oColor = vec4(uLineColor.rgb, uLineColor.a * (1.0 - edgeFactor()));
})GLSL";

GL::debug::debug()
{
	Data = new debug::data();
	Data->WireframeProgram = GL::CreateProgram(gWireframeVertexShaderStr, gWireframeFragmentShaderStr);
	glGenBuffers(1, &Data->BaryBuffer);
	glGenVertexArrays(1, &Data->WireframeVAO);
	glBindVertexArray(Data->WireframeVAO);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

GL::debug::~debug()
{
	glDeleteProgram(Data->WireframeProgram);
	glDeleteVertexArrays(1, &Data->WireframeVAO);
	glDeleteBuffers(1, &Data->BaryBuffer);
	delete Data;
}

void GL::debug::WireframePrepare(GLuint MeshVBO, GLsizei PositionStride, GLsizei PositionOffset, int VertexCount)
{
	assert(Data != nullptr);
	assert(VertexCount % 3 == 0);

	// Bind vertex array
	glBindVertexArray(Data->WireframeVAO);

	// Build more barycentric coordinates attributes if needed
	if (VertexCount > Data->BaryBufferData.size())
	{
		int OldSize = (int)Data->BaryBufferData.size();
		Data->BaryBufferData.resize(VertexCount);
		for (int i = OldSize; i < VertexCount; i += 3)
		{
			Data->BaryBufferData[i + 0] = { 1.f, 0.f, 0.f };
			Data->BaryBufferData[i + 1] = { 0.f, 1.f, 0.f };
			Data->BaryBufferData[i + 2] = { 0.f, 0.f, 1.f };
		}
		glBindBuffer(GL_ARRAY_BUFFER, Data->BaryBuffer);
		glBufferData(GL_ARRAY_BUFFER, Data->BaryBufferData.size() * sizeof(v3), &Data->BaryBufferData[0], GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}

	// Bind position buffer
	glBindBuffer(GL_ARRAY_BUFFER, MeshVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, PositionStride, (void*)(size_t)PositionOffset);

	// Setup rendering
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GL::debug::WireframeDrawArray(GLint First, GLsizei Count, const mat4& MVP)
{
	assert(Data != nullptr);

	glUseProgram(Data->WireframeProgram);
	//glUniform1f(glGetUniformLocation(Data->WireframeShader, "uLineWidth"), LineWidth);
	//glUniform4fv(glGetUniformLocation(Data->WireframeShader, "uLineColor"), 1, LineColor.e);
	glUniformMatrix4fv(glGetUniformLocation(Data->WireframeProgram, "uModelViewProj"), 1, GL_FALSE, MVP.e);
	glDrawArrays(GL_TRIANGLES, First, Count);
}
