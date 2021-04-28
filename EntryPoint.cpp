#include <filesystem>
#include <iostream>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraph.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/CommandBuffer.h"
#include "VulkanAbstractionLayer/ShaderLoader.h"
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
    vk::ShaderModule VertexShader;
    vk::ShaderModule FragmentShader;
    vk::DescriptorSetLayout DescriptorSetLayout;
};

vk::ShaderModule CreateShaderModuleFromSource(const VulkanContext& context, const std::filesystem::path& filename, ShaderType type)
{
    auto bytecode = ShaderLoader::LoadFromSource(filename.string(), type, ShaderLanguage::GLSL, context.GetAPIVersion());
    vk::ShaderModuleCreateInfo createInfo;
    createInfo
        .setPCode(bytecode.data())
        .setCodeSize(bytecode.size() * sizeof(uint32_t));

    return context.GetDevice().createShaderModule(createInfo);
}

vk::ShaderModule CreateShaderModuleFromBinary(const VulkanContext& context, const std::string& filename, ShaderType type)
{
    (void)type;
    auto bytecode = ShaderLoader::LoadFromBinary(filename);
    vk::ShaderModuleCreateInfo createInfo;
    createInfo
        .setPCode(bytecode.data())
        .setCodeSize(bytecode.size() * sizeof(uint32_t));

    return context.GetDevice().createShaderModule(createInfo);
}

RenderGraph CreateRenderGraph(const RenderGraphResources& resources, const VulkanContext& context)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass(RenderPassBuilder{ "TrianglePass"_id }
            .SetPipeline(GraphicPipeline{
                GraphicShader{
                    resources.VertexShader,
                    resources.FragmentShader,
                    resources.DescriptorSetLayout,
                    {
                        // VertexBinding{
                        //     {
                        //         VertexAttributeLayout{
                        //             // position
                        //         },
                        //         VertexAttributeLayout{
                        //             // uv
                        //         }
                        //     },
                        //     vk::VertexInputRate::eVertex
                        // }
                    }
                }
            })
            .AddWriteOnlyColorAttachment("Output"_id, ClearColor{ 0.8f, 0.6f, 0.0f, 1.0f })
            .AddOnRenderCallback(
                [](RenderState state)
                {
                    auto& output = state.GetColorAttachment(0);
                    state.Commands.SetRenderArea(output);
                    state.Commands.Draw(3, 1);
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
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build(context);
}

vk::DescriptorSetLayout CreateDescriptorSetLayout(const VulkanContext& context)
{
    std::array<vk::DescriptorSetLayoutBinding, 0> layoutBindings = {
    };

    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.setBindings(layoutBindings);

    return context.GetDevice().createDescriptorSetLayout(createInfo);
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

    RenderGraphResources renderGraphResources{
        CreateShaderModuleFromSource(Vulkan, "main_vertex.glsl", ShaderType::VERTEX),
        CreateShaderModuleFromSource(Vulkan, "main_fragment.glsl", ShaderType::FRAGMENT),
        CreateDescriptorSetLayout(Vulkan)
    };
    RenderGraph renderGraph = CreateRenderGraph(renderGraphResources, Vulkan);

    window.OnResize([&Vulkan, &renderGraphResources, &renderGraph](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        renderGraph = CreateRenderGraph(renderGraphResources, Vulkan);
    });
    
    ImGuiVulkanContext::Init(Vulkan, window, renderGraph.GetNodeByName("ImGuiPass"_id).Pass.GetNativeHandle());

    while (!window.ShouldClose())
    {
        window.PollEvents();

        if (Vulkan.IsRenderingEnabled())
        {
            Vulkan.StartFrame();
            ImGuiVulkanContext::StartFrame();
            
            ImGui::ShowDemoWindow();

            CommandBuffer commandBuffer{ Vulkan.GetCurrentFrame().CommandBuffer };

            renderGraph.Execute(commandBuffer);
            renderGraph.Present(commandBuffer, Vulkan.GetCurrentSwapchainImage());

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    Vulkan.GetDevice().waitIdle();

    Vulkan.GetDevice().destroyDescriptorSetLayout(renderGraphResources.DescriptorSetLayout);
    Vulkan.GetDevice().destroyShaderModule(renderGraphResources.VertexShader);
    Vulkan.GetDevice().destroyShaderModule(renderGraphResources.FragmentShader);

    ImGuiVulkanContext::Destroy();

    return 0;
}