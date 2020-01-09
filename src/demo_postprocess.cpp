
#include <cstdio>
#include <vector>

#include "imgui.h"
#include "opengl_helpers.h"
#include "platform.h"
#include "mesh.h"
#include "maths.h"

#include "demo_postprocess.h"

// ================================================================================================
// FRAMEBUFFER
// ================================================================================================
static demo_postprocess::framebuffer CreateFramebuffer(int ScreenWidth, int ScreenHeight)
{
	GLint PreviousFramebuffer;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &PreviousFramebuffer);

	// Create Framebuffer that will hold 1 color attachement
	GLuint FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    
	// Create texture that will be used as color attachment
	GLuint ColorTexture;
	glGenTextures(1, &ColorTexture);
	glBindTexture(GL_TEXTURE_2D, ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ScreenWidth, ScreenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint DepthStencil;
    glGenTextures(1, &DepthStencil);
    glBindTexture(GL_TEXTURE_2D, DepthStencil);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, ScreenWidth, ScreenHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Setup attachement
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, ColorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DepthStencil, 0);

	unsigned int DrawAttachments[2] = { GL_COLOR_ATTACHMENT0 + 0, GL_COLOR_ATTACHMENT0 + 1 };
	glDrawBuffers(2, DrawAttachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fprintf(stderr, "demo_postprocess::framebuffer failed to complete\n");
    
	glBindFramebuffer(GL_FRAMEBUFFER, PreviousFramebuffer);

	demo_postprocess::framebuffer Framebuffer = {};
    Framebuffer.FBO = FBO;
    Framebuffer.ColorTexture = ColorTexture;
    Framebuffer.DepthStencilTexture = DepthStencil;

    return Framebuffer;    
}

static void DeleteFramebuffer(const demo_postprocess::framebuffer& Framebuffer)
{
	glDeleteTextures(1, &Framebuffer.ColorTexture);
	glDeleteFramebuffers(1, &Framebuffer.FBO);
}

// ================================================================================================
// FIRST PASS
// ================================================================================================
struct mesh_vertex
{
    v3 Position;
    v2 UV;
    v3 Normal;
};

static const char* gMeshVertexShaderStr = R"GLSL(
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

uniform float uTime;
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

out vec2 aUV;
out vec3 aViewPos;
out vec3 aViewNormal;
out mat4 aViewMatrix;

void main()
{
    aUV = uv;
    mat4 modelView = uView * uModel;
    aViewPos = (modelView * vec4(position, 1.0)).xyz;
    aViewNormal = (modelView * vec4(normal, 0.0)).xyz;
    aViewMatrix = uView;
    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
})GLSL";

static const char* gMeshFragmentShaderStr = R"GLSL(
uniform sampler2D colorTexture;
in vec2 aUV;
in vec3 aViewPos;
in vec3 aViewNormal;
in mat4 aViewMatrix;
out vec3 color;
uniform float uTime;

void main()
{
    color = texture(colorTexture, aUV).rgb;
    
    // Apply a phong light shading (converting its position to modelView)
    // In real case, we do not use gDefaultLight and precompute the light ViewPosition
    light light = gDefaultLight;
    light.position = aViewMatrix * gDefaultLight.position;
    color *= light_shade(light, aViewPos, normalize(aViewNormal));
})GLSL";

