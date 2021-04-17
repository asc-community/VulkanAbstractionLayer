#include <filesystem>
#include <iostream>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraph.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/CommandBuffer.h"
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

RenderGraph CreateRenderGraph(const Image& outputImage, const VulkanContext& context)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddImageReference("Output"_id, &outputImage)
        .AddRenderPass(RenderPassBuilder{ "ImGuiPass"_id }
            .AddWriteOnlyColorAttachment("Output"_id, ClearColor{ 0.8f, 0.6f, 0.0f, 1.0f })
            .AddOnRenderCallback(
                [](CommandBuffer& commandBuffer)
                {
                    ImGuiVulkanContext::RenderFrame(commandBuffer.GetNativeHandle());
                }
            )
        )
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build(context);
}

Image CreateOutputImage(const VulkanContext& context)
{
    Image result{ context.GetAllocator() };
    result.Init(
        context.GetSurfaceExtent().width,
        context.GetSurfaceExtent().height,
        context.GetSurfaceFormat(),
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        MemoryUsage::GPU_ONLY
    );
    return result;
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

    ContextInitializeOptions deviceOptions;
    deviceOptions.PreferredDeviceType = DeviceType::DISCRETE_GPU;
    deviceOptions.ErrorCallback = VulkanErrorCallback;
    deviceOptions.InfoCallback = VulkanInfoCallback;

    Vulkan.InitializeContext(window.CreateWindowSurface(Vulkan), deviceOptions);

    Image outputImage = CreateOutputImage(Vulkan);
    RenderGraph renderGraph = CreateRenderGraph(outputImage, Vulkan);

    window.OnResize([&Vulkan, &outputImage, &renderGraph](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        outputImage = CreateOutputImage(Vulkan);
        renderGraph = CreateRenderGraph(outputImage, Vulkan);
    });
    
    ImGuiVulkanContext::Init(Vulkan, window, renderGraph.GetNodeByName("ImGuiPass"_id).Pass.GetNativeHandle());

    while (!window.ShouldClose())
    {
        window.PollEvents();

        Vulkan.StartFrame();
        ImGuiVulkanContext::StartFrame();

        ImGui::ShowDemoWindow();

        CommandBuffer commandBuffer{ Vulkan.GetCurrentFrame().CommandBuffer };

        renderGraph.Execute(commandBuffer);
        renderGraph.Present(commandBuffer, Vulkan.GetCurrentSwapchainImage());

        ImGuiVulkanContext::EndFrame();
        Vulkan.EndFrame();
    }

    Vulkan.GetDevice().waitIdle();

    ImGuiVulkanContext::Destroy();

    return 0;
}