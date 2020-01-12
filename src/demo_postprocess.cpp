
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ScreenWidth, ScreenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
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
// POSTPROCESS PASS
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

static demo_postprocess::postprocess_pass_data CreatePostProcessPass()
{
	demo_postprocess::postprocess_pass_data Data = {};

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

	return Data;
}

static void DeletePostProcessPass(const demo_postprocess::postprocess_pass_data& Data)
{
    glDeleteProgram(Data.Program);
	glDeleteBuffers(1, &Data.VertexBuffer);
	glDeleteVertexArrays(1, &Data.VAO);
}

static void DrawPostProcessPass(const demo_postprocess::postprocess_pass_data& Data, const platform_io& IO, GLuint Texture)
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
demo_postprocess::demo_postprocess(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
    : DemoBase(GLCache, GLDebug)
{
    Framebuffer = CreateFramebuffer(IO.ScreenWidth, IO.ScreenHeight);
    PostProcessPassData = CreatePostProcessPass();
}

demo_postprocess::~demo_postprocess()
{
	DeletePostProcessPass(PostProcessPassData);
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
    const float AspectRatio = (float)IO.ScreenWidth / (float)IO.ScreenHeight;
    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);
    mat4 ProjectionTransform = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewTransform = CameraGetInverseMatrix(Camera);

	// Clear screen
	// Keep track previous framebuffer (should be 0)
	GLint PreviousFramebuffer;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &PreviousFramebuffer);

	// First pass, render geometry inside FBO
    {
        glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.FBO);

        glClearColor(0.0f, 0.0f, 0.0f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        DemoBase.Render(ProjectionTransform, ViewTransform, Mat4::Identity());
    }

	// Second pass render FBO.ColorTexture to screen with postprocess shader
    {
        glBindFramebuffer(GL_FRAMEBUFFER, PreviousFramebuffer);

        glDisable(GL_DEPTH_TEST);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        DrawPostProcessPass(PostProcessPassData, IO, Framebuffer.ColorTexture);
    }

    // Debug
    DemoBase.DisplayDebugUI();
    if (ImGui::TreeNodeEx("demo_postprocess", ImGuiTreeNodeFlags_Framed))
    {
        if (ImGui::TreeNodeEx("PostProcessPassData.Program"))
        {
            GLImGui::InspectProgram(PostProcessPassData.Program);
            ImGui::TreePop();
        }

        ImGui::Text("First pass image:");
        ImGui::Image((ImTextureID)(size_t)Framebuffer.ColorTexture, { IO.ScreenWidth / 3.f, IO.ScreenHeight / 3.f }, { 0.f, 1.f }, { 1.f, 0.f });

        PostProcessPassData.ColorTransform = DebugUI_GetColorTransformMatrix();
        ImGui::TreePop();
    }
}
