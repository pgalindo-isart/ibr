
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
#line 20
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

static const char* gFragmentShaderStr = R"GLSL(
#line 45
uniform sampler2D colorTexture;
in vec2 aUV;
in vec3 aViewPos;
in vec3 aViewNormal;
in mat4 aViewMatrix;
out vec4 color;
uniform float uTime;

void main()
{
    color = texture(colorTexture, aUV);
    
    // Apply a phong light shading (converting its position to modelView)
    // In real case, we do not use gDefaultLight and precompute the light ViewPosition
    light light = gDefaultLight;
    light.position = aViewMatrix * gDefaultLight.position;
    color.rgb *= light_shade(light, aViewPos, normalize(aViewNormal));
})GLSL";

demo_base::demo_base()
{
    this->Camera.Position.z = 2.f;

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
#if 0
        // This...
        Cur = (vertex*)Mesh::Transform(Cur, Mesh::BuildCube(Cur, VerticesEnd, Descriptor), Descriptor, Mat4::Scale({0.5f, 0.5f, 0.5f}));
#else
        // is the same than this:
        vertex* QuadBegin = Cur;
        vertex* QuadEnd = (vertex*)Mesh::BuildCube(Cur, VerticesEnd, Descriptor);
        Mesh::Transform(QuadBegin, QuadEnd, Descriptor, Mat4::Scale({0.5f, 0.5f, 0.5f}));
        Cur = QuadEnd;
#endif

        // Add a obj
        Cur = (vertex*)Mesh::Transform(
            Cur, Mesh::LoadObj(Cur, VerticesEnd, Descriptor, "media/teapot.obj", 0.1f),
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

    // Gen texture
    {
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        GL::UploadCheckerboardTexture(64, 64, 8);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
    // Create a vertex array and bind it
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

    mat4 ProjectionTransform = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionTransform.e);

    mat4 ViewTransform = CameraGetInverseMatrix(Camera);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewTransform.e);

    mat4 MeshTransform = Mat4::Translate({ 0.f, 0.f, -2.f });
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, MeshTransform.e);
    
    // Display mesh
    glBindTexture(GL_TEXTURE_2D, Texture);

    glBindVertexArray(VAO);

    // Draw call
    glDrawArrays(GL_TRIANGLES, 0, VertexCount);

    glBindVertexArray(0);

    // Debug display
    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
        ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
        ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
    }

    if (ImGui::CollapsingHeader("Shader"))
    {
        GLImGui::InspectProgram(Program);
    }
}
