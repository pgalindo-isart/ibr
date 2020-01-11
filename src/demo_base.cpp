
#include <vector>

#include "imgui.h"
#include "opengl_helpers.h"
#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_base.h"

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
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

void main()
{
    oColor = texture(uColorTexture, vUV);
    
    // Compute phong light shading
    vec3 lightsColor = vec3(0.0, 0.0, 0.0);
	for (int i = 0; i < LIGHT_COUNT; ++i)
        lightsColor += light_shade(uLight[i], vViewPos, normalize(vViewNormal));
    
    // Apply light color
    oColor.rgb *= lightsColor;
})GLSL";

static bool EditLight(GL::light* Light)
{
    bool Result =
          ImGui::Checkbox("Enabled", (bool*)&Light->Enabled)
        + ImGui::ColorEdit3("Ambient", Light->Ambient.e)
        + ImGui::ColorEdit3("Diffuse", Light->Diffuse.e)
        + ImGui::ColorEdit3("Specular", Light->Specular.e)
        + ImGui::SliderFloat("Shininess", &Light->Shininess, 0.f, 1024.f)
        + ImGui::SliderFloat("Attenuation (constant)",  &Light->Attenuation.e[0], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (linear)",    &Light->Attenuation.e[1], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (quadratic)", &Light->Attenuation.e[2], 0.f, 10.f);
    return Result;
}

demo_base::demo_base()
{
    // Init camera pos
    this->Camera.Position = { 0.f, 0.f, 0.f };

    // Init lights
    {
        GL::light DefaultLight = // (Default light, standard values)
        {
            true, { -1.f, -1.f, -1.f }, // enabled (+padding)
            { 0.0f, 0.0f, 0.0f, 1.0f }, // position
            { 0.2f, 0.2f, 0.2f }, -1.f, // ambient (+padding)
            { 1.0f, 1.0f, 1.0f }, -1.f, // diffuse (+padding)
            { 1.0f, 1.0f, 1.0f },       // specular
            32.f,                       // shininess
            { 1.0f, 0.0f, 0.0f },       // attenuation(constant, linear, quadratic)
        };

        // Sun light
        this->Lights[0] = DefaultLight;
        LightsWorldPosition[0] = { 1.f, 3.f, 1.f, 0.f }; // Directional light
        this->Lights[0].Diffuse = Color::RGB(0x374D58);

        // Candles
        GL::light CandleLight = DefaultLight;
        CandleLight.Diffuse = Color::RGB(0xFFB400);
        CandleLight.Specular = CandleLight.Diffuse;
        CandleLight.Attenuation = { 0.f, 0.f, 2.0f };

        this->Lights[1] = this->Lights[2] = this->Lights[3] = this->Lights[4] = this->Lights[5] = CandleLight;
        // Candle positions (taken from mesh data)
        LightsWorldPosition[1] = { -3.21437f, -0.162299f, 5.54766f,  1.f }; // Candle 1
        LightsWorldPosition[2] = { -4.72162f, -0.162299f, 2.59089f,  1.f }; // Candle 2
        LightsWorldPosition[3] = { -2.66101f, -0.162299f, 0.235029f, 1.f }; // Candle 3
        LightsWorldPosition[4] = {  0.012123f, 0.352532f,-2.3027f,   1.f }; // Candle 4
        LightsWorldPosition[5] = {  3.03036f,  0.352532f,-1.64417f,  1.f }; // Candle 5
    }

    // Assemble fragment shader strings (defines + code)
    char FragmentShaderConfig[] = "#define LIGHT_COUNT %d  \n";
    snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), FragmentShaderConfig, LIGHT_COUNT);
    const char* FragmentShaderStrs[2] = {
        FragmentShaderConfig,
        gFragmentShaderStr,
    };

    // Create main shader
    this->Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
    
    // Create mesh
    {
        // Use vbo from GLCache
        MeshBuffer = GLCache::LoadObj("media/fantasy_game_inn.obj", 1.f, &this->VertexCount);
    }

    // Gen light uniform block
    {
        glGenBuffers(1, &LightsUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, LIGHT_COUNT * sizeof(GL::light), &Lights[0], GL_DYNAMIC_DRAW);
    }

    // Gen texture
    {
        Texture = GLCache::LoadTexture("media/fantasy_game_inn_diffuse.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    }
    
    // Create a vertex array and bind it with the vertex buffer
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, MeshBuffer);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)OFFSETOF(vertex_full, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)OFFSETOF(vertex_full, UV));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)OFFSETOF(vertex_full, Normal));
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
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        GL::light& Light = Lights[i];
        Light.Position = ViewTransform * LightsWorldPosition[i];
        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(GL::light) + OFFSETOF(GL::light, Position), sizeof(v4), &Light.Position);
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, glGetUniformBlockIndex(Program, "uLightBlock"), LightsUniformBuffer);
    
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

    if (ImGui::TreeNodeEx("Lights", ImGuiTreeNodeFlags_Framed))
    {
        for (int i = 0; i < LIGHT_COUNT; ++i)
        {
            if (ImGui::TreeNode(&Lights[i], "Light[%d]", i))
            {
                v4& LightWorldPosition = LightsWorldPosition[i];
                GL::light& Light = Lights[i];
                if (ImGui::SliderFloat4("Position", LightWorldPosition.e, -3.f, 3.f) + EditLight(&Light))
                {
                    glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
                    glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(GL::light), sizeof(GL::light), &Light);
                }

                // Calculate attenuation based on the light values
                if (ImGui::TreeNode("Attenuation calculator"))
                {
                    static float Dist = 5.f;
                    float Att = 1.f / (Light.Attenuation.e[0] + Light.Attenuation.e[1] * Dist + Light.Attenuation.e[2] * Light.Attenuation.e[2] * Dist);
                    ImGui::Text("att(d) = 1.0 / (c + ld + qdd)");
                    ImGui::SliderFloat("d", &Dist, 0.f, 20.f);
                    ImGui::Text("att(%.2f) = %.2f", Dist, Att);
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
}
