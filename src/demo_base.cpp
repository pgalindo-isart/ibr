
#include <vector>

#include "imgui.h"
#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"

#include "demo_base.h"

struct vertex
{
    v3 Position;
    v2 UV;
    v3 Normal;
};

static const char* gVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Uniforms
uniform float uTime;
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

// Varyings
out vec2 vUV;
out vec3 vViewPos;
out vec3 vViewNormal;

void main()
{
    vUV = aUV;
    mat4 modelView = uView * uModel;
    vec4 viewPos4 = (modelView * vec4(aPosition, 1.0));
    vViewPos = viewPos4.xyz / viewPos4.w;
    vViewNormal = (modelView * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * viewPos4;
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vViewPos;
in vec3 vViewNormal;

// Uniforms
uniform sampler2D uColorTexture;
uniform float uTime;
uniform uLightBlock
{
	light uLight;
};

// Shader outputs
out vec4 oColor;

void main()
{
    oColor = texture(uColorTexture, vUV);
    
    // Apply a phong light shading
    oColor.rgb *= light_shade(uLight, vViewPos, normalize(vViewNormal));
})GLSL";

static bool EditLight(GL::light* Light)
{
    bool Result = 
          ImGui::ColorEdit3("Ambient", Light->Ambient.e)
        + ImGui::ColorEdit3("Diffuse", Light->Diffuse.e)
        + ImGui::ColorEdit3("Specular", Light->Specular.e)
        + ImGui::SliderFloat("Shininess", &Light->Shininess, 0.f, 1024.f)
        + ImGui::SliderFloat("Attenuation (constant)",  &Light->Attenuation[0], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (linear)",    &Light->Attenuation[1], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (quadratic)", &Light->Attenuation[2], 0.f, 10.f);
    return Result;
}

demo_base::demo_base()
{
    // Init camera pos
    this->Camera.Position.z = 2.f;

    // Init light
    this->Light =
    {
        { 0.0f, 0.0f, 0.0f, 1.0f }, // position
        { 0.2f, 0.2f, 0.2f }, -1.f, // ambient
        { 1.0f, 1.0f, 1.0f }, -1.f, // diffuse
        { 1.0f, 1.0f, 1.0f },       // specular
        32.f,                       // shininess
        { 1.0f, 0.0f, 0.0f },       // attenuation(constant, linear, quadratic)
    };

    // Create render pipeline
    this->Program = GL::CreateProgram(gVertexShaderStr, gFragmentShaderStr, true);
    
    // Gen mesh
    {
        std::vector<vertex> VerticeBuffer(20480);

        vertex* VerticesStart = &VerticeBuffer[0];
        vertex* VerticesEnd = VerticesStart + VerticeBuffer.size();

        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex);
        Descriptor.HasUV = true;
        Descriptor.HasNormal = true;
        Descriptor.PositionOffset = OFFSETOF(vertex, Position);
        Descriptor.UVOffset = OFFSETOF(vertex, UV);
        Descriptor.NormalOffset = OFFSETOF(vertex, Normal);

        vertex* Cur = VerticesStart;

        // Add a scaled down cube
        Cur = (vertex*)Mesh::Transform(
            Cur, Mesh::BuildCube(Cur, VerticesEnd, Descriptor),
            Descriptor, Mat4::Scale({0.5f, 0.5f, 0.5f}));

        // Add a obj
        Cur = (vertex*)Mesh::Transform(
            Cur, Mesh::LoadObj(Cur, VerticesEnd, Descriptor, "media/teapot.obj", 0.2f),
            Descriptor, Mat4::Translate({ 0.f, 0.25f, 0.f }));

        // Add a sphere
        Cur = (vertex*)Mesh::Transform(
            Cur, Mesh::BuildSphere(Cur, VerticesEnd, Descriptor, 8, 8),
            Descriptor, Mat4::Translate({ 1.f, 0.f, 0.f}) * Mat4::Scale({ 0.5f, 0.5f, 0.5f }));

        // Calculate vertex count (we need it to call glDrawArrays)
        this->VertexCount = (int)(Cur - VerticesStart);

        // Upload mesh to gpu
        glGenBuffers(1, &MeshBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, MeshBuffer);
        glBufferData(GL_ARRAY_BUFFER, this->VertexCount * sizeof(vertex), VerticesStart, GL_STATIC_DRAW);
    }

    // Gen light uniform block
    {
        glGenBuffers(1, &LightUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, LightUniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(GL::light), &Light, GL_DYNAMIC_DRAW);
    }

    // Gen texture
    {
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        GL::UploadCheckerboardTexture(64, 64, 8);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
    // Create a vertex array and bind it with the vertex buffer
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, MeshBuffer);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, UV));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, Normal));
        glBindVertexArray(0);
    }
}

demo_base::~demo_base()
{
    // Cleanup GL
    glDeleteTextures(1, &Texture);
    glDeleteBuffers(1, &MeshBuffer);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

void demo_base::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.ScreenWidth / (float)IO.ScreenHeight;
    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    glEnable(GL_DEPTH_TEST);

    // Clear screen
    glClearColor(0.3f, 0.3f, 0.3f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader and configure its uniforms
    glUseProgram(Program);
    glUniform1f(glGetUniformLocation(Program, "uTime"), (float)IO.Time);

    // Matrices
    mat4 ProjectionTransform = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewTransform = CameraGetInverseMatrix(Camera);
    mat4 ModelTransform = Mat4::Translate({ 0.f, 0.f, 0.f });
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionTransform.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewTransform.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelTransform.e);

    // Update light position inside the uniform buffer
    if (LightAutoMove)
    {
        LightWorldPosition.x = Math::Sin((float)(IO.Time * 0.5)) * 3.f;
        LightWorldPosition.y = 2.f;
        LightWorldPosition.z = Math::Cos((float)(IO.Time * 0.75)) * 3.f;
    }
    Light.Position = ViewTransform * LightWorldPosition;
    glBindBuffer(GL_UNIFORM_BUFFER, LightUniformBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, OFFSETOF(GL::light, Position), sizeof(v4), &Light.Position);
    glBindBufferBase(GL_UNIFORM_BUFFER, glGetUniformBlockIndex(Program, "uLightBlock"), LightUniformBuffer);
    
    // Draw mesh
    glBindTexture(GL_TEXTURE_2D, Texture);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, VertexCount);
    glBindVertexArray(0);

    // Debug display
    if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
        ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
        ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Shader", ImGuiTreeNodeFlags_Framed))
    {
        GLImGui::InspectProgram(Program);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Light", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Checkbox("Anim", &LightAutoMove);
        if (ImGui::SliderFloat4("Position", LightWorldPosition.e, -3.f, 3.f) + EditLight(&Light))
        {
            glBindBuffer(GL_UNIFORM_BUFFER, LightUniformBuffer);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GL::light), &Light);
        }

        // Calculate attenuation based on the light values
        {
            ImGui::Text("Attenuation calculator:");
            static float Dist = 5.f;
            float Att = 1.f / (Light.Attenuation[0] + Light.Attenuation[1] * Dist + Light.Attenuation[2] * Light.Attenuation[2] * Dist);
            ImGui::Text("att(d) = 1.0 / (c + ld + qdd)");
            ImGui::SliderFloat("d", &Dist, 0.f, 20.f);
            ImGui::Text("att(%.2f) = %.2f", Dist, Att);
            ImGui::TreePop();
        }
    }
}
