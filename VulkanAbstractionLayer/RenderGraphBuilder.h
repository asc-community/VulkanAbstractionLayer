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

#include <vector>
#include <array>
#include <optional>

#include "StringId.h"
#include "RenderGraph.h"
#include "GraphicShader.h"

namespace VulkanAbstractionLayer
{
    enum class AttachmentLayout
    {
        UNKWNON = 0,
        SHADER_READ,
        COLOR_ATTACHMENT,
        DEPTH_ATTACHMENT,
        TRANSFER_SOURCE,
        TRANSFER_DISTANCE,
    };

    enum class AttachmentInitialState
    {
        CLEAR = 0,
        DISCARD,
        LOAD,
    };

    struct ReadOnlyAttachment
    {
        StringId Name = { };
        AttachmentLayout InitialLayout = AttachmentLayout::UNKWNON;
    };

    struct ReadOnlyColorAttachment : ReadOnlyAttachment { };

    struct ClearColor
    {
        float R = 0.0f, G = 0.0f, B = 0.0f, A = 1.0f;
    };

    struct ClearDepthSpencil
    {
        float Depth = 0.0f;
        uint32_t Spencil = 0;
    };

    struct WriteOnlyColorAttachment
    {
        StringId Name = { };
        ClearColor ClearValue = { };
        AttachmentLayout InitialLayout = AttachmentLayout::UNKWNON;
        AttachmentInitialState InitialState = AttachmentInitialState::DISCARD;
    };

    struct WriteOnlyDepthAttachment
    {
        StringId Name = { };
        ClearDepthSpencil ClearValue = { };
        AttachmentLayout InitialLayout = AttachmentLayout::UNKWNON;
        AttachmentInitialState InitialState = AttachmentInitialState::DISCARD;
    };

    struct GraphicPipeline
    {
        GraphicShader Shader;
        std::vector<VertexBinding> VertexBindings;
    };

    class RenderPassBuilder
    {
        StringId name = { };
        RenderGraphNode::RenderCallback onRenderCallback;
        std::vector<ReadOnlyColorAttachment> inputColorAttachments;
        std::vector<WriteOnlyColorAttachment> outputColorAttachments;
        WriteOnlyDepthAttachment depthAttachment;
        std::optional<GraphicPipeline> graphicPipeline;
    public:
        RenderPassBuilder(StringId name);
        RenderPassBuilder(RenderPassBuilder&&) = default;
        RenderPassBuilder& operator=(RenderPassBuilder&&) = default;

        RenderPassBuilder& AddOnRenderCallback(RenderGraphNode::RenderCallback callback);
        RenderPassBuilder& AddReadOnlyColorAttachment(StringId name);
        RenderPassBuilder& AddWriteOnlyColorAttachment(StringId name);
        RenderPassBuilder& AddWriteOnlyColorAttachment(StringId name, ClearColor clear);
        RenderPassBuilder& AddWriteOnlyColorAttachment(StringId name, AttachmentInitialState state);
        RenderPassBuilder& SetWriteOnlyDepthAttachment(StringId name);
        RenderPassBuilder& SetWriteOnlyDepthAttachment(StringId name, ClearDepthSpencil clear);
        RenderPassBuilder& SetWriteOnlyDepthAttachment(StringId name, AttachmentInitialState state);
        RenderPassBuilder& SetPipeline(GraphicPipeline pipeline);

        friend class RenderGraphBuilder;
    };

    class RenderGraphBuilder
    {
        using AttachmentHashMap = std::unordered_map<StringId, Image>;

        std::vector<RenderPassBuilder> renderPasses;
        StringId outputName = { };

        RenderPass BuildRenderPass(const RenderPassBuilder& renderPassBuilder, const AttachmentHashMap& attachments);
        AttachmentLayout ResolveImageTransitions(StringId outputName);
        AttachmentHashMap AllocateAttachments();
    public:
        RenderGraphBuilder& AddRenderPass(RenderPassBuilder&& renderPass);
        RenderGraphBuilder& AddRenderPass(RenderPassBuilder& renderPass);
        RenderGraphBuilder& SetOutputName(StringId name);
        RenderGraph Build();
    };
}