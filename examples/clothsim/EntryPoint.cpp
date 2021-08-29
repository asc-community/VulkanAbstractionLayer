#include <filesystem>
#include <iostream>
#include <map>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/ShaderLoader.h"
#include "VulkanAbstractionLayer/ModelLoader.h"
#include "VulkanAbstractionLayer/ImageLoader.h"
#include "VulkanAbstractionLayer/ImGuiRenderPass.h"
#include "VulkanAbstractionLayer/GraphicShader.h"
#include "VulkanAbstractionLayer/ComputeShader.h"

using namespace VulkanAbstractionLayer;

void VulkanInfoCallback(const std::string& message)
{
    std::cout << "[INFO Vulkan]: " << message << std::endl;
}

void VulkanErrorCallback(const std::string& message)
{
    std::cout << "[ERROR Vulkan]: " << message << std::endl;
}

void WindowErrorCallback(const std::string& message)
{
    std::cerr << "[ERROR Window]: " << message << std::endl;
}

constexpr uint32_t ClothSizeX = 16 * 8;
constexpr uint32_t ClothSizeY = 16 * 8;
constexpr uint32_t BallCount = 2;

struct CameraUniformData
{
    Matrix4x4 Matrix;
    Vector3 Position;
};

struct BallStorageData
{
    Vector3 Position{ ClothSizeX * 0.5f, 20.0f, ClothSizeY * 0.5f };
    float Radius = 1.0f;
};

struct SharedResources
{
    Buffer CameraUniformBuffer;
    CameraUniformData CameraUniform;
    Image PositionImage;
    Image VelocityImage;
    Buffer BallVertexBuffer;
    Buffer BallIndexBuffer;
    std::array<BallStorageData, BallCount> BallStorage;
    Buffer BallStorageBuffer;
};

class UniformSubmitRenderPass : public RenderPass
{
    SharedResources& sharedResources;

public:
    UniformSubmitRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {

    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.DeclareBuffer(this->sharedResources.CameraUniformBuffer);
        pipeline.DeclareBuffer(this->sharedResources.BallStorageBuffer);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddBuffer(this->sharedResources.CameraUniformBuffer,   BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.BallStorageBuffer,     BufferUsage::TRANSFER_DESTINATION);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto FillUniform = [&state](const auto& uniformData, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
            auto uniformAllocation = stageBuffer.Submit(&uniformData);
            state.Commands.CopyBuffer(
                BufferInfo{ stageBuffer.GetBuffer(), uniformAllocation.Offset },
                BufferInfo{ uniformBuffer, 0 },
                uniformAllocation.Size
            );
        };
        auto FillUniformArray = [&state](const auto& uniformData, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
            auto uniformAllocation = stageBuffer.Submit(MakeView(uniformData));
            state.Commands.CopyBuffer(
                BufferInfo{ stageBuffer.GetBuffer(), uniformAllocation.Offset },
                BufferInfo{ uniformBuffer, 0 },
                uniformAllocation.Size
            );
        };

        FillUniform(this->sharedResources.CameraUniform, this->sharedResources.CameraUniformBuffer);
        FillUniformArray(this->sharedResources.BallStorage, this->sharedResources.BallStorageBuffer);
    }
};

class ComputeRenderPass : public RenderPass
{
    SharedResources& sharedResources;

    int selectedControlNodeIndex = 0;
    std::array<Vector3, 4> controlNodePositions = {
        Vector3{ 0.0f,       0.0f, 0.0f       },
        Vector3{ 0.0f,       0.0f, ClothSizeY },
        Vector3{ ClothSizeX, 0.0f, 0.0f       },
        Vector3{ ClothSizeX, 0.0f, ClothSizeY },
    };

    std::pair<uint32_t, uint32_t> GetNodeIndicies(int index)
    {
        switch (index)
        {
        case 0:
            return { 0, 0 };
        case 1:
            return { ClothSizeX - 1, 0 };
        case 2:
            return { 0, ClothSizeY -1 };
        case 3:
            return { ClothSizeX - 1, ClothSizeY - 1 };
        default:
            return { -1, -1 };
        }
    }
public:

    ComputeRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<ComputeShader>(
            ShaderLoader::LoadFromSourceFile("main_compute.glsl", ShaderType::COMPUTE, ShaderLanguage::GLSL)
        );

        pipeline.DeclareImage(sharedResources.PositionImage, ImageUsage::TRANSFER_DISTINATION);
        pipeline.DeclareImage(sharedResources.VelocityImage, ImageUsage::TRANSFER_DISTINATION);

        pipeline.DescriptorBindings
            .Bind(0, sharedResources.PositionImage, UniformType::STORAGE_IMAGE)
            .Bind(1, sharedResources.VelocityImage, UniformType::STORAGE_IMAGE)
            .Bind(2, sharedResources.BallStorageBuffer, UniformType::UNIFORM_BUFFER);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {

    }

    virtual void BeforeRender(RenderPassState state) override
    {
        ImGui::Begin("node control");
        ImGui::DragInt("node index", &this->selectedControlNodeIndex, 0.1f, 0, 3);
        ImGui::DragFloat3("node velocity", &this->controlNodePositions[this->selectedControlNodeIndex][0], 0.1f);
        ImGui::End();
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        struct
        {
            Vector4 NodeVelocity;
            uint32_t NodeIndexX;
            uint32_t NodeIndexY;
        } pushConstants;

        auto indices = GetNodeIndicies(this->selectedControlNodeIndex);
        pushConstants.NodeIndexX = indices.first;
        pushConstants.NodeIndexY = indices.second;
        pushConstants.NodeVelocity = Vector4(this->controlNodePositions[this->selectedControlNodeIndex], 0.0f);

        state.Commands.PushConstants(state.Pass, &pushConstants);
        state.Commands.Dispatch(this->sharedResources.PositionImage.GetWidth() / 16, this->sharedResources.PositionImage.GetHeight() / 16, 1);
    }
};

class OpaqueRenderPass : public RenderPass
{
    SharedResources& sharedResources;
    Sampler textureSampler;
public:

    OpaqueRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->textureSampler.Init(
            Sampler::MinFilter::LINEAR, 
            Sampler::MagFilter::LINEAR, 
            Sampler::AddressMode::REPEAT, 
            Sampler::MipFilter::LINEAR
        );
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("cloth_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.DeclareAttachment("Output"_id, Format::R8G8B8A8_UNORM);
        pipeline.DeclareAttachment("OutputDepth"_id, Format::D32_SFLOAT_S8_UINT);

        pipeline.DescriptorBindings
            .Bind(0, sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, sharedResources.PositionImage, this->textureSampler, UniformType::COMBINED_IMAGE_SAMPLER);

        pipeline.AddOutputAttachment("Output"_id, ClearColor{ 0.3f, 0.4f, 0.7f });
        pipeline.AddOutputAttachment("OutputDepth"_id, ClearDepthStencil{ });
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
    }

    virtual void OnRender(RenderPassState state) override
    {
        state.Commands.SetRenderArea(state.GetAttachment("Output"_id));

        uint32_t width = sharedResources.PositionImage.GetWidth();
        uint32_t height = sharedResources.PositionImage.GetHeight();
        uint32_t quadCount = (width - 1) * (height - 1);

        struct
        {
            Vector3 BaseColor;
            float QuadsPerRow;
        } pushConstants;
        pushConstants.BaseColor = Vector3{ 0.8f, 0.0f, 0.0f };
        pushConstants.QuadsPerRow = width - 1;

        size_t vertexCount = 12 * quadCount;
        state.Commands.PushConstants(state.Pass, &pushConstants);
        state.Commands.Draw(vertexCount, 1);
    }
};

class BallRenderPass : public RenderPass
{
    SharedResources& sharedResources;
public:

    BallRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {

    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("ball_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                5
            }
        };

        pipeline.DescriptorBindings
            .Bind(0, sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, sharedResources.BallStorageBuffer, UniformType::UNIFORM_BUFFER);

