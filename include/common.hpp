#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <string>

struct Vector2 {
    float x, y;
};

struct Context {
    const std::string& name;
    const std::string& basePath;
    SDL_Window* window;
    SDL_GPUDevice* GPUDevice;
    Vector2 windowSize;
    Vector2 mousPos;
    float deltaTime;
};

int GeneralInit(Context* context, SDL_WindowFlags windowFlags);
void GeneralQuit(Context* context);

void InitAssetLoader();
SDL_Surface* LoadImage(const std::string& imageFileName, int channels);

SDL_GPUShader* LoadShader(
    SDL_GPUDevice* GPUDevice,
    const std::string& fileName,
    uint32_t samplerCount,
    uint32_t uniformBufferCount,
    uint32_t storageBufferCount,
    uint32_t storageTextureCount
);

SDL_GPUComputePipeline* CreateComputePipelineFromShader(
    SDL_GPUDevice* GPUDevice,
    const std::string& shaderFileName,
    SDL_GPUComputePipelineCreateInfo* createInfo
);

struct PositionVertex {
    float x, y, z;
};

struct PositionColorVertex {
    float x, y, z;
    float r, g, b, a;
};

struct PositionTextureVertex {
    float x, y, z;
    float u, v;
};

struct Matrix4x4 {
    float m11, m12, m13, m14;
    float m21, m22, m23, m24;
    float m31, m32, m33, m34;
    float m41, m42, m43, m44;
};

struct Vector3 {
    float x, y, z;
};


Matrix4x4 Matrix4x4_Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2);
Matrix4x4 Matrix4x4_CreateRotationZ(float radians);
Matrix4x4 Matrix4x4_CreateTranslation(float x, float y, float z);
Matrix4x4 Matrix4x4_CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);
Matrix4x4 Matrix4x4_CreatePerspectiveFieldOfView(float fieldOfView, float aspectRatio, float nearPlaneDistance, float farPlaneDistance);
Matrix4x4 Matrix4x4_CreateLookAt(Vector3 cameraPosition, Vector3 cameraTarget, Vector3 cameraUpVector);
Vector3 Vector3_Normalize(Vector3 vec);
float Vector3_Dot(Vector3 vecA, Vector3 vecB);
Vector3 Vector3_Cross(Vector3 vecA, Vector3 vecB);

