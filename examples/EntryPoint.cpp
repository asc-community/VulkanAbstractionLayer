#include <filesystem>
#include <iostream>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/ShaderLoader.h"
#include "VulkanAbstractionLayer/ModelLoader.h"
#include "VulkanAbstractionLayer/ImageLoader.h"
#include "VulkanAbstractionLayer/ImGuiRenderPass.h"
#include "imgui.h"

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

struct Mesh
{
    Buffer VertexBuffer;
    Buffer InstanceBuffer;
    std::vector<Image> Textures;
};

struct InstanceData
{
    Vector3 Position;
    uint32_t AlbedoTextureIndex;
};

struct SharedResources
{
    Buffer CameraUniformBuffer;
    Buffer ModelUniformBuffer;
    Buffer LightUniformBuffer;
    std::vector<Mesh> Meshes;
    std::vector<ImageReference> MeshTextures;
};

Mesh CreateMesh(const std::vector<ModelData::Vertex>& vertices, const std::vector<InstanceData>& instances, ArrayView<ImageData> textures)
{
    Mesh result;

    auto& vulkanContext = GetCurrentVulkanContext();
    auto& stageBuffer = vulkanContext.GetCurrentStageBuffer();
    auto& commandBuffer = vulkanContext.GetCurrentCommandBuffer();

    commandBuffer.Begin();

    auto instanceAllocation = stageBuffer.Submit(MakeView(instances));
    auto vertexAllocation = stageBuffer.Submit(MakeView(vertices));

    result.InstanceBuffer.Init(instanceAllocation.Size, BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);
    result.VertexBuffer.Init(vertexAllocation.Size, BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);

    commandBuffer.CopyBuffer(stageBuffer.GetBuffer(), instanceAllocation.Offset, result.InstanceBuffer, 0, instanceAllocation.Size);
    commandBuffer.CopyBuffer(stageBuffer.GetBuffer(), vertexAllocation.Offset, result.VertexBuffer, 0, vertexAllocation.Size);

    for (const auto& texture : textures)
    {
        auto& image = result.Textures.emplace_back();
        image.Init(
            texture.Width,
            texture.Height,
            Format::R8G8B8A8_UNORM,
            ImageUsage::TRANSFER_DISTINATION | ImageUsage::SHADER_READ,
            MemoryUsage::GPU_ONLY
        );

        auto textureAllocation = stageBuffer.Submit(texture.ByteData, texture.GetByteSize());

        commandBuffer.CopyBufferToImage(stageBuffer.GetBuffer(), textureAllocation.Offset, image, ImageUsage::UNKNOWN, textureAllocation.Size);
    }

    stageBuffer.Flush();
    commandBuffer.End();

    vulkanContext.SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();

    return result;
}

