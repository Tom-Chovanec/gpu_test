#include "../include/common.hpp"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>

static SDL_GPUGraphicsPipeline* pipeline;
static SDL_GPUBuffer* vertexBuffer;
static SDL_GPUBuffer* indexBuffer;

static void Quit(Context* context);

static Context context = {
    .name = "SDL_GPU",
    .basePath = SDL_GetBasePath(),
    .windowSize = {
        .x = 800,
        .y = 800,
    }
};


typedef struct GradientUniforms
{
    float time;
} GradientUniforms;

static GradientUniforms GradientUniformValues;

int Init(Context* context) {
    int result = GeneralInit(context, 0);
    if (result < 0) return result;

    SDL_GPUShader* vertexShader = LoadShader( context->GPUDevice, "position.vert", 0, 0, 0, 0);
    if (vertexShader == NULL) {

        SDL_Log("Failed to create vertex shader");
        return -1;
    }

    SDL_GPUShader* fragmentShader = LoadShader( context->GPUDevice, "solidColor.frag", 0, 1, 0, 0);
    if (fragmentShader == NULL) {
        SDL_Log("Failed to create fragment shader");
        return -1;
    }

    SDL_GPUVertexBufferDescription vertexBufferDescription[] = {{
        .slot = 0,
        .pitch = sizeof(PositionVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    }};

    SDL_GPUVertexAttribute vertexAttributes[] = {{
        .location = 0,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = 0
    }};

    SDL_GPUVertexInputState vertexInputState = {
        .vertex_buffer_descriptions = vertexBufferDescription,
        .num_vertex_buffers = 1,
        .vertex_attributes = vertexAttributes,
        .num_vertex_attributes = 1,
    };


    SDL_GPUColorTargetDescription colorTargetDescription[] = {{
        .format = SDL_GetGPUSwapchainTextureFormat(context->GPUDevice, context->window),
        .blend_state = {
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
            .enable_blend = true,
        },
    }};

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
        .vertex_input_state = vertexInputState,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info = {
            .color_target_descriptions = colorTargetDescription,
            .num_color_targets = 1,
        },
    };

    pipeline = SDL_CreateGPUGraphicsPipeline(context->GPUDevice, &pipelineCreateInfo);
    if (pipeline == NULL) {
        SDL_LogError(1, "Failed creating graphics pipeline error: %s", SDL_GetError());
    }

    SDL_ReleaseGPUShader(context->GPUDevice, vertexShader);
    SDL_ReleaseGPUShader(context->GPUDevice, fragmentShader);

    SDL_GPUBufferCreateInfo vertexBufferCreateInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(PositionVertex) * 4
    };
    vertexBuffer = SDL_CreateGPUBuffer(
        context->GPUDevice,
        &vertexBufferCreateInfo
    );

    SDL_GPUBufferCreateInfo indexBufferCreateInfo = {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(uint16_t) * 6
    };
    indexBuffer = SDL_CreateGPUBuffer(
        context->GPUDevice,
        &indexBufferCreateInfo
    );

    SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (sizeof(PositionVertex) * 4) + (sizeof(uint16_t) * 6),
    };
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
        context->GPUDevice,
        &transferBufferCreateInfo
    );

    PositionVertex* transferData = static_cast<PositionVertex*>(SDL_MapGPUTransferBuffer(
        context->GPUDevice,
        transferBuffer,
        false
    ));

    transferData[0] = (PositionVertex){ -0.5f, -0.5f, 0};
    transferData[1] = (PositionVertex){  0.5f, -0.5f, 0};
    transferData[2] = (PositionVertex){  0.5f,  0.5f, 0};
    transferData[3] = (PositionVertex){ -0.5f,  0.5f, 0};

    Uint16* indexData = (Uint16*) &transferData[4];
    indexData[0] = 0;
    indexData[1] = 1;
    indexData[2] = 2;
    indexData[3] = 0;
    indexData[4] = 2;
    indexData[5] = 3;

    SDL_UnmapGPUTransferBuffer(context->GPUDevice, transferBuffer);


    // Upload the transfer data to the vertex and index buffer
    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(context->GPUDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

    SDL_GPUTransferBufferLocation transferVertexBufferLocation =  {
        .transfer_buffer = transferBuffer,
        .offset = 0
    };
    SDL_GPUBufferRegion vertexBufferRegion = {
        .buffer = vertexBuffer,
        .offset = 0,
        .size = sizeof(PositionVertex) * 4
    };
    SDL_UploadToGPUBuffer(
        copyPass,
        &transferVertexBufferLocation,
        &vertexBufferRegion,
        false
    );

    SDL_GPUTransferBufferLocation transferIndexBufferLocation = {
        .transfer_buffer = transferBuffer,
        .offset = sizeof(PositionVertex) * 4
    };
    SDL_GPUBufferRegion indexBufferRegion = {
        .buffer = indexBuffer,
        .offset = 0,
        .size = sizeof(Uint16) * 6
    };
    SDL_UploadToGPUBuffer(
        copyPass,
        &transferIndexBufferLocation,
        &indexBufferRegion,
        false
    );

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
    SDL_ReleaseGPUTransferBuffer(context->GPUDevice, transferBuffer);

    return 0;
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(1, "Failed to init video error: %s", SDL_GetError());
        return -1;
    }
    InitAssetLoader();
    Init(&context);

    bool running = true;
    SDL_Event e;
    while (running) {
        GradientUniformValues.time += 0.1f;
        SDL_GetMouseState(&context.mousPos.x, &context.mousPos.y);

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                running = false;
            }
        }

        SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(context.GPUDevice);
        if (cmdbuf == NULL) {
            SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
            return -1;
        }

        SDL_GPUTexture* swapchainTexture;
        if (!SDL_AcquireGPUSwapchainTexture(cmdbuf, context.window, &swapchainTexture, NULL, NULL)) {
            SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
            return -1;
        }

        if (swapchainTexture != NULL) {
            SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
            colorTargetInfo.texture = swapchainTexture;
            colorTargetInfo.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
            SDL_GPUBufferBinding vertexBufferBinding = {
                .buffer = vertexBuffer,
                .offset = 0
            };
            SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
            SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

            SDL_GPUBufferBinding indexBufferBinding = {
                .buffer = indexBuffer,
                .offset = 0
            };
            SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            //SDL_PushGPUFragmentUniformData(cmdbuf, 1, &context.mousPos, sizeof(context.mousPos));
            //SDL_PushGPUFragmentUniformData(cmdbuf, 2, &context.windowSize, sizeof(context.windowSize));
            SDL_PushGPUFragmentUniformData(cmdbuf, 0, &GradientUniformValues, sizeof(GradientUniformValues));
            SDL_Log("%f", GradientUniformValues.time);

            SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

            SDL_EndGPURenderPass(renderPass);
        }

        SDL_SubmitGPUCommandBuffer(cmdbuf);
    }

    Quit(&context);
    return 0;
}

void Quit(Context* context) {
    SDL_ReleaseGPUGraphicsPipeline(context->GPUDevice, pipeline);
    SDL_ReleaseGPUBuffer(context->GPUDevice, vertexBuffer);
    SDL_ReleaseGPUBuffer(context->GPUDevice, indexBuffer);

    GeneralQuit(context);
}
