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

constexpr size_t MaxLightCount = 4;
constexpr size_t MaxMaterialCount = 256;

struct CameraUniformData
{
    Matrix4x4 Matrix;
    Vector3 Position;
};

struct MeshData
{
    struct InstanceData
    {
        Vector3 Position;
    };

    Buffer VertexBuffer;
    Buffer InstanceBuffer;
    Vector3 BaseColor = { 0.0f, 0.0f, 0.0f };
};

struct SharedResources
{
    std::vector<MeshData> Meshes;
    Buffer CameraUniformBuffer;
    CameraUniformData CameraUniform;
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
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddBuffer(this->sharedResources.CameraUniformBuffer,   BufferUsage::TRANSFER_DESTINATION);
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

        FillUniform(this->sharedResources.CameraUniform, this->sharedResources.CameraUniformBuffer);
    }
};

class ComputeRenderPass : public RenderPass
{    
    BufferReference computeBuffer;
public:

    ComputeRenderPass(BufferReference computeBuffer)
        : computeBuffer(computeBuffer)
    {
        
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<ComputeShader>(
            ShaderLoader::LoadFromSourceFile("main_compute.glsl", ShaderType::COMPUTE, ShaderLanguage::GLSL)
        );

        pipeline.DeclareBuffer(computeBuffer.get());

        pipeline.DescriptorBindings
            .Bind(0, computeBuffer, UniformType::STORAGE_BUFFER);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddBuffer(computeBuffer.get(), BufferUsage::STORAGE_BUFFER);
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        struct
        {
            uint32_t InstanceCount;
            float TimeDelta;
        } computeShaderInfo;
        computeShaderInfo.InstanceCount = computeBuffer.get().GetByteSize() / sizeof(MeshData::InstanceData);
        computeShaderInfo.TimeDelta = ImGui::GetIO().DeltaTime;

        state.Commands.PushConstants(state.Pass, &computeShaderInfo);
        state.Commands.Dispatch(1, 0, 0);
    }
};

class OpaqueRenderPass : public RenderPass
{
    SharedResources& sharedResources;
public:

    OpaqueRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                5
            },
            VertexBinding{
                VertexBinding::Rate::PER_INSTANCE,
                1
            }
        };

        pipeline.DeclareAttachment("Output"_id, Format::R8G8B8A8_UNORM);
        pipeline.DeclareAttachment("OutputDepth"_id, Format::D32_SFLOAT_S8_UINT);

        pipeline.DescriptorBindings
            .Bind(0, sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddAttachment("Output"_id, ClearColor{ 0.3f, 0.4f, 0.7f });
        depedencies.AddAttachment("OutputDepth"_id, ClearDepthStencil{ });

        depedencies.AddBuffer(sharedResources.CameraUniformBuffer, BufferUsage::UNIFORM_BUFFER);
        for (const auto& mesh : sharedResources.Meshes)
        {
            depedencies.AddBuffer(mesh.VertexBuffer, BufferUsage::VERTEX_BUFFER);
            depedencies.AddBuffer(mesh.InstanceBuffer, BufferUsage::VERTEX_BUFFER);
        }
    }

    virtual void OnRender(RenderPassState state) override
    {
        state.Commands.SetRenderArea(state.GetAttachment("Output"_id));

        for (const auto& mesh : sharedResources.Meshes)
        {
            size_t vertexCount = mesh.VertexBuffer.GetByteSize() / sizeof(ModelData::Vertex);
            size_t instanceCount = mesh.InstanceBuffer.GetByteSize() / sizeof(MeshData::InstanceData);
            state.Commands.BindVertexBuffers(mesh.VertexBuffer, mesh.InstanceBuffer);
            state.Commands.PushConstants(state.Pass, &mesh.BaseColor);
            state.Commands.Draw(vertexCount, instanceCount);
        }
    }
};

auto CreateRenderGraph(SharedResources& resources, RenderGraphOptions::Value options)
{
    auto& cubeInstances = resources.Meshes[1].InstanceBuffer;

    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("UniformSubmitPass"_id, std::make_unique<UniformSubmitRenderPass>(resources))
        .AddRenderPass("ComputePass"_id, std::make_unique<ComputeRenderPass>(cubeInstances))
        .AddRenderPass("OpaquePass"_id, std::make_unique<OpaqueRenderPass>(resources))
        .AddRenderPass("ImGuiPass"_id, std::make_unique<ImGuiRenderPass>("Output"_id))
        .SetOptions(options)
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build();
}

struct Camera
{
    Vector3 Position{ 5.0f, 2.0f, 0.0f };
    Vector2 Rotation{ 1.5f * Pi, 0.0f };
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
void LoadData(Buffer& buffer, BufferUsage::Value usage, ArrayView<T> data)
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
        { }, // vertex buffers
        Buffer{ sizeof(CameraUniformData), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        { } // camera uniform
    };

    std::array planeVertices = {
        ModelData::Vertex{ { -500.0f, 0.0f, -500.0f }, { -500.0f, -500.0f }, { 0.0f, 1.0f, 0.0f } },
        ModelData::Vertex{ { -500.0f, 0.0f,  500.0f }, { -500.0f,  500.0f }, { 0.0f, 1.0f, 0.0f } },
        ModelData::Vertex{ {  500.0f, 0.0f,  500.0f }, {  500.0f,  500.0f }, { 0.0f, 1.0f, 0.0f } },
        ModelData::Vertex{ {  500.0f, 0.0f,  500.0f }, {  500.0f,  500.0f }, { 0.0f, 1.0f, 0.0f } },
        ModelData::Vertex{ {  500.0f, 0.0f, -500.0f }, {  500.0f, -500.0f }, { 0.0f, 1.0f, 0.0f } },
        ModelData::Vertex{ { -500.0f, 0.0f, -500.0f }, { -500.0f, -500.0f }, { 0.0f, 1.0f, 0.0f } },
    };
    std::array planePosition = { Vector3{0.0f, 0.0f, 0.0f} };

    auto& planeMesh = sharedResources.Meshes.emplace_back();
    planeMesh.BaseColor = { 0.8f, 0.8f, 0.8f };
    LoadData(planeMesh.VertexBuffer, BufferUsage::VERTEX_BUFFER, MakeView(planeVertices));
    LoadData(planeMesh.InstanceBuffer, BufferUsage::VERTEX_BUFFER, MakeView(planePosition));

    auto cube = ModelLoader::LoadFromObj("models/cube.obj");
    std::array cubeInstances = {
        Vector3{ 0.0f, 0.0f, -5.0f },
        Vector3{ 0.0f, 0.0f, -2.5f },
        Vector3{ 0.0f, 0.0f, -0.0f },
        Vector3{ 0.0f, 0.0f,  2.5f },
        Vector3{ 0.0f, 0.0f,  5.0f },
    };

    for (auto& vertex : cube.Shapes[0].Vertices) vertex.Position.y += 0.5f;
    auto& cubeMesh = sharedResources.Meshes.emplace_back();
    cubeMesh.BaseColor = { 0.8f, 0.0f, 0.0f };
    LoadData(cubeMesh.VertexBuffer, BufferUsage::VERTEX_BUFFER, MakeView(cube.Shapes[0].Vertices));
    LoadData(cubeMesh.InstanceBuffer, BufferUsage::VERTEX_BUFFER | BufferUsage::STORAGE_BUFFER, MakeView(cubeInstances));

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
