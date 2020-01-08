#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

class demo_base : public demo
{
public:
    demo_base();
    virtual ~demo_base();
    virtual void Update(const platform_io& IO);

private:
    // 3d camera
    camera Camera = {};
    int VertexCount = 0;

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint MeshBuffer = 0;
    GLuint VAO = 0;

    GLuint Texture = 0;
};
