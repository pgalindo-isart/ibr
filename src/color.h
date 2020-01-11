#pragma once

#include "types.h"

namespace Color
{
    static v3 RGB(uint32_t Color)
    {
        v3 Res = {};
        Res.r = ((Color & 0x00ff0000) >> 16) / 255.f;
        Res.g = ((Color & 0x0000ff00) >> 8) / 255.f;
        Res.b = ((Color & 0x000000ff) >> 0) / 255.f;
        return Res;
    }

    static v4 RGBA(uint32_t Color)
    {
        v4 Res = {};
        Res.a = ((Color & 0xff000000) >> 24) / 255.f;
        Res.r = ((Color & 0x00ff0000) >> 16) / 255.f;
        Res.g = ((Color & 0x0000ff00) >> 8) / 255.f;
        Res.b = ((Color & 0x000000ff) >> 0) / 255.f;
        return Res;
    }
}
