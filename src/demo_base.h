#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"
#include "opengl_helpers.h"

#include "camera.h"

class demo_base : public demo
{
public:
    demo_base(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_base();
    virtual void Update(const platform_io& IO);

    void Render(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint MeshBuffer = 0;
    GLuint VAO = 0;
    GLuint LightsUniformBuffer = 0;
    GLuint Texture = 0;
    int MeshVertexCount = 0; // Mesh Size

    bool Wireframe = false;

    static const int LIGHT_COUNT = 8;
    GL::light Lights[LIGHT_COUNT] = {};
    // Positions relative to the mesh
    v4 LightsPosition[LIGHT_COUNT] = {};
};
