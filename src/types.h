#pragma once

#include <cstdint>

struct v2
{
    union
    {
        struct { float x, y; };
        float e[2];
    };
};

struct v3
{
    union
    {
        struct { float x, y, z; };
        float e[3];
    };
};

struct v4
{
    union
    {
        struct { float x, y, z, w; };
        v3 xyz;
        float e[4];
    };
};

struct mat4
{
    union
    {
        float e[16];
        v4 c[4];
    };
};