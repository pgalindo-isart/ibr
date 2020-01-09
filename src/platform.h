#pragma once

// Interface used to communicate with examples

#include "types.h"

#include "camera.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define OFFSETOF(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

struct platform_io
{
    int ScreenWidth;
    int ScreenHeight;
    
    double Time;
    double DeltaTime;

    bool MouseCaptured;
    float DeltaMouseX;
    float DeltaMouseY;
    float MouseX;
    float MouseY;

    camera_inputs CameraInputs;

    // F1 to F12 keys
    bool DebugKeysDown[12];
    bool DebugKeysPressed[12];
};
