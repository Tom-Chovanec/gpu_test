#include "../include/common.hpp"
#include <SDL3/SDL_filesystem.h>
#include <cstdint>

int GeneralInit(Context *context, SDL_WindowFlags windowFlags) {
    context->GPUDevice = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV,
        false,
        NULL 
    );
    if (context->GPUDevice == NULL) {
        SDL_LogError(1, "CreateGPUDevice failed with error: %s", SDL_GetError());
        return -1;
    }

    context->window = SDL_CreateWindow(context->name.c_str(), context->windowSize.x, context->windowSize.y, windowFlags);
    if (context->window == NULL) {
        SDL_LogError(1, "CreaWindow failed with error: %s", SDL_GetError());
        return -1;
    }

    if (!SDL_ClaimWindowForGPUDevice(context->GPUDevice, context->window)) {
        SDL_LogError(1, "ClaimWindowForGPUDevice failed with error: %s", SDL_GetError());
        return -1;
    }

    return 0;
}

void GeneralQuit(Context* context) {
    SDL_ReleaseWindowFromGPUDevice(context->GPUDevice, context->window);
    SDL_DestroyWindow(context->window);
    SDL_DestroyGPUDevice(context->GPUDevice);
}

// you win this time
static const char* basePath = NULL;
void InitAssetLoader() {
    basePath = SDL_GetBasePath();
}

