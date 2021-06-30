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

void VulkanInfoCallback(const char* message)
{
    std::cout << "[INFO Vulkan]: " << message << std::endl;
}

void VulkanErrorCallback(const char* message)
{
    std::cout << "[ERROR Vulkan]: " << message << std::endl;
}

void WindowErrorCallback(const char* message)
{
    std::cerr << "[ERROR Window]: " << message << std::endl;
}

struct InstanceData
{
    Vector3 Position;
    uint32_t AlbedoTextureIndex;
};

struct CameraUniformData
{
    Matrix4x4 Matrix;
};

struct ModelUniformData
{
    Matrix3x4 Matrix;
};

struct LightUniformData
{
    Vector3 Color;
    float Padding_1;
    Vector3 Direction;
};

struct Mesh
{
    Buffer VertexBuffer;
    Buffer InstanceBuffer;
    std::vector<Image> Textures;
};

struct RenderGraphResources
{
    Sampler BaseSampler;
    CameraUniformData CameraUniform;
    ModelUniformData ModelUniform;
    LightUniformData LightUniform;
    Buffer CameraUniformBuffer;
    Buffer ModelUniformBuffer;
    Buffer LightUniformBuffer;
    Mesh DragonMesh;
    Mesh PlaneMesh;
};

class OpaqueRenderPass : public RenderPass
{    
    RenderGraphResources& resources;
public:
    OpaqueRenderPass(RenderGraphResources& resources)
        : resources(resources) { }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = GraphicShader{
            ShaderLoader::LoadFromSource("main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSource("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL),
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

        std::vector<DescriptorBinding::ImageRef> imageReferences;
        for (const auto& texture : this->resources.DragonMesh.Textures)
            imageReferences.push_back(std::ref(texture));
        for (const auto& texture : this->resources.PlaneMesh.Textures)
            imageReferences.push_back(std::ref(texture));

        pipeline.DescriptorBindings
            .Bind(0, this->resources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, this->resources.ModelUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(2, this->resources.LightUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(3, this->resources.BaseSampler, UniformType::SAMPLER)
            .Bind(4, imageReferences, UniformType::SAMPLED_IMAGE);
    }

    virtual void SetupDependencies(DependencyState state) override
    {
        state.AddAttachment("Output"_id, ClearColor{ 0.5f, 0.8f, 1.0f, 1.0f });
        state.AddAttachment("OutputDepth"_id, ClearDepthSpencil{ });

        state.AddBuffer("CameraUniform"_id, BufferUsage::UNIFORM_BUFFER);
        state.AddBuffer("ModelUniform"_id, BufferUsage::UNIFORM_BUFFER);
        state.AddBuffer("LightUniform"_id, BufferUsage::UNIFORM_BUFFER);
    }

    virtual void BeforeRender(RenderPassState state) override
    {
        auto MakeBarrier = [](const Buffer& uniformBuffer, vk::AccessFlagBits from, vk::AccessFlagBits to)
        {
            vk::BufferMemoryBarrier barrier;
            barrier
                .setSrcAccessMask(from)
                .setDstAccessMask(to)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setBuffer(uniformBuffer.GetNativeHandle())
                .setSize(uniformBuffer.GetByteSize())
                .setOffset(0);
            return barrier;
        };

        auto FillUniform = [&state](const auto& uniformData, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

            auto uniformAllocation = stageBuffer.Submit(&uniformData);
            state.Commands.CopyBuffer(
                stageBuffer.GetBuffer(), vk::AccessFlagBits::eNoneKHR, uniformAllocation.Offset,
                uniformBuffer, vk::AccessFlagBits::eUniformRead, 0,
                vk::PipelineStageFlagBits::eHost | vk::PipelineStageFlagBits::eVertexShader,
                uniformAllocation.Size
            );
        };

        FillUniform(this->resources.CameraUniform, this->resources.CameraUniformBuffer);
        FillUniform(this->resources.ModelUniform, this->resources.ModelUniformBuffer);
        FillUniform(this->resources.LightUniform, this->resources.LightUniformBuffer);

        MakeBarrier(this->resources.CameraUniformBuffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eUniformRead);
        MakeBarrier(this->resources.ModelUniformBuffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eUniformRead);
        MakeBarrier(this->resources.LightUniformBuffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eUniformRead);
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetOutputColorAttachment(0);
        state.Commands.SetRenderArea(output);

        auto Draw = [&state](const Mesh& mesh)
        {
            size_t vertexCount = mesh.VertexBuffer.GetByteSize() / sizeof(ModelData::Vertex);
            size_t instanceCount = mesh.InstanceBuffer.GetByteSize() / sizeof(Vector3);
            state.Commands.BindVertexBuffers(mesh.VertexBuffer, mesh.InstanceBuffer);
            state.Commands.Draw((uint32_t)vertexCount, (uint32_t)instanceCount);
        };

        Draw(this->resources.DragonMesh);
        Draw(this->resources.PlaneMesh);
    }
};

RenderGraph CreateRenderGraph(RenderGraphResources& resources, const VulkanContext& context)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("OpaquePass"_id, std::make_unique<OpaqueRenderPass>(resources))
        .AddRenderPass("ImGuiPass"_id, std::make_unique<ImGuiRenderPass>("Output"_id))
        .AddAttachment("Output"_id, Format::R8G8B8A8_UNORM)
        .AddAttachment("OutputDepth"_id, Format::D32_SFLOAT_S8_UINT)
        .AddExternalBuffer("CameraUniform"_id, resources.CameraUniformBuffer, BufferUsage::UNIFORM_BUFFER)
        .AddExternalBuffer("ModelUniform"_id, resources.ModelUniformBuffer, BufferUsage::UNIFORM_BUFFER)
        .AddExternalBuffer("LightUniform"_id, resources.LightUniformBuffer, BufferUsage::UNIFORM_BUFFER)
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build();
}

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