static demo_postprocess::first_pass_data CreateFirstPass()
{
	demo_postprocess::first_pass_data Data = {};

	Data.Program = GL::CreateProgram(gMeshVertexShaderStr, gMeshFragmentShaderStr, true);
    // Gen mesh
    {
        std::vector<mesh_vertex> VerticeBuffer(20480);

        mesh_vertex* VerticesStart = &VerticeBuffer[0];
        mesh_vertex* VerticesEnd = VerticesStart + VerticeBuffer.size();

        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(mesh_vertex);
        Descriptor.HasUV = true;
        Descriptor.HasNormal = true;
        Descriptor.PositionOffset = OFFSETOF(mesh_vertex, Position);
        Descriptor.UVOffset = OFFSETOF(mesh_vertex, UV);
        Descriptor.NormalOffset = OFFSETOF(mesh_vertex, Normal);

        mesh_vertex* Cur = VerticesStart;

        // Add a scaled down cube
        // This...
        Cur = (mesh_vertex*)Mesh::Transform(
            Cur, Mesh::BuildCube(Cur, VerticesEnd, Descriptor),
            Descriptor, Mat4::Scale({ 0.5f, 0.5f, 0.5f }));

        // Add a obj
        Cur = (mesh_vertex*)Mesh::Transform(
            Cur, Mesh::LoadObj(Cur, VerticesEnd, Descriptor, "media/teapot.obj", 0.1f),
            Descriptor, Mat4::Translate({ 0.f, 0.25f, 0.f }));

        // Add a sphere
        Cur = (mesh_vertex*)Mesh::Transform(
            Cur, Mesh::BuildSphere(Cur, VerticesEnd, Descriptor, 8, 8),
            Descriptor, Mat4::Translate({ 1.f, 0.f, 0.f }) * Mat4::Scale({ 0.5f, 0.5f, 0.5f }));

        // Calculate vertex count (we need it to call glDrawArrays)
        Data.MeshEnd = (int)(Cur - VerticesStart) - Data.MeshStart;

        // Upload mesh to gpu
        glGenBuffers(1, &Data.VertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, Data.VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, (int)(Cur - VerticesStart) * sizeof(mesh_vertex), VerticesStart, GL_STATIC_DRAW);
    }

    // Gen texture
    {
        glGenTextures(1, &Data.Texture);
        glBindTexture(GL_TEXTURE_2D, Data.Texture);
        //GL::UploadCheckerboardTexture(64, 64, 8);
        GL::UploadTexture("media/ss_ft.tga");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Create the vertex array
    {
        glGenVertexArrays(1, &Data.VAO);
        glBindVertexArray(Data.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, Data.VertexBuffer);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(mesh_vertex), (void*)OFFSETOF(mesh_vertex, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(mesh_vertex), (void*)OFFSETOF(mesh_vertex, UV));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(mesh_vertex), (void*)OFFSETOF(mesh_vertex, Normal));
        glBindVertexArray(0);
    }

	return Data;
}

static void DeleteFirstPass(const demo_postprocess::first_pass_data& Data)
{
	glDeleteTextures(1, &Data.Texture);
	glDeleteBuffers(1, &Data.VertexBuffer);
	glDeleteVertexArrays(1, &Data.VAO);
}

static void DrawFirstPass(const demo_postprocess::first_pass_data& Data, const platform_io& IO, const camera& Camera)
{
    const float AspectRatio = (float)IO.ScreenWidth / (float)IO.ScreenHeight;

    glUseProgram(Data.Program);

    glUniform1f(glGetUniformLocation(Data.Program, "uTime"), (float)IO.Time);

    mat4 ProjectionTransform = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uProjection"), 1, GL_FALSE, ProjectionTransform.e);

    mat4 ViewTransform = CameraGetInverseMatrix(Camera);
    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uView"), 1, GL_FALSE, ViewTransform.e);

    mat4 MeshTransform = Mat4::Translate({ 0.f, 0.f, -2.f });
    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uModel"), 1, GL_FALSE, MeshTransform.e);

	glBindVertexArray(Data.VAO);
	glBindTexture(GL_TEXTURE_2D, Data.Texture);
	glDrawArrays(GL_TRIANGLES, Data.MeshStart, Data.MeshEnd);
}

// ================================================================================================
// SECOND PASS
// ================================================================================================
struct quad_vertex
{
    v3 Position;
    v2 UV;
};

static const char* gQuadVertexShaderStr = R"GLSL(
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
out vec2 aUV;

void main()
{
    aUV = uv;
    gl_Position = vec4(position, 1.0);
})GLSL";

static const char* gQuadFragmentShaderStr = R"GLSL(
in vec2 aUV;
out vec4 color;
uniform sampler2D colorTexture;
uniform float uTime;
uniform mat4 uColorTransform;

void main()
{
    // Treat color like a homogeneous vector
    vec4 rgb4 = vec4(texture(colorTexture, aUV).rgb, 1.0);

    // Transform color
    color.rgb = (uColorTransform * rgb4).rgb;
    color.a = rgb4.a;
})GLSL";

static demo_postprocess::second_pass_data CreateSecondPass()
{
	demo_postprocess::second_pass_data Data = {};

    Data.Program = GL::CreateProgram(gQuadVertexShaderStr, gQuadFragmentShaderStr);

    // Gen mesh
    {
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(quad_vertex);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(quad_vertex, Position);
        Descriptor.UVOffset = OFFSETOF(quad_vertex, UV);

        mesh_vertex Quad[6];

        Mesh::Transform(Quad, Mesh::BuildQuad(Quad, Quad + 6, Descriptor), Descriptor, Mat4::Scale({ 2.f, 2.f, 2.f }));

        // Upload mesh to gpu
        glGenBuffers(1, &Data.VertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, Data.VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(quad_vertex), Quad, GL_STATIC_DRAW);
    }

    // Create a vertex array and bind it
    glGenVertexArrays(1, &Data.VAO);

    glBindVertexArray(Data.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, Data.VertexBuffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(quad_vertex), (void*)OFFSETOF(quad_vertex, Position));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(quad_vertex), (void*)OFFSETOF(quad_vertex, UV));
    glBindVertexArray(0);

	return Data;
}

