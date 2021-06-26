#include <filesystem>
#include <iostream>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/ShaderLoader.h"
#include "VulkanAbstractionLayer/ModelLoader.h"
#include "VulkanAbstractionLayer/ImageLoader.h"
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

template<typename T>
struct UniformData
{
    Buffer Buffer;
    T Data;
};

struct Mesh
{
    Buffer VertexBuffer;
    Buffer InstanceBuffer;
    std::vector<Image> Textures;
};

struct RenderGraphResources
{
    ShaderData OpaquePassVertexShader;
    ShaderData OpaquePassFragmentShader;
    Sampler BaseSampler;
    UniformData<CameraUniformData> CameraUniform;
    UniformData<ModelUniformData> ModelUniform;
    UniformData<LightUniformData> LightUniform;
    Mesh DragonMesh;
    Mesh PlaneMesh;
};

RenderGraph CreateRenderGraph(const RenderGraphResources& resources, const VulkanContext& context)
{
    std::vector<DescriptorBinding::ImageRef> imageReferences;
    for (const auto& texture : resources.DragonMesh.Textures)
        imageReferences.push_back(std::ref(texture));
    for (const auto& texture : resources.PlaneMesh.Textures)
        imageReferences.push_back(std::ref(texture));

    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass(RenderPassBuilder{ "OpaquePass"_id }
            .SetPipeline(GraphicPipeline{
                GraphicShader{
                    resources.OpaquePassVertexShader,
                    resources.OpaquePassFragmentShader,
                },
                {
                    VertexBinding{
                        VertexBinding::Rate::PER_VERTEX,
                        3,
                    },
                    VertexBinding{
                        VertexBinding::Rate::PER_INSTANCE,
                        2,
                    },
                },
                DescriptorBinding{ }
                    .Bind(0, resources.CameraUniform.Buffer, UniformType::UNIFORM_BUFFER)
                    .Bind(1, resources.ModelUniform.Buffer, UniformType::UNIFORM_BUFFER)
                    .Bind(2, resources.LightUniform.Buffer, UniformType::UNIFORM_BUFFER)
                    .Bind(3, resources.BaseSampler, UniformType::SAMPLER)
                    .Bind(4, imageReferences, UniformType::SAMPLED_IMAGE)
            })
            .AddWriteOnlyColorAttachment("Output"_id, ClearColor{ 0.5f, 0.8f, 1.0f, 1.0f })
            .SetWriteOnlyDepthAttachment("OutputDepth"_id, ClearDepthSpencil{ })
            .AddBeforeRenderCallback([&resources](RenderState state)
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

                    auto FillUniform = [&state, stageBufferOffset = (size_t)0](auto& uniform) mutable
                    {
                        auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

                        size_t dataSize = sizeof(uniform.Data);

                        stageBuffer.LoadData((uint8_t*)&uniform.Data, dataSize, stageBufferOffset);
                        state.Commands.CopyBuffer(
                            stageBuffer, vk::AccessFlagBits::eHostWrite, stageBufferOffset,
                            uniform.Buffer, vk::AccessFlagBits::eUniformRead, 0,
                            vk::PipelineStageFlagBits::eHost | vk::PipelineStageFlagBits::eVertexShader,
                            dataSize
                        );
                        stageBufferOffset += dataSize;
                    };

                    FillUniform(resources.CameraUniform);
                    FillUniform(resources.ModelUniform);
                    FillUniform(resources.LightUniform);

                    state.Commands.GetNativeHandle().pipelineBarrier(
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader,
                        { }, // dependency flags
                        { }, // memory barriers
                        {
                            MakeBarrier(resources.CameraUniform.Buffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eUniformRead),
                            MakeBarrier(resources.ModelUniform.Buffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eUniformRead),
                            MakeBarrier(resources.LightUniform.Buffer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eUniformRead),
                        },
                        { } // image barriers
                    );
                })
            .AddOnRenderCallback(
                [&resources](RenderState state)
                {
                    auto& output = state.GetOutputColorAttachment(0);
                    state.Commands.SetRenderArea(output);

                    size_t dragonVertexCount = resources.DragonMesh.VertexBuffer.GetByteSize() / sizeof(ModelData::Vertex);
                    size_t dragonInstanceCount = resources.DragonMesh.InstanceBuffer.GetByteSize() / sizeof(Vector3);
                    state.Commands.BindVertexBuffers(resources.DragonMesh.VertexBuffer, resources.DragonMesh.InstanceBuffer);
                    state.Commands.Draw((uint32_t)dragonVertexCount, (uint32_t)dragonInstanceCount);

                    size_t planeVertexCount = resources.PlaneMesh.VertexBuffer.GetByteSize() / sizeof(ModelData::Vertex);
                    size_t planeInstanceCount = resources.PlaneMesh.InstanceBuffer.GetByteSize() / sizeof(Vector3);
                    state.Commands.BindVertexBuffers(resources.PlaneMesh.VertexBuffer, resources.PlaneMesh.InstanceBuffer);
                    state.Commands.Draw((uint32_t)planeVertexCount, (uint32_t)planeInstanceCount);
                }
            )
        )
        .AddRenderPass(RenderPassBuilder{ "ImGuiPass"_id }
            .AddWriteOnlyColorAttachment("Output"_id, AttachmentInitialState::LOAD)
            .AddOnRenderCallback(
                [](RenderState state)
                {
                    ImGuiVulkanContext::RenderFrame(state.Commands.GetNativeHandle());
                }
            )
        )
        .AddAttachment("Output"_id, Format::R8G8B8A8_UNORM)
        .AddAttachment("OutputDepth"_id, Format::D32_SFLOAT_S8_UINT)
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build();
}