Mesh CreatePlaneMesh(uint32_t& globalTextureIndex)
{
    std::vector vertices = {
        ModelData::Vertex{ { -500.0f, -500.0f, -0.01f }, { -15.0f, -15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ {  500.0f,  500.0f, -0.01f }, {  15.0f,  15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ { -500.0f,  500.0f, -0.01f }, { -15.0f,  15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ {  500.0f,  500.0f, -0.01f }, {  15.0f,  15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ { -500.0f, -500.0f, -0.01f }, { -15.0f, -15.0f }, { 0.0f, 0.0f, 1.0f } },
        ModelData::Vertex{ {  500.0f, -500.0f, -0.01f }, {  15.0f, -15.0f }, { 0.0f, 0.0f, 1.0f } },
    };
    std::vector instances = {
        InstanceData{ { 0.0f, 0.0f, 0.0f }, globalTextureIndex++ },
    };

    auto texture = ImageLoader::LoadFromFile("models/sand.png");

    return CreateMesh(vertices, instances, MakeView(std::array{ texture }));
}

Mesh CreateDragonMesh(uint32_t& globalTextureIndex)
{
    std::vector instances = {
        InstanceData{ { 0.0f, 0.0f, -40.0f }, globalTextureIndex++ },
        InstanceData{ { 0.0f, 0.0f, -20.0f }, globalTextureIndex++ },
        InstanceData{ { 0.0f, 0.0f,   0.0f }, globalTextureIndex++ },
        InstanceData{ { 0.0f, 0.0f,  20.0f }, globalTextureIndex++ },
        InstanceData{ { 0.0f, 0.0f,  40.0f }, globalTextureIndex++ },
    };

    auto model = ModelLoader::LoadFromObj("models/dragon.obj");
    auto& vertices = model.Shapes.front().Vertices;

    std::array textureData = {
        std::array<uint8_t, 4>{ 255, 255, 255, 255 },
        std::array<uint8_t, 4>{ 150, 225, 100, 255 },
        std::array<uint8_t, 4>{ 100, 150, 225, 255 },
        std::array<uint8_t, 4>{ 255, 220,  60, 255 },
        std::array<uint8_t, 4>{ 150, 150, 150, 255 },
    };

    std::array textures = {
        ImageData{ textureData[0].data(), 1, 1, 4, sizeof(uint8_t) },
        ImageData{ textureData[1].data(), 1, 1, 4, sizeof(uint8_t) },
        ImageData{ textureData[2].data(), 1, 1, 4, sizeof(uint8_t) },
        ImageData{ textureData[3].data(), 1, 1, 4, sizeof(uint8_t) },
        ImageData{ textureData[4].data(), 1, 1, 4, sizeof(uint8_t) },
    };

    return CreateMesh(vertices, instances, textures);
}

class UniformSubmitRenderPass : public RenderPass
{
    SharedResources& sharedResources;

public:
    struct CameraUniformData
    {
        Matrix4x4 Matrix;
        Vector3 Position;
    } CameraUniform;

    struct ModelUniformData
    {
        Matrix3x4 Matrix;
    } ModelUniform;

    struct LightUniformData
    {
        Matrix4x4 Projection;
        Vector3 Color;
        float AmbientIntensity;
        Vector3 Direction;
    } LightUniform;

    UniformSubmitRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {

    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.DeclareBuffer(this->sharedResources.CameraUniformBuffer, BufferUsage::UNKNOWN);
        pipeline.DeclareBuffer(this->sharedResources.ModelUniformBuffer, BufferUsage::UNKNOWN);
        pipeline.DeclareBuffer(this->sharedResources.LightUniformBuffer, BufferUsage::UNKNOWN);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddBuffer(this->sharedResources.CameraUniformBuffer, BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.ModelUniformBuffer, BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.LightUniformBuffer, BufferUsage::TRANSFER_DESTINATION);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto FillUniform = [&state](const auto& uniformData, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
            auto uniformAllocation = stageBuffer.Submit(&uniformData);
            state.Commands.CopyBuffer(stageBuffer.GetBuffer(), uniformAllocation.Offset, uniformBuffer, 0, uniformAllocation.Size);
        };

        FillUniform(this->CameraUniform, this->sharedResources.CameraUniformBuffer);
        FillUniform(this->ModelUniform, this->sharedResources.ModelUniformBuffer);
        FillUniform(this->LightUniform, this->sharedResources.LightUniformBuffer);
    }
};

class ShadowRenderPass : public RenderPass
{
    SharedResources& sharedResources;
public:
    ShadowRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {

    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = GraphicShader{
            ShaderLoader::LoadFromSourceFile("shadow_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("shadow_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL),
        };
        
        pipeline.DeclareAttachment("ShadowDepth"_id, Format::D32_SFLOAT_S8_UINT, 2048, 2048);

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                3,
            },
            VertexBinding{
                VertexBinding::Rate::PER_INSTANCE,
                2,
            },
        };

        pipeline.DescriptorBindings
            .Bind(1, this->sharedResources.ModelUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(2, this->sharedResources.LightUniformBuffer, UniformType::UNIFORM_BUFFER);
    }

    virtual void SetupDependencies(DependencyState dependencies) override
    {
        dependencies.AddAttachment("ShadowDepth"_id, ClearDepthStencil{ });
        dependencies.AddBuffer(this->sharedResources.CameraUniformBuffer, BufferUsage::UNIFORM_BUFFER);
        dependencies.AddBuffer(this->sharedResources.ModelUniformBuffer, BufferUsage::UNIFORM_BUFFER);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("ShadowDepth"_id);
        state.Commands.SetRenderArea(output);

        for (const auto& mesh : this->sharedResources.Meshes)
        {
            size_t vertexCount = mesh.VertexBuffer.GetByteSize() / sizeof(ModelData::Vertex);
            size_t instanceCount = mesh.InstanceBuffer.GetByteSize() / sizeof(Vector3);
            state.Commands.BindVertexBuffers(mesh.VertexBuffer, mesh.InstanceBuffer);
            state.Commands.Draw((uint32_t)vertexCount, (uint32_t)instanceCount);
        }
    }
};

class OpaqueRenderPass : public RenderPass
{    
    SharedResources& sharedResources;
    Sampler textureSampler;
    Sampler depthSampler;
public:
    OpaqueRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->textureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);
        this->depthSampler.Init(Sampler::MinFilter::NEAREST, Sampler::MagFilter::NEAREST, Sampler::AddressMode::CLAMP_TO_EDGE, Sampler::MipFilter::NEAREST);
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = GraphicShader{
            ShaderLoader::LoadFromSourceFile("main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL),
        };

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                3,
            },
            VertexBinding{
                VertexBinding::Rate::PER_INSTANCE,
                2,
            },
        };

        pipeline.DeclareImages(this->sharedResources.MeshTextures, ImageUsage::TRANSFER_DISTINATION);
        pipeline.DeclareAttachment("Output"_id, Format::R8G8B8A8_UNORM);
        pipeline.DeclareAttachment("OutputDepth"_id, Format::D32_SFLOAT_S8_UINT);

        pipeline.DescriptorBindings
            .Bind(0, this->sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, this->sharedResources.ModelUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(2, this->sharedResources.LightUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(3, this->textureSampler, UniformType::SAMPLER)
            .Bind(4, this->sharedResources.MeshTextures, UniformType::SAMPLED_IMAGE)
            .Bind(5, "ShadowDepth"_id, UniformType::SAMPLED_IMAGE, ImageView::DEPTH)
            .Bind(6, this->depthSampler, UniformType::SAMPLER);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddAttachment("Output"_id, ClearColor{ 0.5f, 0.8f, 1.0f, 1.0f });
        depedencies.AddAttachment("OutputDepth"_id, ClearDepthStencil{ });

        depedencies.AddBuffer(this->sharedResources.CameraUniformBuffer, BufferUsage::UNIFORM_BUFFER);
        depedencies.AddBuffer(this->sharedResources.ModelUniformBuffer, BufferUsage::UNIFORM_BUFFER);
        depedencies.AddBuffer(this->sharedResources.LightUniformBuffer, BufferUsage::UNIFORM_BUFFER);

        depedencies.AddImages(this->sharedResources.MeshTextures, ImageUsage::SHADER_READ);
        depedencies.AddImage("ShadowDepth"_id, ImageUsage::SHADER_READ);
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output"_id);
        state.Commands.SetRenderArea(output);

        for (const auto& mesh : this->sharedResources.Meshes)
        {
            size_t vertexCount = mesh.VertexBuffer.GetByteSize() / sizeof(ModelData::Vertex);
            size_t instanceCount = mesh.InstanceBuffer.GetByteSize() / sizeof(Vector3);
            state.Commands.BindVertexBuffers(mesh.VertexBuffer, mesh.InstanceBuffer);
            state.Commands.Draw((uint32_t)vertexCount, (uint32_t)instanceCount);
        }
    }
};

auto CreateRenderGraph(SharedResources& resources, RenderGraphOptions::Value options)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("UniformSubmitPass"_id, std::make_unique<UniformSubmitRenderPass>(resources))
        .AddRenderPass("ShadowPass"_id, std::make_unique<ShadowRenderPass>(resources))
        .AddRenderPass("OpaquePass"_id, std::make_unique<OpaqueRenderPass>(resources))
        .AddRenderPass("ImGuiPass"_id, std::make_unique<ImGuiRenderPass>("Output"_id))
        .SetOptions(options)
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build();
}

struct Camera
{
    Vector3 Position{ 40.0f, 25.0f, -90.0f };
    Vector2 Rotation{ 5.74f, 0.0f };
    float Fov = 65.0f;
    float MovementSpeed = 250.0f;
    float RotationMovementSpeed = 2.5f;
    float AspectRatio = 16.0f / 9.0f;
    float ZNear = 0.001f;
    float ZFar = 10000.0f;

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

int main()
{
    std::filesystem::current_path(APPLICATION_WORKING_DIRECTORY);

    WindowCreateOptions windowOptions;
    windowOptions.Position = { 300.0f, 100.0f };
    windowOptions.Size = { 1280.0f, 720.0f };
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
        Buffer{ sizeof(UniformSubmitRenderPass::CameraUniform), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(UniformSubmitRenderPass::ModelUniform),  BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(UniformSubmitRenderPass::LightUniform),  BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        { }, // meshes
        { }, // mesh textures
    };

    uint32_t textureCount = 0;
    sharedResources.Meshes.push_back(CreateDragonMesh(textureCount));
    sharedResources.Meshes.push_back(CreatePlaneMesh(textureCount));
    for (const auto& mesh : sharedResources.Meshes)
        for (const auto& texture : mesh.Textures)
            sharedResources.MeshTextures.push_back(std::ref(texture));

    std::unique_ptr<RenderGraph> renderGraph = CreateRenderGraph(sharedResources, RenderGraphOptions::Value{ });

    Camera camera;
    Vector3 modelRotation{ -HalfPi, Pi, 0.0f };
    Vector3 lightColor{ 0.7f, 0.7f, 0.7f };
    Vector3 lightDirection{ -0.3f, 1.0f, -0.6f };
    float lightBounds = 150.0f;
    float lightAmbientIntensity = 0.3f;

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

            ImGui::Begin("Model");
            ImGui::DragFloat3("rotation", &modelRotation[0], 0.01f);
            ImGui::End();

            ImGui::Begin("Light");
            ImGui::ColorEdit3("color", &lightColor[0], ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
            ImGui::DragFloat3("direction", &lightDirection[0], 0.01f);
            ImGui::DragFloat("bounds", &lightBounds, 0.1f);
            ImGui::DragFloat("ambient intensity", &lightAmbientIntensity, 0.01f);
            ImGui::End();

            Vector3 low { -lightBounds, -lightBounds, -lightBounds };
            Vector3 high{ lightBounds, lightBounds, lightBounds };

            auto& uniformSubmitPass = renderGraph->GetRenderPassByName<UniformSubmitRenderPass>("UniformSubmitPass"_id);
            uniformSubmitPass.CameraUniform.Matrix = camera.GetMatrix();
            uniformSubmitPass.CameraUniform.Position = camera.Position;
            uniformSubmitPass.ModelUniform.Matrix = MakeRotationMatrix(modelRotation);
            uniformSubmitPass.LightUniform.Color = lightColor;
            uniformSubmitPass.LightUniform.AmbientIntensity = lightAmbientIntensity;
            uniformSubmitPass.LightUniform.Direction = Normalize(lightDirection);
            uniformSubmitPass.LightUniform.Projection =
                MakeOrthographicMatrix(low.x, high.x, low.y, high.y, low.z, high.z) *
                MakeLookAtMatrix(camera.Position, -lightDirection, Vector3{ 0.001f, 1.0f, 0.001f });

            renderGraph->Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph->Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.GetCurrentSwapchainImage());

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
