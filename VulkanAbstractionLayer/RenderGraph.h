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
#include "StringId.h"

#include <vector>
#include <functional>
#include <unordered_map>

namespace VulkanAbstractionLayer
{
    struct RenderGraphNode
    {
        StringId Name;
        RenderPassNative PassNative;
        std::unique_ptr<RenderPass> PassCustom;
        std::function<void(CommandBuffer&)> PipelineBarrierCallback;
    };

    class RenderGraph
    {
        using PresentCallback = std::function<void(CommandBuffer&, const Image&, const Image&)>;
        using CreateCallback = std::function<void(CommandBuffer&)>;
        using DestroyCallback = std::function<void(CommandBuffer&)>;

        std::vector<RenderGraphNode> nodes;
        std::unordered_map<StringId, Image> attachments;
        StringId outputName;
        PresentCallback onPresent;
        CreateCallback onCreate;

        void InitializeOnFirstFrame(CommandBuffer& commandBuffer);
    public:
        RenderGraph(std::vector<RenderGraphNode> nodes, std::unordered_map<StringId, Image> attachments, StringId outputName, PresentCallback onPresent, CreateCallback onCreate);
        ~RenderGraph();
        RenderGraph(RenderGraph&&) = default;
        RenderGraph& operator=(RenderGraph&& other) = delete;

        void ExecuteRenderGraphNode(const RenderGraphNode& node, CommandBuffer& commandBuffer);
        void Execute(CommandBuffer& commandBuffer);
        void Present(CommandBuffer& commandBuffer, const Image& presentImage);
        const RenderGraphNode& GetNodeByName(StringId name) const;
        RenderGraphNode& GetNodeByName(StringId name);
        const Image& GetAttachmentByName(StringId name) const;

        template<typename T>
        T& GetRenderPassByName(StringId name)
        {
            auto& node = this->GetNodeByName(name);
            assert(dynamic_cast<T*>(node.PassCustom.get()) != nullptr);
            return *static_cast<T*>(node.PassCustom.get());
        }

        template<typename T>
        const T& GetRenderPassByName(StringId name) const
        {
            const auto& node = this->GetNodeByName(name);
            assert(dynamic_cast<const T*>(node.PassCustom.get()) != nullptr);
            return *static_cast<const T*>(node.PassCustom.get());
        }
    };
}