SDL_GPUShader* LoadShader(
    SDL_GPUDevice *GPUDevice,
    const std::string &fileName,
    uint32_t samplerCount,
    uint32_t uniformBufferCount,
    uint32_t storageBufferCount,
    uint32_t storageTextureCount
) {
    SDL_GPUShaderStage stage;
    if (SDL_strstr(fileName.c_str(), ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (SDL_strstr(fileName.c_str(), ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else {
        SDL_LogWarn(1, "Invalid shader stage!");
        return NULL;
    }

    char fullPath[256];
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(GPUDevice);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char *entrypoint;

    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/%s.spv", basePath, fileName.c_str());
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else {
        SDL_LogError(1, "Unrecognized backend shader format!");
        return NULL;
    }

    size_t codeSize;
    void* code = SDL_LoadFile(fullPath, &codeSize);
    if (code == NULL) {
        SDL_LogError(1, "Failed to load shader from disk! path: %s    error: %s", fullPath, SDL_GetError());
        return NULL;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code_size = codeSize,
        .code = static_cast<const uint8_t*>(code),
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = samplerCount,
        .num_storage_textures = storageTextureCount,
        .num_storage_buffers = storageBufferCount,
        .num_uniform_buffers = uniformBufferCount,
    };
    SDL_GPUShader* shader = SDL_CreateGPUShader(GPUDevice, &shaderInfo);
    if (shader == NULL) {
        SDL_Log("Failed to create shader! error: %s", SDL_GetError());
        SDL_free(code);
        return NULL;
    }

    SDL_free(code);
    return shader;
}

SDL_GPUComputePipeline* CreateComputePipelineFromShader(
    SDL_GPUDevice* GPUDevice,
    const std::string& fileName,
    SDL_GPUComputePipelineCreateInfo *createInfo
) {
    char fullPath[256];
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(GPUDevice);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char *entrypoint;

    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/%s.spv", basePath, fileName.c_str());
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else {
        SDL_LogError(1, "Unrecognized backend shader format!");
        return NULL;
    }

    size_t codeSize;
    void* code = SDL_LoadFile(fullPath, &codeSize);
    if (code == NULL) {
        SDL_Log("Failed to load compute shader from disk! file: %s    error: %s", fullPath, SDL_GetError());
        return NULL;
    }

    // Make a copy of the create data, then overwrite the parts we need
    SDL_GPUComputePipelineCreateInfo newCreateInfo = *createInfo;
    newCreateInfo.code = static_cast<const uint8_t*>(code);
    newCreateInfo.code_size = codeSize;
    newCreateInfo.entrypoint = entrypoint;
    newCreateInfo.format = format;

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(GPUDevice, &newCreateInfo);
    if (pipeline == NULL) {
        SDL_LogError(1, "Failed to create compute pipeline! error: %s", SDL_GetError());
        SDL_free(code);
        return NULL;
    }

    SDL_free(code);
    return pipeline;
}

SDL_Surface* LoadImage(const std::string& imageFileName, int desiredChannels) {
    char fullPath[256];
    SDL_Surface *result;
    SDL_PixelFormat format;

    SDL_snprintf(fullPath, sizeof(fullPath), "%sassets/%s", basePath, imageFileName.c_str());

    result = SDL_LoadBMP(fullPath);
    if (result == NULL) {
        SDL_LogError(1, "Failed to load BMP error: %s", SDL_GetError());
        return NULL;
    }

    if (desiredChannels == 4) {
        format = SDL_PIXELFORMAT_ABGR8888;
    } else {
        SDL_assert(!"Unexpected desiredChannels");
        SDL_DestroySurface(result);
        return NULL;
    } if (result->format != format) {
        SDL_Surface *next = SDL_ConvertSurface(result, format);
        SDL_DestroySurface(result);
        result = next;
    }

    return result;
}

// Matrix Math
Matrix4x4 Matrix4x4_Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2)
{
	Matrix4x4 result;

	result.m11 = (
		(matrix1.m11 * matrix2.m11) +
		(matrix1.m12 * matrix2.m21) +
		(matrix1.m13 * matrix2.m31) +
		(matrix1.m14 * matrix2.m41)
	);
	result.m12 = (
		(matrix1.m11 * matrix2.m12) +
		(matrix1.m12 * matrix2.m22) +
		(matrix1.m13 * matrix2.m32) +
		(matrix1.m14 * matrix2.m42)
	);
	result.m13 = (
		(matrix1.m11 * matrix2.m13) +
		(matrix1.m12 * matrix2.m23) +
		(matrix1.m13 * matrix2.m33) +
		(matrix1.m14 * matrix2.m43)
	);
	result.m14 = (
		(matrix1.m11 * matrix2.m14) +
		(matrix1.m12 * matrix2.m24) +
		(matrix1.m13 * matrix2.m34) +
		(matrix1.m14 * matrix2.m44)
	);
	result.m21 = (
		(matrix1.m21 * matrix2.m11) +
		(matrix1.m22 * matrix2.m21) +
		(matrix1.m23 * matrix2.m31) +
		(matrix1.m24 * matrix2.m41)
	);
	result.m22 = (
		(matrix1.m21 * matrix2.m12) +
		(matrix1.m22 * matrix2.m22) +
		(matrix1.m23 * matrix2.m32) +
		(matrix1.m24 * matrix2.m42)
	);
	result.m23 = (
		(matrix1.m21 * matrix2.m13) +
		(matrix1.m22 * matrix2.m23) +
		(matrix1.m23 * matrix2.m33) +
		(matrix1.m24 * matrix2.m43)
	);
	result.m24 = (
		(matrix1.m21 * matrix2.m14) +
		(matrix1.m22 * matrix2.m24) +
		(matrix1.m23 * matrix2.m34) +
		(matrix1.m24 * matrix2.m44)
	);
	result.m31 = (
		(matrix1.m31 * matrix2.m11) +
		(matrix1.m32 * matrix2.m21) +
		(matrix1.m33 * matrix2.m31) +
		(matrix1.m34 * matrix2.m41)
	);
	result.m32 = (
		(matrix1.m31 * matrix2.m12) +
		(matrix1.m32 * matrix2.m22) +
		(matrix1.m33 * matrix2.m32) +
		(matrix1.m34 * matrix2.m42)
	);
	result.m33 = (
		(matrix1.m31 * matrix2.m13) +
		(matrix1.m32 * matrix2.m23) +
		(matrix1.m33 * matrix2.m33) +
		(matrix1.m34 * matrix2.m43)
	);
	result.m34 = (
		(matrix1.m31 * matrix2.m14) +
		(matrix1.m32 * matrix2.m24) +
		(matrix1.m33 * matrix2.m34) +
		(matrix1.m34 * matrix2.m44)
	);
	result.m41 = (
		(matrix1.m41 * matrix2.m11) +
		(matrix1.m42 * matrix2.m21) +
		(matrix1.m43 * matrix2.m31) +
		(matrix1.m44 * matrix2.m41)
	);
	result.m42 = (
		(matrix1.m41 * matrix2.m12) +
		(matrix1.m42 * matrix2.m22) +
		(matrix1.m43 * matrix2.m32) +
		(matrix1.m44 * matrix2.m42)
	);
	result.m43 = (
		(matrix1.m41 * matrix2.m13) +
		(matrix1.m42 * matrix2.m23) +
		(matrix1.m43 * matrix2.m33) +
		(matrix1.m44 * matrix2.m43)
	);
	result.m44 = (
		(matrix1.m41 * matrix2.m14) +
		(matrix1.m42 * matrix2.m24) +
		(matrix1.m43 * matrix2.m34) +
		(matrix1.m44 * matrix2.m44)
	);

	return result;
}

Matrix4x4 Matrix4x4_CreateRotationZ(float radians)
{
	return (Matrix4x4) {
		 SDL_cosf(radians), SDL_sinf(radians), 0, 0,
		-SDL_sinf(radians), SDL_cosf(radians), 0, 0,
						 0, 				0, 1, 0,
						 0,					0, 0, 1
	};
}

Matrix4x4 Matrix4x4_CreateTranslation(float x, float y, float z)
{
	return (Matrix4x4) {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		x, y, z, 1
	};
}

Matrix4x4 Matrix4x4_CreateOrthographicOffCenter(
	float left,
	float right,
	float bottom,
	float top,
	float zNearPlane,
	float zFarPlane
) {
	return (Matrix4x4) {
		2.0f / (right - left), 0, 0, 0,
		0, 2.0f / (top - bottom), 0, 0,
		0, 0, 1.0f / (zNearPlane - zFarPlane), 0,
		(left + right) / (left - right), (top + bottom) / (bottom - top), zNearPlane / (zNearPlane - zFarPlane), 1
	};
}

Matrix4x4 Matrix4x4_CreatePerspectiveFieldOfView(
	float fieldOfView,
	float aspectRatio,
	float nearPlaneDistance,
	float farPlaneDistance
) {
	float num = 1.0f / ((float) SDL_tanf(fieldOfView * 0.5f));
	return (Matrix4x4) {
		num / aspectRatio, 0, 0, 0,
		0, num, 0, 0,
		0, 0, farPlaneDistance / (nearPlaneDistance - farPlaneDistance), -1,
		0, 0, (nearPlaneDistance * farPlaneDistance) / (nearPlaneDistance - farPlaneDistance), 0
	};
}

Matrix4x4 Matrix4x4_CreateLookAt(
	Vector3 cameraPosition,
	Vector3 cameraTarget,
	Vector3 cameraUpVector
) {
	Vector3 targetToPosition = {
		cameraPosition.x - cameraTarget.x,
		cameraPosition.y - cameraTarget.y,
		cameraPosition.z - cameraTarget.z
	};
	Vector3 vectorA = Vector3_Normalize(targetToPosition);
	Vector3 vectorB = Vector3_Normalize(Vector3_Cross(cameraUpVector, vectorA));
	Vector3 vectorC = Vector3_Cross(vectorA, vectorB);

	return (Matrix4x4) {
		vectorB.x, vectorC.x, vectorA.x, 0,
		vectorB.y, vectorC.y, vectorA.y, 0,
		vectorB.z, vectorC.z, vectorA.z, 0,
		-Vector3_Dot(vectorB, cameraPosition), -Vector3_Dot(vectorC, cameraPosition), -Vector3_Dot(vectorA, cameraPosition), 1
	};
}

Vector3 Vector3_Normalize(Vector3 vec)
{
	float magnitude = SDL_sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
	return (Vector3) {
		vec.x / magnitude,
		vec.y / magnitude,
		vec.z / magnitude
	};
}

float Vector3_Dot(Vector3 vecA, Vector3 vecB)
{
	return (vecA.x * vecB.x) + (vecA.y * vecB.y) + (vecA.z * vecB.z);
}

Vector3 Vector3_Cross(Vector3 vecA, Vector3 vecB)
{
	return (Vector3) {
		vecA.y * vecB.z - vecB.y * vecA.z,
		-(vecA.x * vecB.z - vecB.x * vecA.z),
		vecA.x * vecB.y - vecB.x * vecA.y
	};
}