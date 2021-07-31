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
    struct Material
    {
        uint32_t AlbedoIndex;
        uint32_t NormalIndex;
        uint32_t Stub_1;
        uint32_t Stub_2;
    };
    constexpr static size_t MaxMaterialCount = 256;

    struct Submesh
    {
        Buffer VertexBuffer;
        uint32_t MaterialIndex;
    };

    std::vector<Submesh> Submeshes;
    std::vector<Material> Materials;
    std::vector<Image> Textures;
};

struct SharedResources
{
    Buffer CameraUniformBuffer;
    Buffer ModelUniformBuffer;
    Buffer MaterialUniformBuffer;
    Buffer LightUniformBuffer;
    Mesh Sponza;
};

Mesh CreateSponza()
{
    Mesh mesh;
    auto sponza = ModelLoader::LoadFromObj("models/Sponza/sponza.obj");
    
    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    commandBuffer.Begin();

    for (const auto& shape : sponza.Shapes)
    {
        auto& submesh = mesh.Submeshes.emplace_back();
        submesh.VertexBuffer.Init(
            shape.Vertices.size() * sizeof(ModelData::Vertex),
            BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, 
            MemoryUsage::GPU_ONLY
        );

        auto allocation = stageBuffer.Submit(MakeView(shape.Vertices));
        commandBuffer.CopyBuffer(stageBuffer.GetBuffer(), allocation.Offset, submesh.VertexBuffer, 0, allocation.Size);

        submesh.MaterialIndex = shape.MaterialIndex;
    }

    stageBuffer.Flush();
    commandBuffer.End();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();

    uint32_t textureIndex = 0;
    for (const auto& material : sponza.Materials)
    {
        commandBuffer.Begin();

        auto& albedoImage = mesh.Textures.emplace_back();
        albedoImage.Init(
            material.AlbedoTexture.Width,
            material.AlbedoTexture.Height,
            Format::R8G8B8A8_UNORM,
            ImageUsage::SHADER_READ | ImageUsage::TRANSFER_DISTINATION,
            MemoryUsage::GPU_ONLY
        );
        auto albedoAllocation = stageBuffer.Submit(MakeView(material.AlbedoTexture.ByteData));
        commandBuffer.CopyBufferToImage(stageBuffer.GetBuffer(), albedoAllocation.Offset, albedoImage, ImageUsage::UNKNOWN, albedoAllocation.Size);

        auto& normalImage = mesh.Textures.emplace_back();
        normalImage.Init(
            material.NormalTexture.Width,
            material.NormalTexture.Height,
            Format::R8G8B8A8_UNORM,
            ImageUsage::SHADER_READ | ImageUsage::TRANSFER_DISTINATION,
            MemoryUsage::GPU_ONLY
        );
        auto normalAllocation = stageBuffer.Submit(MakeView(material.NormalTexture.ByteData));
        commandBuffer.CopyBufferToImage(stageBuffer.GetBuffer(), normalAllocation.Offset, normalImage, ImageUsage::UNKNOWN, normalAllocation.Size);

        mesh.Materials.push_back(Mesh::Material{ textureIndex, textureIndex + 1 });
        textureIndex += 2;

        stageBuffer.Flush();
        commandBuffer.End();
        GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
        stageBuffer.Reset();
    }

    return mesh;
}

