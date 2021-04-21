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
#include "StringId.h"

#include <vector>
#include <functional>

namespace VulkanAbstractionLayer
{
    class CommandBuffer;
    class Image;
    class VulkanContext;
    class RenderPass;
    class RenderGraph;

    struct RenderState
    {
        RenderGraph& Graph;
        CommandBuffer& Commands;
        const std::vector<StringId>& ColorAttachments;

        const Image& GetColorAttachment(size_t index) const;
    };

    struct RenderGraphNode
    {
        using RenderCallback = std::function<void(RenderState)>;

        StringId Name;
        RenderPass Pass;
        RenderCallback OnRender;
        std::vector<StringId> ColorAttachments;
    };

    class RenderGraph
    {
        using PresentCallback = std::function<void(CommandBuffer&, const Image&, const Image&)>;
        using DestroyCallback = std::function<void(const RenderPass&)>;

        std::vector<RenderGraphNode> nodes;
        std::unordered_map<StringId, Image> images;
        StringId outputName;
        PresentCallback onPresent;
        DestroyCallback onDestroy;

    public:
        RenderGraph(std::vector<RenderGraphNode> nodes, std::unordered_map<StringId, Image> images, StringId outputName, PresentCallback onPresent, DestroyCallback onDestroy);
        ~RenderGraph();
        RenderGraph(RenderGraph&&) = default;
        RenderGraph& operator=(RenderGraph&& other) noexcept;

        void ExecuteRenderGraphNode(const RenderGraphNode& node, CommandBuffer& commandBuffer);
        void Execute(CommandBuffer& commandBuffer);
        void Present(CommandBuffer& commandBuffer, const Image& presentImage);
        const RenderGraphNode& GetNodeByName(StringId name) const;
        const Image& GetImageByName(StringId name) const;
    };
}