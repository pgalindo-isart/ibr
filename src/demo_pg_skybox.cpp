
#include <vector>

#include "imgui.h"
#include "stb_image.h"
#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"

#include "demo_pg_skybox.h"

struct vertex
{
    v3 Position;
    v2 UV;
};

static const char* gVertexShaderStr = R"GLSL(
#line 20
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

out vec2 aUV;

void main()
{
    aUV = uv;
    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
#line 36
uniform sampler2D colorTexture;
in vec2 aUV;
out vec4 color;
void main()
{
    color = texture(colorTexture, aUV);
})GLSL";

demo_pg_skybox::demo_pg_skybox()
{
    this->Camera.Position.z = 2.f;

    // Create render pipeline
    this->Program = GL::CreateProgram(gVertexShaderStr, gFragmentShaderStr);
    
    // Gen meshes and skybox faces
    {
        std::vector<vertex> VerticeBuffer(2048);

        vertex* VerticesStart = &VerticeBuffer[0];
        vertex* VerticesEnd = VerticesStart + VerticeBuffer.size();

        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex, Position);
        Descriptor.UVOffset = OFFSETOF(vertex, UV);

        vertex* Cur = VerticesStart;
        
        // Construct the mesh that will be displayed inside the world
        {
            this->MeshVertexStart = (int)(Cur - VerticesStart);

            // Add a scaled down cube
            Cur = (vertex*)Mesh::Transform(
                Cur, Mesh::BuildCube(Cur, VerticesEnd, Descriptor),
                Descriptor, Mat4::Scale({0.5f, 0.5f, 0.5f}));
            // Add a sphere on top of everything
            Cur = (vertex*)Mesh::Transform(
                Cur, Mesh::BuildSphere(Cur, VerticesEnd, Descriptor, 8, 8),
                Descriptor, Mat4::Translate({ 0.f, 1.f, 0.f}) * Mat4::Scale({ 0.5f, 0.5f, 0.5f }));

            this->MeshVertexCount = (int)(Cur - VerticesStart) - this->MeshVertexStart;
        }

        // Create the skybox mesh
        {
            this->SkyboxStart = (int)(Cur - VerticesStart);

            Cur = (vertex*)Mesh::BuildInvertedCube(Cur, VerticesEnd, Descriptor);

            this->SkyboxCount = (int)(Cur - VerticesStart) - this->SkyboxStart;
        }

        // Upload mesh to gpu
        glGenBuffers(1, &VertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, (int)(Cur - VerticesStart) * sizeof(vertex), VerticesStart, GL_STATIC_DRAW);
    }

    // Gen mesh texture
    {
        glGenTextures(1, &MeshTexture);
        glBindTexture(GL_TEXTURE_2D, MeshTexture);
        GL::UploadCheckerboardTexture(64, 64, 8);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Load skybox textures
    {
        glGenTextures(6, SkyboxTextures);

        const char* SkyboxFiles[] = {
            "media/ss_ft.tga",
            "media/ss_bk.tga",
            "media/ss_lf.tga",
            "media/ss_rt.tga",
            "media/ss_up.tga",
            "media/ss_dn.tga",
        };

        // Load 6 textures
        for (int i = 0; i < 6; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, SkyboxTextures[i]);
            GL::UploadTexture(SkyboxFiles[i], IMG_FLIP | IMG_FORCE_RGB);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
    
    // Create a vertex array and bind it
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, Position));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, UV));
    glBindVertexArray(0);
}

demo_pg_skybox::~demo_pg_skybox()
{
    // Cleanup GL
    glDeleteTextures(1, &MeshTexture);
    glDeleteBuffers(1, &VertexBuffer);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

void demo_pg_skybox::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.ScreenWidth / (float)IO.ScreenHeight;
    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE); // Enabling face culling

    // Clear screen
    glClearColor(0.3f, 0.3f, 0.3f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader and configure its uniforms
    glUseProgram(Program);

    mat4 ProjectionTransform = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionTransform.e);

    mat4 ViewTransform = CameraGetInverseMatrix(Camera);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewTransform.e);
    
    glBindVertexArray(VAO);

    // Draw skybox
    {
        // Disable depth 
        glDepthMask(GL_FALSE);

        // Place skybox at the camera position (simulating a fixed view)
        mat4 ModelTransform = Mat4::Translate(Camera.Position);
        glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelTransform.e);

        for (int i = 0; i < 6; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, SkyboxTextures[i]);
            glDrawArrays(GL_TRIANGLES, SkyboxStart + i * 6, 6);
        }
        glDepthMask(GL_TRUE);
    }

    // Draw meshes
    {
        mat4 ModelTransform = Mat4::Translate({ 0.f, 0.f, -2.f });
        glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelTransform.e);

        glBindTexture(GL_TEXTURE_2D, MeshTexture);
        glDrawArrays(GL_TRIANGLES, MeshVertexStart, MeshVertexCount);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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
