#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

// Demo of the simplest skybox
// 6 Faces + 6 Textures + Disabling depth write
class demo_pg_skybox : public demo
{
public:
    demo_pg_skybox();
    virtual ~demo_pg_skybox();
    virtual void Update(const platform_io& IO);

private:
    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;

    // Buffer storing all the vertices
    GLuint VertexBuffer = 0;

    // Mesh to display in the world (positions in the VertexBuffer)
    int MeshVertexStart = 0;
    int MeshVertexCount = 0;

    // Skybox (positions in the VertexBuffer)
    int SkyboxStart = 0;
    int SkyboxCount = 0;

    // Textures
    GLuint MeshTexture;
    GLuint SkyboxTextures[6];
};
