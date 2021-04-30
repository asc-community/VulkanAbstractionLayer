// Copyright(c) 2021, #Momo
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and /or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "ImGuiContext.h"
#include "VulkanContext.h"
#include "Window.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"

namespace VulkanAbstractionLayer
{
    void ImGuiVulkanContext::Init(const VulkanContext& context, const Window& window, const vk::RenderPass& renderPass)
    {
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(window.GetNativeHandle(), true);
        ImGui_ImplVulkan_InitInfo init_info = { };
        init_info.Instance = context.GetInstance();
        init_info.PhysicalDevice = context.GetPhysicalDevice();
        init_info.Device = context.GetDevice();
        init_info.QueueFamily = context.GetQueueFamilyIndex();
        init_info.Queue = context.GetGraphicsQueue();
        init_info.PipelineCache = vk::PipelineCache{ };
        init_info.DescriptorPool = context.GetDescriptorPool();
        init_info.Allocator = nullptr;
        init_info.MinImageCount = context.GetPresentImageCount();
        init_info.ImageCount = context.GetPresentImageCount();
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info, renderPass);

        auto commandBuffer = context.GetCurrentCommandBuffer().GetNativeHandle();

        commandBuffer.begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        commandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(commandBuffer);
        context.GetGraphicsQueue().submit(submitInfo);

        context.GetDevice().waitIdle();
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void ImGuiVulkanContext::Destroy()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    void ImGuiVulkanContext::StartFrame()
    {
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiVulkanContext::RenderFrame(const vk::CommandBuffer& commandBuffer)
    {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    }

    void ImGuiVulkanContext::EndFrame()
    {
        ImGui::EndFrame();
    }
}