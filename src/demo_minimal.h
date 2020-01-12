#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

class demo_minimal : public demo
{
public:
    demo_minimal();
    virtual ~demo_minimal();
    virtual void Update(const platform_io& IO);

private:
    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint Texture = 0;

    GLuint VAO = 0;
    GLuint VertexBuffer = 0;
    int VertexCount = 0;

};
