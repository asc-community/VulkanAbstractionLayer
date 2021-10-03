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

#include "RenderGraph.h"
#include "VulkanContext.h"
#include "CommandBuffer.h"

namespace VulkanAbstractionLayer
{
    RenderGraph::RenderGraph(std::vector<RenderGraphNode> nodes, std::unordered_map<std::string, Image> attachments, const std::string& outputName, PresentCallback onPresent, CreateCallback onCreate)
        : nodes(std::move(nodes)), attachments(std::move(attachments)), outputName(std::move(outputName)), onPresent(std::move(onPresent)), onCreate(std::move(onCreate))
    {

    }

    void RenderGraph::InitializeOnFirstFrame(CommandBuffer& commandBuffer)
    {
        if ((bool)this->onCreate)
        {
            this->onCreate(commandBuffer);
            this->onCreate = { }; // destroy resources, avoid calling onCreate again
        }
    }

    void RenderGraph::ExecuteRenderGraphNode(RenderGraphNode& node, CommandBuffer& commandBuffer, ResolveInfo& resolve)
    {
        RenderPassState state{ *this, commandBuffer, node.PassNative };

        node.PassCustom->ResolveResources(resolve);
        node.Descriptors.Resolve(resolve);
        node.Descriptors.Write(node.PassNative.DescriptorSet);

        node.PassCustom->BeforeRender(state);
        node.PipelineBarrierCallback(commandBuffer, resolve);

        commandBuffer.BeginPass(node.PassNative);
        node.PassCustom->OnRender(state);
        commandBuffer.EndPass(node.PassNative);

        node.PassCustom->AfterRender(state);
    }

    void RenderGraph::Execute(CommandBuffer& commandBuffer)
    {
        this->InitializeOnFirstFrame(commandBuffer);

        ResolveInfo resolve;
        for (const auto& [attachmentName, attachment] : this->attachments)
        {
            resolve.Resolve(attachmentName, attachment);
        }

        for (auto& node : this->nodes)
        {
            this->ExecuteRenderGraphNode(node, commandBuffer, resolve);
        }
    }

    void RenderGraph::Present(CommandBuffer& commandBuffer, const Image& presentImage)
    {
        this->onPresent(commandBuffer, this->attachments.at(this->outputName), presentImage);
    }

    const RenderGraphNode& RenderGraph::GetNodeByName(const std::string& name) const
    {
        auto it = std::find_if(this->nodes.begin(), this->nodes.end(), [&name](const RenderGraphNode& node) { return node.Name == name; });
        assert(it != this->nodes.end());
        return *it;
    }

    RenderGraphNode& RenderGraph::GetNodeByName(const std::string& name)
    {
        auto it = std::find_if(this->nodes.begin(), this->nodes.end(), [&name](const RenderGraphNode& node) { return node.Name == name; });
        assert(it != this->nodes.end());
        return *it;
    }

    RenderGraph::~RenderGraph()
    {
        auto& vulkan = GetCurrentVulkanContext();
        auto& device = vulkan.GetDevice();
        device.waitIdle();

        for (const auto& node : this->nodes)
        {
            auto& pass = node.PassNative;
            if ((bool)pass.Pipeline)         device.destroyPipeline(pass.Pipeline);
            if ((bool)pass.PipelineLayout)   device.destroyPipelineLayout(pass.PipelineLayout);
            if ((bool)pass.Framebuffer)      device.destroyFramebuffer(pass.Framebuffer);
            if ((bool)pass.RenderPassHandle) device.destroyRenderPass(pass.RenderPassHandle);
        }
        this->nodes.clear();
        this->attachments.clear();
    }

    const Image& RenderGraph::GetAttachmentByName(const std::string& name) const
    {
        return this->attachments.at(name);
    }
}