#pragma once

#include "types.h"

// Camera data (for fps or freefly type)
struct camera
{
    v3 Position;
    float Yaw;
    float Pitch;
};

enum camera_key_inputs_mask
{
    CAM_NO_MOVE       = 0,
    CAM_MOVE_FORWARD  = 1 << 0,
    CAM_MOVE_BACKWARD = 1 << 1,
    CAM_STRAFE_LEFT   = 1 << 2,
    CAM_STRAFE_RIGHT  = 1 << 3,
    CAM_MOVE_FAST     = 1 << 4,
    CAM_MOVE_UP       = 1 << 5,
    CAM_MOVE_DOWN     = 1 << 6,
};

// Camera input data (needed to update movement)
struct camera_inputs
{
    double DeltaTime;
    int KeyInputsMask; // Using values of camera_key_inputs_mask
    float MouseDX;
    float MouseDY;
};

camera CameraUpdateFPS(const camera& PreviousCamera, const camera_inputs& Inputs);
camera CameraUpdateFreefly(const camera& PreviousCamera, const camera_inputs& Inputs);
mat4 CameraGetInverseMatrix(const camera& Camera);