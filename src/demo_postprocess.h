#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

class demo_postprocess : public demo
{
public:
    // New framebuffer to render offscreen
    struct framebuffer
    {
        GLuint FBO;
        GLuint ColorTexture;
        GLuint DepthStencilRenderbuffer;
    };

    // Render some geometry
    struct first_pass_data
    {
        GLuint Program = 0;
        GLuint VAO = 0;
        GLuint VertexBuffer = 0;
        GLuint MeshStart = 0;
        GLuint MeshEnd = 0;
        GLuint Texture = 0;
    };

    // Used to render a quad
    struct second_pass_data
    {
        GLuint Program = 0;
        GLuint VAO = 0;
        GLuint VertexBuffer = 0; // We will store a quad (6 vertices)
        
        mat4 ColorTransform = Mat4::Identity();
    };

    demo_postprocess(const platform_io& IO);
    virtual ~demo_postprocess();
    virtual void Update(const platform_io& IO);

private:
    // 3d camera
    camera Camera = {};

    first_pass_data FirstPassData = {};
    second_pass_data SecondPassData = {};
    framebuffer Framebuffer = {};
};
