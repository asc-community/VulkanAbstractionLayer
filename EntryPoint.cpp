#include <filesystem>
#include <iostream>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/ShaderLoader.h"
#include "VulkanAbstractionLayer/ModelLoader.h"
#include "VulkanAbstractionLayer/Buffer.h"
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

struct RenderGraphResources
{
    std::vector<uint32_t> VertexShaderBytecode;
    std::vector<uint32_t> FragmentShaderBytecode;
    std::vector<VertexAttribute> VertexAttributes;
    Buffer VertexBuffer;
    Buffer UniformBuffer;
    vk::DescriptorSet DescriptorSet;
    vk::DescriptorSetLayout DescriptorSetLayout;
    struct UniformBufferData
    {
        Matrix4x4 CameraMatrix;
        Matrix3x4 ModelMatrix;
    } UniformData;
};

auto CreateShaderModuleFromSource(const std::filesystem::path& filename, ShaderType type)
{
    return ShaderLoader::LoadFromSourceWithReflection(filename.string(), type, ShaderLanguage::GLSL, GetCurrentVulkanContext().GetAPIVersion());
}

vk::ShaderModule CreateShaderModuleFromBinary(const std::string& filename, ShaderType type)
{
    (void)type;
    auto bytecode = ShaderLoader::LoadFromBinary(filename);
    vk::ShaderModuleCreateInfo createInfo;
    createInfo
        .setPCode(bytecode.data())
        .setCodeSize(bytecode.size() * sizeof(uint32_t));

    return GetCurrentVulkanContext().GetDevice().createShaderModule(createInfo);
}