        pipeline.AddOutputAttachment("Output"_id, AttachmentState::LOAD_COLOR);
        pipeline.AddOutputAttachment("OutputDepth"_id, AttachmentState::LOAD_DEPTH_SPENCIL);
    }

    virtual void OnRender(RenderPassState state) override
    {
        state.Commands.SetRenderArea(state.GetAttachment("Output"_id));

        struct
        {
            Vector3 BaseColor;
            float Padding;
        } pushConstants;
        pushConstants.BaseColor = Vector3{ 0.0f, 0.8f, 0.0f };

        size_t indexCount = this->sharedResources.BallIndexBuffer.GetByteSize() / sizeof(ModelData::Index);
        state.Commands.BindVertexBuffers(this->sharedResources.BallVertexBuffer);
        state.Commands.BindIndexBufferUInt32(this->sharedResources.BallIndexBuffer);
        state.Commands.PushConstants(state.Pass, &pushConstants);
        state.Commands.DrawIndexed(indexCount, this->sharedResources.BallStorage.size());
    }
};

auto CreateRenderGraph(SharedResources& resources, RenderGraphOptions::Value options)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("UniformSubmitPass"_id, std::make_unique<UniformSubmitRenderPass>(resources))
        .AddRenderPass("ComputePass"_id, std::make_unique<ComputeRenderPass>(resources))
        .AddRenderPass("ClothPass"_id, std::make_unique<OpaqueRenderPass>(resources))
        .AddRenderPass("BallPass"_id, std::make_unique<BallRenderPass>(resources))
        .AddRenderPass("ImGuiPass"_id, std::make_unique<ImGuiRenderPass>("Output"_id))
        .SetOptions(options)
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build();
}

struct Camera
{
    Vector3 Position{ 0.0f, 25.0f, 50.0f };
    Vector2 Rotation{ 2.2f, -1.0f };
    float Fov = 65.0f;
    float MovementSpeed = 10.00f;
    float RotationMovementSpeed = 2.5f;
    float AspectRatio = 16.0f / 9.0f;
    float ZNear = 0.1f;
    float ZFar = 5000.0f;

    void Rotate(const Vector2& delta)
    {
        this->Rotation += this->RotationMovementSpeed * delta;

        constexpr float MaxAngleY = HalfPi - 0.001f;
        constexpr float MaxAngleX = TwoPi;
        this->Rotation.y = std::clamp(this->Rotation.y, -MaxAngleY, MaxAngleY);
        this->Rotation.x = std::fmod(this->Rotation.x, MaxAngleX);
    }
    
    void Move(const Vector3& direction)
    {
        Matrix3x3 view{
            std::sin(Rotation.x), 0.0f, std::cos(Rotation.x), // forward
            0.0f, 1.0f, 0.0f, // up
            std::sin(Rotation.x - HalfPi), 0.0f, std::cos(Rotation.x - HalfPi) // right
        };

        this->Position += this->MovementSpeed * (view * direction);
    }
    
    Matrix4x4 GetViewMatrix() const
    {
        Vector3 direction{
            std::cos(this->Rotation.y) * std::sin(this->Rotation.x),
            std::sin(this->Rotation.y),
            std::cos(this->Rotation.y) * std::cos(this->Rotation.x)
        };
        return MakeLookAtMatrix(this->Position, direction, Vector3{0.0f, 1.0f, 0.0f});
    }

    Matrix4x4 GetProjectionMatrix() const
    {
        return MakePerspectiveMatrix(ToRadians(this->Fov), this->AspectRatio, this->ZNear, this->ZFar);
    }

    Matrix4x4 GetMatrix()
    {
        return this->GetProjectionMatrix() * this->GetViewMatrix();
    }
};

