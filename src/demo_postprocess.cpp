
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
    glObjectLabel(GL_TEXTURE, ColorTexture, -1, "ColorTexture");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ScreenWidth, ScreenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Note: Here we store the depth stencil in a renderbuffer object, 
    // but we can as well store it in a texture if we want to display it later
    GLuint DepthStencilRenderbuffer;
    glGenRenderbuffers(1, &DepthStencilRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, DepthStencilRenderbuffer);
    glObjectLabel(GL_RENDERBUFFER, DepthStencilRenderbuffer, -1, "DepthStencilRenderbuffer");
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ScreenWidth, ScreenHeight);
	
    // Setup attachements
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, ColorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, DepthStencilRenderbuffer);

	unsigned int DrawAttachments[1] = { GL_COLOR_ATTACHMENT0 + 0 };
	glDrawBuffers(1, DrawAttachments);

    GLenum FramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (FramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "demo_postprocess::framebuffer failed to complete (0x%x)\n", FramebufferStatus);
    }
    
	glBindFramebuffer(GL_FRAMEBUFFER, PreviousFramebuffer);

	demo_postprocess::framebuffer Framebuffer = {};
    Framebuffer.FBO = FBO;
    Framebuffer.ColorTexture = ColorTexture;
    Framebuffer.DepthStencilRenderbuffer = DepthStencilRenderbuffer;

    return Framebuffer;    
}

static void DeleteFramebuffer(const demo_postprocess::framebuffer& Framebuffer)
{
    glDeleteRenderbuffers(1, &Framebuffer.DepthStencilRenderbuffer);
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
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Uniforms
uniform float uTime;
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;
uniform mat4 uModelView;
uniform mat4 uModelViewProj;

// Varyings
out vec2 vUV;
out vec3 vViewPos;
out vec3 vViewNormal;
out mat4 vViewMatrix;

void main()
{
    vUV = aUV;
    vViewPos = (uModelView * vec4(aPosition, 1.0)).xyz;
    vViewMatrix = uView;
    vViewNormal = (uModelView * vec4(aNormal, 0.0)).xyz; // We should use the normal matrix in case of non-linear transforms
    gl_Position = uModelViewProj * vec4(aPosition, 1.0);
})GLSL";

static const char* gMeshFragmentShaderStr = R"GLSL(
// Uniforms
uniform sampler2D uColorTexture;
uniform float uTime;

// Varyings
in vec2 vUV;
in vec3 vViewPos;
in vec3 vViewNormal;
in mat4 vViewMatrix;

// Shader outputs
out vec3 oColor;

void main()
{
    oColor = texture(uColorTexture, vUV).rgb;
    
    // Apply a phong light shading (converting its position to modelView)
    // In real case, we do not use gDefaultLight and precompute the light ViewPosition
    light light = gDefaultLight; // Copy light and transform world pos to view
    light.position = vViewMatrix * gDefaultLight.position;
    oColor *= light_shade(light, vViewPos, normalize(vViewNormal));
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
    mat4 ViewTransform = CameraGetInverseMatrix(Camera);
    mat4 ModelTransform = Mat4::Translate({ 0.f, 0.f, -2.f });
    mat4 ModelViewTransform = ViewTransform * ModelTransform;
    mat4 ModelViewProjTransform = ProjectionTransform * ModelViewTransform;

    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uProjection"), 1, GL_FALSE, ProjectionTransform.e);
    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uView"), 1, GL_FALSE, ViewTransform.e);
    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uModel"), 1, GL_FALSE, ModelTransform.e);
    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uModelView"), 1, GL_FALSE, ModelViewTransform.e);
    glUniformMatrix4fv(glGetUniformLocation(Data.Program, "uModelViewProj"), 1, GL_FALSE, ModelViewProjTransform.e);

	glBindVertexArray(Data.VAO);
	glBindTexture(GL_TEXTURE_2D, Data.Texture);
	glDrawArrays(GL_TRIANGLES, Data.MeshStart, Data.MeshEnd);
}

// ================================================================================================
// SECOND PASS
// ================================================================================================
struct quad_vertex
{
    v2 Position;
    v2 UV;
};

static const char* gQuadVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;

// Varyings
out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = vec4(aPosition, 0.0, 1.0);
})GLSL";

static const char* gQuadFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;

// Uniforms
uniform sampler2D uColorTexture;
uniform float uTime;
uniform mat4 uColorTransform;

// Outputs
out vec4 oColor;

void main()
{
    // Treat color like a homogeneous vector
    vec4 rgb4 = vec4(texture(uColorTexture, vUV).rgb, 1.0);

    // Transform color
    oColor.rgb = (uColorTransform * rgb4).rgb;
    oColor.a = rgb4.a;
})GLSL";

static demo_postprocess::second_pass_data CreateSecondPass()
{
	demo_postprocess::second_pass_data Data = {};

    Data.Program = GL::CreateProgram(gQuadVertexShaderStr, gQuadFragmentShaderStr);

    // Gen unit quad
    {
        quad_vertex Quad[6] =
        {
            { {-1.f,-1.f }, { 0.f, 0.f } }, // bl
            { { 1.f,-1.f }, { 1.f, 0.f } }, // br
            { { 1.f, 1.f }, { 1.f, 1.f } }, // tr

            { {-1.f, 1.f }, { 0.f, 1.f } }, // tl
            { {-1.f,-1.f }, { 0.f, 0.f } }, // bl
            { { 1.f, 1.f }, { 1.f, 1.f } }, // tr
        };

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

static mat4 DebugUI_GetColorTransformMatrix()
{
    // Using static variable only for debug purpose!
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

    SecondPassData.ColorTransform = DebugUI_GetColorTransformMatrix();
}
