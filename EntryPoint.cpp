#include <filesystem>
#include <iostream>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/ShaderLoader.h"
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
    vk::DescriptorSetLayout DescriptorSetLayout;
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
            .AddOnRenderCallback(
                [&resources](RenderState state)
                {
                    auto& output = state.GetOutputColorAttachment(0);
                    state.Commands.SetRenderArea(output);
                    state.Commands.BindVertexBuffer(resources.VertexBuffer);
                    state.Commands.Draw(6, 1);
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
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build();
}

vk::DescriptorSetLayout CreateDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 0> layoutBindings;

    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.setBindings(layoutBindings);

    return GetCurrentVulkanContext().GetDevice().createDescriptorSetLayout(createInfo);
}

Buffer CreateVertexBuffer()
{
    Buffer buffer;

    struct Vertex
    {
        Vector4 Position;
        Vector2 TexCoord;
    };

    std::array vertexData = {
       Vertex {
           Vector4 { -0.9f, -0.6f, 0.0f, 1.0f },
           Vector2 { 0.0f, 0.0f },
       },
       Vertex {
           Vector4 { -0.9f, 0.6f, 0.0f, 1.0f },
           Vector2 { 0.0f, 1.0f },
       },
       Vertex {
           Vector4 { 0.9f, -0.6f, 0.0f, 1.0f },
           Vector2 { 1.0f, 0.0f },
       },
       Vertex {
           Vector4 { 0.9f, 0.6f, 0.0f, 1.0f },
           Vector2 { 1.0f, 1.0f },
       },
       Vertex {
           Vector4 { 0.9f, -0.6f, 0.0f, 1.0f },
           Vector2 { 1.0f, 0.0f },
       },
       Vertex {
           Vector4 { -0.9f, 0.6f, 0.0f, 1.0f },
           Vector2 { 0.0f, 1.0f },
       },
    };

    constexpr size_t byteSize = vertexData.size() * sizeof(Vertex);

    auto& vulkanContext = GetCurrentVulkanContext();
    auto& stageBuffer = vulkanContext.GetCurrentStageBuffer();
    auto& commandBuffer = vulkanContext.GetCurrentCommandBuffer();

    buffer.Init(byteSize, BufferUsageType::VERTEX_BUFFER | BufferUsageType::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY);
    stageBuffer.LoadData((uint8_t*)vertexData.data(), byteSize, 0);
    commandBuffer.Begin();
    commandBuffer.CopyBuffer(stageBuffer, vk::AccessFlagBits::eHostWrite, buffer, vk::AccessFlags{ });
    commandBuffer.End();
    vulkanContext.SubmitCommandsImmediate(commandBuffer);
    return buffer;
}

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

    RenderGraphResources renderGraphResources{
        vertexShader.Data,
        fragmentShader.Data,
        vertexShader.VertexAttributes,
        CreateVertexBuffer(),
        CreateDescriptorSetLayout(),
    };
    RenderGraph renderGraph = CreateRenderGraph(renderGraphResources, Vulkan);

    window.OnResize([&Vulkan, &renderGraphResources, &renderGraph](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        renderGraph = CreateRenderGraph(renderGraphResources, Vulkan);
    });
    
    ImGuiVulkanContext::Init(window, renderGraph.GetNodeByName("ImGuiPass"_id).Pass.GetNativeHandle());

    while (!window.ShouldClose())
    {
        window.PollEvents();

        if (Vulkan.IsRenderingEnabled())
        {
            Vulkan.StartFrame();
            ImGuiVulkanContext::StartFrame();
            
            ImGui::ShowDemoWindow();

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