template<typename T>
void LoadImageData(Image& image, uint32_t width, uint32_t height, Format format, ImageUsage::Value usage, ArrayView<T> data)
{
    assert(data.size() == width * height);
    image.Init(
        width,
        height,
        format,
        usage | ImageUsage::TRANSFER_DISTINATION,
        MemoryUsage::GPU_ONLY,
        ImageOptions::DEFAULT
    );

    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stagingBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    auto allocation = stagingBuffer.Submit(data);
    commandBuffer.Begin();
    commandBuffer.CopyBufferToImage(
        BufferInfo{ stagingBuffer.GetBuffer(), allocation.Offset },
        ImageInfo{ image, ImageUsage::UNKNOWN, 0, 0 }
    );

    stagingBuffer.Flush();
    commandBuffer.End();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stagingBuffer.Reset();
}

template<typename T>
void LoadBufferData(Buffer& buffer, BufferUsage::Value usage, ArrayView<T> data)
{
    buffer.Init(
        data.size() * sizeof(T),
        usage | BufferUsage::TRANSFER_DESTINATION,
        MemoryUsage::GPU_ONLY
    );

    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stagingBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    auto allocation = stagingBuffer.Submit(data);
    commandBuffer.Begin();
    commandBuffer.CopyBuffer(
        BufferInfo{ stagingBuffer.GetBuffer(), allocation.Offset },
        BufferInfo{ buffer, 0 },
        allocation.Size
    );

    stagingBuffer.Flush();
    commandBuffer.End();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stagingBuffer.Reset();
}

