#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"
#include "demo_base.h"

// Demo of the simplest skybox
// 6 Faces + 6 Textures + Disabling depth write
class demo_pg_skybox : public demo
{
public:
    demo_pg_skybox(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_pg_skybox();
    virtual void Update(const platform_io& IO);

private:
    demo_base DemoBase;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;

    // Buffer storing all the vertices
    GLuint VertexBuffer = 0;

    // Skybox (positions in the VertexBuffer)
    int SkyboxStart = 0;
    int SkyboxCount = 0;

    // Textures
    GLuint SkyboxTextures[6] = {};
};