static void DeleteSecondPass(const demo_postprocess::second_pass_data& Data)
{
	glDeleteBuffers(1, &Data.VertexBuffer);
	glDeleteVertexArrays(1, &Data.VAO);
}

static void DrawSecondPass(const demo_postprocess::second_pass_data& Data, const platform_io& IO, GLuint Texture)
{
    glUseProgram(Data.Program);

    glUniform1f(glGetUniformLocation(Data.Program, "uTime"), (float)IO.Time);
    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uColorTransform"), 1, GL_FALSE, Data.ColorTransform.e);

    glBindTexture(GL_TEXTURE_2D, Texture);
	glBindVertexArray(Data.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

// ================================================================================================
// DEMO POSTPROCESS
// ================================================================================================
demo_postprocess::demo_postprocess(const platform_io& IO)
{
    Framebuffer = CreateFramebuffer(IO.ScreenWidth, IO.ScreenHeight);
	FirstPassData = CreateFirstPass();
	SecondPassData = CreateSecondPass();
}

demo_postprocess::~demo_postprocess()
{
	DeleteSecondPass(SecondPassData);
	DeleteFirstPass(FirstPassData);
	DeleteFramebuffer(Framebuffer);
}

enum class color_transform : int
{
    IDENTITY,
    GRAYSCALE,
    SEPIA,
    INVERSE,
    CUSTOM
};

static mat4 GetColorTransformMatrix()
{
    static color_transform ColorTransformMode = color_transform::IDENTITY;
    ImGui::RadioButton("Identity",  (int*)&ColorTransformMode, (int)color_transform::IDENTITY);
    ImGui::RadioButton("GrayScale", (int*)&ColorTransformMode, (int)color_transform::GRAYSCALE);
    ImGui::RadioButton("Sepia",     (int*)&ColorTransformMode, (int)color_transform::SEPIA);
    ImGui::RadioButton("Inverse",   (int*)&ColorTransformMode, (int)color_transform::INVERSE);
    ImGui::RadioButton("Custom",    (int*)&ColorTransformMode, (int)color_transform::CUSTOM);

    switch (ColorTransformMode)
    {
    default:
    case color_transform::IDENTITY:
        return Mat4::Identity();

    case color_transform::GRAYSCALE:
    {
        static v3 GrayScaleColorFactors = { 0.299f, 0.587f, 0.114f };
        ImGui::SliderFloat3("GrayScaleColorFactors", GrayScaleColorFactors.e, 0.f, 1.f);
        return
        {
            GrayScaleColorFactors.x, GrayScaleColorFactors.x, GrayScaleColorFactors.x, 0.f,
            GrayScaleColorFactors.y, GrayScaleColorFactors.y, GrayScaleColorFactors.y, 0.f,
            GrayScaleColorFactors.z, GrayScaleColorFactors.z, GrayScaleColorFactors.z, 0.f,
            0.f, 0.f, 0.f, 1.f,
        };
    }

    case color_transform::SEPIA:
        return
        {
            0.393f, 0.349f, 0.272f, 0.f,
            0.769f, 0.686f, 0.534f, 0.f,
            0.189f, 0.168f, 0.131f, 0.f,
            0.f, 0.f, 0.f, 1.f,
        };

    case color_transform::INVERSE:
        return
        {
            -1.f,0.f, 0.f, 0.f,
            0.f,-1.f, 0.f, 0.f,
            0.f, 0.f,-1.f, 0.f,
            1.f, 1.f, 1.f, 1.f,
        };

    case color_transform::CUSTOM:
        return
        {
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            1.f, 0.f, 0.f, 0.f,
            0.f, 0.f, 0.f, 1.f,
        };
    }
}

void demo_postprocess::Update(const platform_io& IO)
{
    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

	// Clear screen
	// Keep track previous framebuffer (should be 0)
	GLint PreviousFramebuffer;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &PreviousFramebuffer);

	// First pass, render geometry inside FBO
    {
        glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.FBO);

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.3f, 0.3f, 0.3f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        DrawFirstPass(FirstPassData, IO, Camera);
    }

	// Second pass render FBO.ColorTexture to screen with postprocess shader
    {
        glBindFramebuffer(GL_FRAMEBUFFER, PreviousFramebuffer);

        glDisable(GL_DEPTH_TEST);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        DrawSecondPass(SecondPassData, IO, Framebuffer.ColorTexture);
    }

    // Custom editor with ImGui
    if (ImGui::TreeNode("FirstPassData.Program"))
    {
        GLImGui::InspectProgram(FirstPassData.Program);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("SecondPassData.Program"))
    {
        GLImGui::InspectProgram(SecondPassData.Program);
        ImGui::TreePop();
    }

    SecondPassData.ColorTransform = GetColorTransformMatrix();
}