int main()
{
    if(std::filesystem::exists(APPLICATION_WORKING_DIRECTORY))
        std::filesystem::current_path(APPLICATION_WORKING_DIRECTORY);

    WindowCreateOptions windowOptions;
    windowOptions.Position = { 100.0f, 100.0f };
    windowOptions.Size = { 1728.0f, 972.0f };
    windowOptions.ErrorCallback = WindowErrorCallback;

    Window window(windowOptions);

    VulkanContextCreateOptions vulkanOptions;
    vulkanOptions.VulkanApiMajorVersion = 1;
    vulkanOptions.VulkanApiMinorVersion = 2;
    vulkanOptions.Extensions = window.GetRequiredExtensions();
    vulkanOptions.Layers = { "VK_LAYER_KHRONOS_validation" };
    vulkanOptions.ErrorCallback = VulkanErrorCallback;
    vulkanOptions.InfoCallback = VulkanInfoCallback;

    VulkanContext Vulkan(vulkanOptions);
    SetCurrentVulkanContext(Vulkan);

    ContextInitializeOptions deviceOptions;
    deviceOptions.PreferredDeviceType = DeviceType::DISCRETE_GPU;
    deviceOptions.ErrorCallback = VulkanErrorCallback;
    deviceOptions.InfoCallback = VulkanInfoCallback;

    Vulkan.InitializeContext(window.CreateWindowSurface(Vulkan), deviceOptions);

    SharedResources sharedResources{
        Buffer{ sizeof(CameraUniformData), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        { }, // camera uniform
        Image{ }, // position image
        Image{ }, // velocity image
        Buffer{ }, // ball vertex data
        Buffer{ }, // ball index data
        { BallStorageData{ { 0.0f, 20.0f, 0.0f }, 5.0f }, BallStorageData{ { ClothSizeX, 30.0f, ClothSizeY }, 20.0f } },
        Buffer{ sizeof(BallStorageData) * BallCount, BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
    };
    
    std::vector<Vector4> positions(ClothSizeX * ClothSizeY);
    for (size_t x = 0; x < ClothSizeX; x++)
    {
        for (size_t y = 0; y < ClothSizeY; y++)
        {
            positions[x * ClothSizeY + y] = Vector4{ (float)x, 0.0f, (float)y, 1.0f };
        }
    }
    LoadImageData(
        sharedResources.PositionImage,
        ClothSizeX,
        ClothSizeY,
        Format::R32G32B32A32_SFLOAT,
        ImageUsage::STORAGE | ImageUsage::SHADER_READ,
        MakeView(positions)
    );
    std::vector<Vector4> velocities(ClothSizeX * ClothSizeY, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
    LoadImageData(
        sharedResources.VelocityImage,
        ClothSizeX,
        ClothSizeY,
        Format::R32G32B32A32_SFLOAT,
        ImageUsage::STORAGE,
        MakeView(velocities)
    );

    auto ballModel = ModelLoader::LoadFromObj("../models/sphere/sphere.obj");
    for (auto& vertex : ballModel.Shapes[0].Vertices)
        vertex.Position = Normalize(vertex.Position);
    LoadBufferData(
        sharedResources.BallVertexBuffer,
        BufferUsage::VERTEX_BUFFER,
        MakeView(ballModel.Shapes[0].Vertices)
    );
    LoadBufferData(
        sharedResources.BallIndexBuffer,
        BufferUsage::INDEX_BUFFER,
        MakeView(ballModel.Shapes[0].Indices)
    );

    std::unique_ptr<RenderGraph> renderGraph = CreateRenderGraph(sharedResources, RenderGraphOptions::Value{ });

    Camera camera;

    window.OnResize([&Vulkan, &sharedResources, &renderGraph, &camera](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        renderGraph = CreateRenderGraph(sharedResources, RenderGraphOptions::ON_SWAPCHAIN_RESIZE);
        camera.AspectRatio = size.x / size.y;
    });
    
    ImGuiVulkanContext::Init(window, renderGraph->GetNodeByName("ImGuiPass"_id).PassNative.RenderPassHandle);

    while (!window.ShouldClose())
    {
        window.PollEvents();

        if (Vulkan.IsRenderingEnabled())
        {
            Vulkan.StartFrame();
            ImGuiVulkanContext::StartFrame();
            
            auto dt = ImGui::GetIO().DeltaTime;
            
            auto mouseMovement = ImGui::GetMouseDragDelta(MouseButton::RIGHT, 0.0f);
            ImGui::ResetMouseDragDelta(MouseButton::RIGHT);
            camera.Rotate(Vector2{ -mouseMovement.x, -mouseMovement.y } * dt);
            
            Vector3 movementDirection{ 0.0f };
            if (ImGui::IsKeyDown(KeyCode::W))
                movementDirection += Vector3{  1.0f,  0.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::A))
                movementDirection += Vector3{  0.0f,  0.0f, -1.0f };
            if (ImGui::IsKeyDown(KeyCode::S))
                movementDirection += Vector3{ -1.0f,  0.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::D))
                movementDirection += Vector3{  0.0f,  0.0f,  1.0f };
            if (ImGui::IsKeyDown(KeyCode::SPACE))
                movementDirection += Vector3{  0.0f,  1.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::LEFT_SHIFT))
                movementDirection += Vector3{  0.0f, -1.0f,  0.0f };
            if (movementDirection != Vector3{ 0.0f }) movementDirection = Normalize(movementDirection);
            camera.Move(movementDirection * dt);
            
            ImGui::Begin("Camera");
            ImGui::DragFloat("movement speed", &camera.MovementSpeed, 0.1f);
            ImGui::DragFloat("rotation movement speed", &camera.RotationMovementSpeed, 0.1f);
            ImGui::DragFloat3("position", &camera.Position[0]);
            ImGui::DragFloat2("rotation", &camera.Rotation[0], 0.01f);
            ImGui::DragFloat("fov", &camera.Fov);
            ImGui::End();

            ImGui::Begin("Performace");
            ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
            ImGui::End();

            ImGui::Begin("Balls");
            int ballIndex = 0;
            for (auto& ball : sharedResources.BallStorage)
            {
                ImGui::PushID(ballIndex++);
                ImGui::DragFloat3("position", &ball.Position[0], 0.5f);
                ImGui::DragFloat("radius", &ball.Radius, 0.5f, 0.0f);
                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::End();
            
            sharedResources.CameraUniform.Matrix = camera.GetMatrix();
            sharedResources.CameraUniform.Position = camera.Position;
            
            renderGraph->Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph->Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.AcquireCurrentSwapchainImage(ImageUsage::TRANSFER_DISTINATION));
            
            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