RenderGraph CreateRenderGraph(const RenderGraphResources& resources, const VulkanContext& context)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass(RenderPassBuilder{ "TrianglePass"_id }
            .SetPipeline(GraphicPipeline{
                GraphicShader{
                    resources.VertexShaderBytecode,
                    resources.FragmentShaderBytecode,
                    resources.VertexAttributes,
                    resources.DescriptorSetLayout,
                },
                {
                    VertexBinding{
                        VertexBinding::Rate::PER_VERTEX,
                        VertexBinding::BindingRangeAll
                    }
                }
            })
            .AddWriteOnlyColorAttachment("Output"_id, ClearColor{ 0.8f, 0.6f, 0.0f, 1.0f })
            .SetWriteOnlyDepthAttachment("OutputDepth"_id, ClearDepthSpencil{ })
            .AddBeforeRenderCallback([&resources](RenderState state)
                {
                    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
                    stageBuffer.LoadData((uint8_t*)&resources.UniformData, sizeof(resources.UniformData), 0);

                    state.Commands.CopyBuffer(
                        stageBuffer, vk::AccessFlagBits::eHostWrite,
                        resources.UniformBuffer, vk::AccessFlagBits::eUniformRead,
                        vk::PipelineStageFlagBits::eHost | vk::PipelineStageFlagBits::eFragmentShader
                    );

                    // TODO: hide explicit barriers
                    vk::BufferMemoryBarrier toUniformBuffer;
                    toUniformBuffer
                        .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                        .setDstAccessMask(vk::AccessFlagBits::eUniformRead)
                        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                        .setBuffer(resources.UniformBuffer.GetNativeHandle())
                        .setSize(resources.UniformBuffer.GetByteSize())
                        .setOffset(0);

                    state.Commands.GetNativeHandle().pipelineBarrier(
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader,
                        { }, // dependency flags
                        { }, // memory barriers
                        toUniformBuffer,
                        { } // image barriers
                    );
                })
            .AddOnRenderCallback(
                [&resources](RenderState state)
                {
                    auto& output = state.GetOutputColorAttachment(0);
                    state.Commands.SetRenderArea(output);

                    state.Commands.GetNativeHandle().bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics, 
                        state.PipelineLayout,
                        0, 
                        resources.DescriptorSet,
                        { }
                    );

                    state.Commands.BindVertexBuffer(resources.VertexBuffer);
                    state.Commands.Draw((uint32_t)resources.VertexBuffer.GetByteSize() / sizeof(ModelLoader::Vertex), 1);
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

auto CreateDescriptorSet()
{
    auto& vulkan = GetCurrentVulkanContext();

    std::array layoutBindings = {
        vk::DescriptorSetLayoutBinding {
            1,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex
        }
    };

    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.setBindings(layoutBindings);

    auto descriptorSetLayout = vulkan.GetDevice().createDescriptorSetLayout(createInfo);

    vk::DescriptorSetAllocateInfo allocationInfo;
    allocationInfo
        .setDescriptorPool(vulkan.GetDescriptorPool())
        .setSetLayouts(descriptorSetLayout);


    auto descriptorSet = vulkan.GetDevice().allocateDescriptorSets(allocationInfo).front();
    
    return std::pair{ descriptorSet, descriptorSetLayout };
}

void WriteDescriptorSet(const vk::DescriptorSet& descriptorSet, const Buffer& uniformBuffer)
{
    vk::DescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo
        .setBuffer(uniformBuffer.GetNativeHandle())
        .setOffset(0)
        .setRange(uniformBuffer.GetByteSize());

    vk::WriteDescriptorSet descriptorBufferWrite;
    descriptorBufferWrite
        .setDstSet(descriptorSet)
        .setDstBinding(1)
        .setDstArrayElement(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setBufferInfo(descriptorBufferInfo);

    GetCurrentVulkanContext().GetDevice().updateDescriptorSets(descriptorBufferWrite, { });
}

Buffer CreateUniformBuffer(size_t byteSize)
{
    return Buffer(byteSize, BufferUsageType::UNIFORM_BUFFER | BufferUsageType::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);
}

Buffer CreateVertexBuffer()
{
    Buffer buffer;

    auto model = ModelLoader::LoadFromObj("models/dragon.obj");
    auto& vertexData = model.Shapes[0].Vertices;
    size_t byteSize = vertexData.size() * sizeof(ModelLoader::Vertex);

    std::cout << "loaded model with " << vertexData.size() << " vertices" << std::endl;

    auto& vulkanContext = GetCurrentVulkanContext();
    auto& stageBuffer = vulkanContext.GetCurrentStageBuffer();
    auto& commandBuffer = vulkanContext.GetCurrentCommandBuffer();

    buffer.Init(byteSize, BufferUsageType::VERTEX_BUFFER | BufferUsageType::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);
    stageBuffer.LoadData((uint8_t*)vertexData.data(), byteSize, 0);
    commandBuffer.Begin();
    commandBuffer.CopyBuffer(
        stageBuffer, vk::AccessFlagBits::eHostWrite, 
        buffer, vk::AccessFlags{ },
        vk::PipelineStageFlagBits::eHost
    );
    commandBuffer.End();
    vulkanContext.SubmitCommandsImmediate(commandBuffer);
    return buffer;
}

struct Camera
{
    Vector3 Position{ 10.0f, 12.0f, -27.0f };
    Vector2 Rotation{ 5.74f, 0.0f };
    float Fov = 65.0f;
    float MovementSpeed = 0.5f;
    float RotationMovementSpeed = 0.01f;
    float AspectRatio = 16.0f / 9.0f;
    float ZNear = 0.001f;
    float ZFar = 1000.0f;

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

    auto vertexShader = CreateShaderModuleFromSource("main_vertex.glsl", ShaderType::VERTEX);
    auto fragmentShader = CreateShaderModuleFromSource("main_fragment.glsl", ShaderType::FRAGMENT);

    auto [descriptorSet, descriptorSetLayout] = CreateDescriptorSet();

    RenderGraphResources renderGraphResources{
        vertexShader.Data,
        fragmentShader.Data,
        vertexShader.VertexAttributes,
        CreateVertexBuffer(),
        CreateUniformBuffer(sizeof(RenderGraphResources::UniformData)),
        descriptorSet,
        descriptorSetLayout,
    };

    WriteDescriptorSet(renderGraphResources.DescriptorSet, renderGraphResources.UniformBuffer);

    RenderGraph renderGraph = CreateRenderGraph(renderGraphResources, Vulkan);

    Camera camera;

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
            
            static Vector3 modelRotation{ -1.45f, 5.0f, 0.0f };

            auto mouseMovement = ImGui::GetMouseDragDelta(MouseButton::RIGHT, 0.0f);
            ImGui::ResetMouseDragDelta(MouseButton::RIGHT);
            camera.Rotate({ -mouseMovement.x, -mouseMovement.y });

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
            camera.Move(movementDirection);

            ImGui::Begin("Camera");
            ImGui::DragFloat("movement speed", &camera.MovementSpeed, 0.1f);
            ImGui::DragFloat("rotation movement speed", &camera.RotationMovementSpeed, 0.01f);
            ImGui::DragFloat3("position", &camera.Position[0]);
            ImGui::DragFloat2("rotation", &camera.Rotation[0], 0.01f);
            ImGui::DragFloat("fov", &camera.Fov);
            ImGui::End();

            ImGui::Begin("Model");
            ImGui::DragFloat3("rotation", &modelRotation[0], 0.01f);
            ImGui::End();

            renderGraphResources.UniformData.CameraMatrix = camera.GetMatrix();
            renderGraphResources.UniformData.ModelMatrix = MakeRotationMatrix(modelRotation);

            renderGraph.Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph.Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.GetCurrentSwapchainImage());

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    Vulkan.GetDevice().waitIdle();

    Vulkan.GetDevice().destroyDescriptorSetLayout(renderGraphResources.DescriptorSetLayout);

    ImGuiVulkanContext::Destroy();

    return 0;
}