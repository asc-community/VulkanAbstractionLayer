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

#pragma once

#include "RenderPass.h"
#include "Image.h"
#include "CommandBuffer.h"

#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace VulkanAbstractionLayer
{
    struct RenderGraphNode
    {
        std::string Name;
        PassNative PassNative;
        std::unique_ptr<RenderPass> PassCustom;
        std::vector<std::string> UsedAttachments;
        std::function<void(CommandBuffer&, const ResolveInfo&)> PipelineBarrierCallback;
        DescriptorBinding Descriptors;
    };

    class RenderGraph
    {
        using PresentCallback = std::function<void(CommandBuffer&, const Image&, const Image&)>;
        using CreateCallback = std::function<void(CommandBuffer&)>;

        std::vector<RenderGraphNode> nodes;
        std::unordered_map<std::string, Image> attachments;
        std::string outputName;
        PresentCallback onPresent;
        CreateCallback onCreate;

        void InitializeOnFirstFrame(CommandBuffer& commandBuffer);
    public:
        RenderGraph(std::vector<RenderGraphNode> nodes, std::unordered_map<std::string, Image> attachments, const std::string& outputName, PresentCallback onPresent, CreateCallback onCreate);
        ~RenderGraph();
        RenderGraph(RenderGraph&&) = default;
        RenderGraph& operator=(RenderGraph&& other) = delete;

        void ExecuteRenderGraphNode(RenderGraphNode& node, CommandBuffer& commandBuffer, ResolveInfo& resolve);
        void Execute(CommandBuffer& commandBuffer);
        void Present(CommandBuffer& commandBuffer, const Image& presentImage);
        const RenderGraphNode& GetNodeByName(const std::string& name) const;
        RenderGraphNode& GetNodeByName(const std::string& name);
        const Image& GetAttachmentByName(const std::string& name) const;

        template<typename T>
        T& GetRenderPassByName(const std::string& name)
        {
            auto& node = this->GetNodeByName(name);
            assert(dynamic_cast<T*>(node.PassCustom.get()) != nullptr);
            return *static_cast<T*>(node.PassCustom.get());
        }

        template<typename T>
        const T& GetRenderPassByName(const std::string& name) const
        {
            const auto& node = this->GetNodeByName(name);
            assert(dynamic_cast<const T*>(node.PassCustom.get()) != nullptr);
            return *static_cast<const T*>(node.PassCustom.get());
        }
    };
}