template<typename T>
UniformData<T> CreateUniform()
{
    return { Buffer(sizeof(T), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY), T{ } };
}

Mesh CreateMesh(const std::vector<ModelData::Vertex>& vertices, const std::vector<InstanceData>& instances, ArrayView<ImageData> textures)
{
    Mesh result;

    auto& vulkanContext = GetCurrentVulkanContext();
    auto& stageBuffer = vulkanContext.GetCurrentStageBuffer();
    auto& commandBuffer = vulkanContext.GetCurrentCommandBuffer();
    size_t offset = 0;

    size_t instanceBufferSize = instances.size() * sizeof(InstanceData);
    size_t vertexBufferSize = vertices.size() * sizeof(ModelData::Vertex);

    result.InstanceBuffer.Init(instanceBufferSize, BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);
    result.VertexBuffer.Init(vertexBufferSize, BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);
    
    stageBuffer.LoadData((uint8_t*)instances.data(), instanceBufferSize, offset);
    offset += instanceBufferSize;
    stageBuffer.LoadData((uint8_t*)vertices.data(), vertexBufferSize, instanceBufferSize);
    offset += vertexBufferSize;

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
        stageBuffer.LoadData(texture.ByteData, texture.GetByteSize(), offset);
        offset += texture.GetByteSize();
    }

    commandBuffer.Begin();

    offset = 0;
    commandBuffer.CopyBuffer(
        stageBuffer, vk::AccessFlagBits::eHostWrite, offset,
        result.InstanceBuffer, vk::AccessFlags{ }, 0,
        vk::PipelineStageFlagBits::eHost,
        instanceBufferSize
    );
    offset += instanceBufferSize;

    commandBuffer.CopyBuffer(
        stageBuffer, vk::AccessFlagBits::eTransferRead, offset,
        result.VertexBuffer, vk::AccessFlags{ }, 0,
        vk::PipelineStageFlagBits::eTransfer,
        vertexBufferSize
    );
    offset += vertexBufferSize;

    for (size_t i = 0; i < result.Textures.size(); i++)
    {
        commandBuffer.CopyBufferToImage(
            stageBuffer, vk::AccessFlagBits::eTransferRead, offset,
            result.Textures[i], vk::ImageLayout::eUndefined, vk::AccessFlags{ },
            vk::PipelineStageFlagBits::eTransfer,
            textures[i].GetByteSize()
        );
        offset += textures[i].GetByteSize();
    }

    vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor,
        0, // base mip level
        1, // level count
        0, // base layer
        1  // layer count
    };

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

        commandBuffer.GetNativeHandle().pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            { },
            { },
            { },
            imageLayoutToShaderRead
        );
    }

    commandBuffer.End();
    vulkanContext.SubmitCommandsImmediate(commandBuffer);
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

    auto vertexShader = ShaderLoader::LoadFromSource("main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL, Vulkan.GetAPIVersion());
    auto fragmentShader = ShaderLoader::LoadFromSource("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL, Vulkan.GetAPIVersion());
    Sampler sampler(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MinFilter::LINEAR);

    uint32_t globalTextureIndex = 0;

    RenderGraphResources renderGraphResources{
        std::move(vertexShader),
        std::move(fragmentShader),
        std::move(sampler),
        CreateUniform<CameraUniformData>(),
        CreateUniform<ModelUniformData>(),
        CreateUniform<LightUniformData>(),
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
    
    ImGuiVulkanContext::Init(window, renderGraph.GetNodeByName("ImGuiPass"_id).Pass.GetNativeHandle());

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

            renderGraphResources.CameraUniform.Data.Matrix = camera.GetMatrix();
            renderGraphResources.ModelUniform.Data.Matrix = MakeRotationMatrix(modelRotation);
            renderGraphResources.LightUniform.Data.Color = lightColor;
            renderGraphResources.LightUniform.Data.Direction = Normalize(lightDirection);

            renderGraph.Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph.Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.GetCurrentSwapchainImage());

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