    commandBuffer.CopyBuffer(
        stageBuffer.GetBuffer(), vk::AccessFlagBits::eHostWrite, instanceAllocation.Offset,
        result.InstanceBuffer, vk::AccessFlags{ }, 0,
        vk::PipelineStageFlagBits::eHost,
        instanceAllocation.Size
    );
    commandBuffer.CopyBuffer(
        stageBuffer.GetBuffer(), vk::AccessFlagBits::eTransferRead, vertexAllocation.Offset,
        result.VertexBuffer, vk::AccessFlags{ }, 0,
        vk::PipelineStageFlagBits::eTransfer,
        vertexAllocation.Size
    );

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

        commandBuffer.CopyBufferToImage(
            stageBuffer.GetBuffer(), vk::AccessFlagBits::eTransferRead, textureAllocation.Offset,
            image, vk::ImageLayout::eUndefined, vk::AccessFlags{ },
            vk::PipelineStageFlagBits::eTransfer,
            textureAllocation.Size
        );
    }

    vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor,
        0, // base mip level
        1, // level count
        0, // base layer
        1  // layer count
    };

    std::vector<vk::ImageMemoryBarrier> imageBarriers;
    imageBarriers.reserve(result.Textures.size());
    for (const auto& texture : result.Textures)
    {
        vk::ImageMemoryBarrier imageLayoutToShaderRead;
        imageLayoutToShaderRead
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(texture.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        imageBarriers.push_back(imageLayoutToShaderRead);
    }
    commandBuffer.GetNativeHandle().pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        { },
        { },
        { },
        imageBarriers
    );

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

    Sampler sampler(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MinFilter::LINEAR);

    uint32_t globalTextureIndex = 0;

    RenderGraphResources renderGraphResources{
        std::move(sampler),
        { },
        { },
        { },
        Buffer{ sizeof(RenderGraphResources::CameraUniform), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(RenderGraphResources::ModelUniform),  BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(RenderGraphResources::LightUniform),  BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        CreateDragonMesh(globalTextureIndex),
        CreatePlaneMesh(globalTextureIndex),
    };

    RenderGraph renderGraph = CreateRenderGraph(renderGraphResources, Vulkan);

    Camera camera;
    Vector3 modelRotation{ -HalfPi, Pi, 0.0f };
    Vector3 lightColor{ 1.0f, 1.0f, 1.0f };
    Vector3 lightDirection{ 0.0f, 1.0f, 0.0f };

    window.OnResize([&Vulkan, &renderGraphResources, &renderGraph, &camera](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        renderGraph = CreateRenderGraph(renderGraphResources, Vulkan);
        camera.AspectRatio = size.x / size.y;
    });
    
    ImGuiVulkanContext::Init(window, renderGraph.GetNodeByName("ImGuiPass"_id).PassNative.RenderPassHandle);

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
            ImGui::End();

            renderGraphResources.CameraUniform.Matrix = camera.GetMatrix();
            renderGraphResources.ModelUniform.Matrix = MakeRotationMatrix(modelRotation);
            renderGraphResources.LightUniform.Color = lightColor;
            renderGraphResources.LightUniform.Direction = Normalize(lightDirection);

            renderGraph.Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph.Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.GetCurrentSwapchainImage());

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
