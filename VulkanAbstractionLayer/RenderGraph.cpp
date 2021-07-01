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
    RenderGraph::RenderGraph(std::vector<RenderGraphNode> nodes, std::unordered_map<StringId, Image> images, StringId outputName, PresentCallback onPresent, CreateCallback onCreate, DestroyCallback onDestroy)
        : nodes(std::move(nodes)), images(std::move(images)), outputName(std::move(outputName)), onPresent(std::move(onPresent)), onCreate(std::move(onCreate)), onDestroy(std::move(onDestroy))
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

    void RenderGraph::ExecuteRenderGraphNode(const RenderGraphNode& node, CommandBuffer& commandBuffer)
    {
        RenderPassState state{ *this, commandBuffer, node.ColorAttachments };
            
        node.PassCustom->BeforeRender(state);

        node.PassNative.OnRenderCallback(commandBuffer.GetNativeHandle());

        if ((bool)node.PassNative.RenderPassHandle) // has render pass
        {
            commandBuffer.BeginRenderPass(node.PassNative);
            node.PassCustom->OnRender(state);
            commandBuffer.EndRenderPass();
        }
        else
        {
            node.PassCustom->OnRender(state);
        }

        node.PassCustom->AfterRender(state);
    }

    void RenderGraph::Execute(CommandBuffer& commandBuffer)
    {
        this->InitializeOnFirstFrame(commandBuffer);

        for (const auto& node : this->nodes)
        {
            CommandBuffer command{ commandBuffer };
            this->ExecuteRenderGraphNode(node, command);
        }
    }

    void RenderGraph::Present(CommandBuffer& commandBuffer, const Image& presentImage)
    {
        this->onPresent(commandBuffer, this->images.at(this->outputName), presentImage);
    }

    const RenderGraphNode& RenderGraph::GetNodeByName(StringId name) const
    {
        auto it = std::find_if(this->nodes.begin(), this->nodes.end(), [name](const RenderGraphNode& node) { return node.Name == name; });
        assert(it != this->nodes.end());
        return *it;
    }

    RenderGraph& RenderGraph::operator=(RenderGraph&& other) noexcept
    {
        if (this != std::addressof(other))
        {
            this->~RenderGraph();
            return *std::launder(new(this) RenderGraph(std::move(other)));
        }
        else return *this;
    }

    RenderGraph::~RenderGraph()
    {
        for (const auto& node : this->nodes)
        {
            this->onDestroy(node.PassNative);
        }
        this->nodes.clear();
        this->images.clear();
    }

    const Image& RenderGraph::GetImageByName(StringId name) const
    {
        return this->images.at(name);
    }
}