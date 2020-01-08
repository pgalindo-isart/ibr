
#include "maths.h"

#include "camera.h"

static const float CAM_MOUSE_SENSITIVITY_X = 0.0007f;
static const float CAM_MOUSE_SENSITIVITY_Y = 0.0007f;

camera CameraUpdateFPS(const camera& PreviousCamera, const camera_inputs& Inputs)
{
    camera Camera = PreviousCamera;

    float CameraSpeed = 4.f;
    if (Inputs.KeyInputsMask & CAM_MOVE_FAST)
        CameraSpeed *= 10.f;

    float FrameSpeed = (float)(CameraSpeed * Inputs.DeltaTime);

    float ForwardVelocity = 0.f;
    if (Inputs.KeyInputsMask & CAM_MOVE_BACKWARD)
        ForwardVelocity += -FrameSpeed;
    if (Inputs.KeyInputsMask & CAM_MOVE_FORWARD)
        ForwardVelocity += +FrameSpeed;
        
    float StrafeVelocity = 0.f;
    if (Inputs.KeyInputsMask & CAM_STRAFE_LEFT)
        StrafeVelocity += -FrameSpeed;
    if (Inputs.KeyInputsMask & CAM_STRAFE_RIGHT)
        StrafeVelocity += +FrameSpeed;

    Camera.Position.x += Math::Sin(Camera.Yaw) * ForwardVelocity;
    Camera.Position.z -= Math::Cos(Camera.Yaw) * ForwardVelocity;
    
    Camera.Position.x += Math::Cos(Camera.Yaw) * StrafeVelocity;
    Camera.Position.z += Math::Sin(Camera.Yaw) * StrafeVelocity;

    Camera.Yaw   += Inputs.MouseDX * CAM_MOUSE_SENSITIVITY_X;
    Camera.Pitch -= Inputs.MouseDY * CAM_MOUSE_SENSITIVITY_Y;

    Camera.Yaw = Math::TrueMod(Camera.Yaw, Math::TwoPi());
    Camera.Pitch = Math::Clamp(Camera.Pitch, -Math::HalfPi(), Math::HalfPi());

    return Camera;
}

camera CameraUpdateFreefly(const camera& PreviousCamera, const camera_inputs& Inputs)
{
    camera Camera = PreviousCamera;
    
    // Azimuth/inclination angles in radians
    float Azimuth     = Camera.Yaw;
    float Inclination = Camera.Pitch;

    // Spheric coordinates
    float CosAzimuth     = Math::Cos(Azimuth);
    float SinAzimuth     = Math::Sin(Azimuth);
    float CosInclination = Math::Cos(Inclination);
    float SinInclination = Math::Sin(Inclination);

    // Compute speed
    float Speed = 4.f;
    if (Inputs.KeyInputsMask & CAM_MOVE_FAST)
        Speed *= 10.f;
    float FrameSpeed = (float)(Speed * Inputs.DeltaTime);

    // Move forward/backward
    float ForwardVelocity = 0.f;
    if      (Inputs.KeyInputsMask & CAM_MOVE_FORWARD)  ForwardVelocity += -FrameSpeed;
    else if (Inputs.KeyInputsMask & CAM_MOVE_BACKWARD) ForwardVelocity += +FrameSpeed;

    // Strafe left/right
    float StrafeVelocity = 0.f;
    if      (Inputs.KeyInputsMask & CAM_STRAFE_LEFT)  StrafeVelocity += -FrameSpeed;
    else if (Inputs.KeyInputsMask & CAM_STRAFE_RIGHT) StrafeVelocity += +FrameSpeed;

    // Forward movement
    Camera.Position.z +=  CosAzimuth * CosInclination * ForwardVelocity;
    Camera.Position.x += -SinAzimuth * CosInclination * ForwardVelocity;
    Camera.Position.y +=              -SinInclination * ForwardVelocity;

    // Strafe movement
    Camera.Position.z += SinAzimuth * StrafeVelocity;
    Camera.Position.x += CosAzimuth * StrafeVelocity;

    // Up movement
    if (Inputs.KeyInputsMask & CAM_MOVE_UP)   Camera.Position.y += FrameSpeed;
    if (Inputs.KeyInputsMask & CAM_MOVE_DOWN) Camera.Position.y -= FrameSpeed;
    
    // Yaw
    Camera.Yaw   += Inputs.MouseDX * CAM_MOUSE_SENSITIVITY_X;
    Camera.Yaw    = Math::Mod((Camera.Yaw + Math::TwoPi()) + Math::Pi(), Math::TwoPi()) - Math::Pi(); // Loop around -180,180

    // Pitch
    Camera.Pitch -= Inputs.MouseDY * CAM_MOUSE_SENSITIVITY_Y;
    Camera.Pitch  = Math::Clamp(Camera.Pitch, -Math::HalfPi(), Math::HalfPi()); // Limit rotation to -90,90 range

    return Camera;
}

mat4 CameraGetInverseMatrix(const camera& Camera)
{
    // We know how to compute the inverse of the camera
    mat4 ViewTransform = Mat4::Identity();
    ViewTransform *= Mat4::RotateX(-Camera.Pitch);
    ViewTransform *= Mat4::RotateY(-Camera.Yaw);
    ViewTransform *= Mat4::Translate(-Camera.Position);
    return ViewTransform;
}