void FillMaterialUniform(Buffer& buffer, const std::vector<Mesh::Material>& materials)
{
    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    commandBuffer.Begin();

    auto allocation = stageBuffer.Submit(MakeView(materials));
    commandBuffer.CopyBuffer(stageBuffer.GetBuffer(), allocation.Offset, buffer, 0, allocation.Size);

    commandBuffer.End();
    stageBuffer.Flush();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();
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

class OpaqueRenderPass : public RenderPass
{    
    SharedResources& sharedResources;
    std::vector<ImageReference> textureArray;
    Sampler textureSampler;
public:
    OpaqueRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->textureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);

        for (const auto& texture : this->sharedResources.Sponza.Textures)
        {
            this->textureArray.push_back(std::ref(texture));
        }
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
                VertexBinding::BindingRangeAll
            }
        };

        pipeline.DeclareAttachment("Output"_id, Format::R8G8B8A8_UNORM);
        pipeline.DeclareAttachment("OutputDepth"_id, Format::D32_SFLOAT_S8_UINT);

        pipeline.DescriptorBindings
            .Bind(0, this->sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, this->sharedResources.ModelUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(2, this->sharedResources.MaterialUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(3, this->sharedResources.LightUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(4, this->textureArray, UniformType::SAMPLED_IMAGE)
            .Bind(5, this->textureSampler, UniformType::SAMPLER);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddAttachment("Output"_id, ClearColor{ 0.5f, 0.8f, 1.0f, 1.0f });
        depedencies.AddAttachment("OutputDepth"_id, ClearDepthStencil{ });

        depedencies.AddBuffer(this->sharedResources.CameraUniformBuffer, BufferUsage::UNIFORM_BUFFER);
        depedencies.AddBuffer(this->sharedResources.ModelUniformBuffer, BufferUsage::UNIFORM_BUFFER);
        depedencies.AddBuffer(this->sharedResources.MaterialUniformBuffer, BufferUsage::UNIFORM_BUFFER);
        depedencies.AddBuffer(this->sharedResources.LightUniformBuffer, BufferUsage::UNIFORM_BUFFER);
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output"_id);
        state.Commands.SetRenderArea(output);

        for (const auto& submesh : this->sharedResources.Sponza.Submeshes)
        {
            size_t vertexCount = submesh.VertexBuffer.GetByteSize() / sizeof(ModelData::Vertex);
            state.Commands.PushConstants(state.Pass, &submesh.MaterialIndex);
            state.Commands.BindVertexBuffers(submesh.VertexBuffer);
            state.Commands.Draw((uint32_t)vertexCount, 1);
        }
    }
};

auto CreateRenderGraph(SharedResources& resources, RenderGraphOptions::Value options)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("UniformSubmitPass"_id, std::make_unique<UniformSubmitRenderPass>(resources))
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
        Buffer{ sizeof(UniformSubmitRenderPass::CameraUniform),  BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(UniformSubmitRenderPass::ModelUniform),   BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(Mesh::Material) * Mesh::MaxMaterialCount, BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(UniformSubmitRenderPass::LightUniform),   BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        { }, // sponza
    };

    sharedResources.Sponza = CreateSponza();
    FillMaterialUniform(sharedResources.MaterialUniformBuffer, sharedResources.Sponza.Materials);

    std::unique_ptr<RenderGraph> renderGraph = CreateRenderGraph(sharedResources, RenderGraphOptions::Value{ });

    Camera camera;
    Vector3 modelRotation{ 0.0f, 0.0f, 0.0f };
    Vector3 lightColor{ 0.7f, 0.7f, 0.7f };
    Vector3 lightDirection{ -0.3f, 1.0f, -0.6f };
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
            ImGui::DragFloat("ambient intensity", &lightAmbientIntensity, 0.01f);
            ImGui::End();

            ImGui::Begin("Performace");
            ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
            ImGui::End();

            auto& uniformSubmitPass = renderGraph->GetRenderPassByName<UniformSubmitRenderPass>("UniformSubmitPass"_id);
            uniformSubmitPass.CameraUniform.Matrix = camera.GetMatrix();
            uniformSubmitPass.CameraUniform.Position = camera.Position;
            uniformSubmitPass.ModelUniform.Matrix = MakeRotationMatrix(modelRotation);
            uniformSubmitPass.LightUniform.Color = lightColor;
            uniformSubmitPass.LightUniform.AmbientIntensity = lightAmbientIntensity;
            uniformSubmitPass.LightUniform.Direction = Normalize(lightDirection);

            renderGraph->Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph->Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.GetCurrentSwapchainImage());

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
