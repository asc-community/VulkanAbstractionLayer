#include <filesystem>
#include <iostream>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
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
    window.OnResize([&Vulkan](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
    });

    vk::AttachmentDescription attachmentDescription;
    attachmentDescription
        .setFormat(Vulkan.GetSurfaceFormat())
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference colorAttachmentReference;
    colorAttachmentReference
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpassDescription;
    subpassDescription
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(colorAttachmentReference);

    std::array dependencies = {
        vk::SubpassDependency {
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eMemoryRead,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::DependencyFlagBits::eByRegion
        },
        vk::SubpassDependency {
            0,
            VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::DependencyFlagBits::eByRegion
        },
    };

    vk::RenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo
        .setAttachments(attachmentDescription)
        .setSubpasses(subpassDescription)
        .setDependencies(dependencies);

    auto renderPass = Vulkan.GetDevice().createRenderPass(renderPassCreateInfo);

    VulkanAbstractionLayer::ImGuiContext::Init(Vulkan, window, renderPass);

    std::vector<vk::Framebuffer> presentImageFramebuffers(deviceOptions.virtualFrameCount);
    size_t framebufferImageArrayIndex = 0;
    
    while (!window.ShouldClose())
    {
        window.PollEvents();

        Vulkan.StartFrame();
        VulkanAbstractionLayer::ImGuiContext::StartFrame(Vulkan);

        ImGui::ShowDemoWindow();

        std::array attachments = { Vulkan.GetCurrentSwapchainImage().GetNativeView() };
        vk::FramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo
            .setRenderPass(renderPass)
            .setAttachments(attachments)
            .setWidth(Vulkan.GetSurfaceExtent().width)
            .setHeight(Vulkan.GetSurfaceExtent().height)
            .setLayers(1);

        auto& framebuffer = presentImageFramebuffers[framebufferImageArrayIndex];
        if ((bool)framebuffer) Vulkan.GetDevice().destroyFramebuffer(framebuffer);
        framebuffer = Vulkan.GetDevice().createFramebuffer(framebufferCreateInfo);
        framebufferImageArrayIndex = (framebufferImageArrayIndex + 1) % deviceOptions.virtualFrameCount;

        auto& commandBuffer = Vulkan.GetCurrentFrame().CommandBuffer;

        std::array clearValues = { vk::ClearValue{ vk::ClearColorValue{ std::array { 0.8f, 0.6f, 0.0f, 1.0f } } } };
        vk::RenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo
            .setRenderPass(renderPass)
            .setFramebuffer(framebuffer)
            .setClearValues(clearValues)
            .setRenderArea(vk::Rect2D{ vk::Offset2D{ 0u, 0u }, Vulkan.GetSurfaceExtent() });

        commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        VulkanAbstractionLayer::ImGuiContext::RenderFrame(Vulkan);
        commandBuffer.endRenderPass();

        VulkanAbstractionLayer::ImGuiContext::EndFrame(Vulkan);
        Vulkan.EndFrame();
    }

    Vulkan.GetDevice().waitIdle();

    for(auto framebuffer : presentImageFramebuffers)
        if ((bool)framebuffer) Vulkan.GetDevice().destroyFramebuffer(framebuffer);

    Vulkan.GetDevice().destroyRenderPass(renderPass);

    VulkanAbstractionLayer::ImGuiContext::Destroy(Vulkan);

    return 0;
}