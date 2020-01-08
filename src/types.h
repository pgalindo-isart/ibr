#pragma once

struct v2
{
    float x, y;
};

struct v3
{
    float x, y, z;
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