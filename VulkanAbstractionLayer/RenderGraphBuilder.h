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
#include <unordered_map>

#include "StringId.h"
#include "RenderGraph.h"
#include "GraphicShader.h"
#include "DescriptorBinding.h"

namespace VulkanAbstractionLayer
{
    enum class AttachmentInitialState
    {
        CLEAR = 0,
        DISCARD,
        LOAD,
    };

    struct ClearColor
    {
        float R = 0.0f, G = 0.0f, B = 0.0f, A = 1.0f;
    };

    struct ClearDepthSpencil
    {
        float Depth = 1.0f;
        uint32_t Spencil = 0;
    };

    struct Attachment
    {
        StringId Name = { };
        ImageUsage::Bits InitialLayout = ImageUsage::UNKNOWN;
    };

    struct WriteOnlyAttachment : Attachment
    {
        AttachmentInitialState InitialState = AttachmentInitialState::DISCARD;
    };

    struct ReadOnlyAttachment : Attachment { };

    struct ReadOnlyColorAttachment : ReadOnlyAttachment { };

    struct WriteOnlyColorAttachment : WriteOnlyAttachment
    {
        ClearColor ClearValue;
    };

    struct WriteOnlyDepthAttachment : WriteOnlyAttachment
    {
        ClearDepthSpencil ClearValue;
    };

    struct GraphicPipeline
    {
        GraphicShader Shader;
        std::vector<VertexBinding> VertexBindings;
        DescriptorBinding DescriptorBindings;
    };

    class RenderPassBuilder
    {
        StringId name = { };
        RenderGraphNode::RenderCallback beforeRenderCallback;
        RenderGraphNode::RenderCallback onRenderCallback;
        RenderGraphNode::RenderCallback afterRenderCallback;
        std::vector<ReadOnlyColorAttachment> inputColorAttachments;
        std::vector<WriteOnlyColorAttachment> outputColorAttachments;
        WriteOnlyDepthAttachment depthAttachment;
        std::optional<GraphicPipeline> graphicPipeline;
    public:
        RenderPassBuilder(StringId name);
        RenderPassBuilder(RenderPassBuilder&&) = default;
        RenderPassBuilder& operator=(RenderPassBuilder&&) = default;

        RenderPassBuilder& AddBeforeRenderCallback(RenderGraphNode::RenderCallback callback);
        RenderPassBuilder& AddOnRenderCallback(RenderGraphNode::RenderCallback callback);
        RenderPassBuilder& AddAfterRenderCallback(RenderGraphNode::RenderCallback callback);
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
        struct AttachmentCreateOptions
        {
            Format LayoutFormat;
            uint32_t Width;
            uint32_t Height;
        };

        using AttachmentHashMap = std::unordered_map<StringId, Image>;
        using AttachmentCreateOptionsHashMap = std::unordered_map<StringId, AttachmentCreateOptions>;

        AttachmentCreateOptionsHashMap attachmentsCreateOptions;
        std::vector<RenderPassBuilder> renderPasses;
        StringId outputName = { };

        RenderPass BuildRenderPass(const RenderPassBuilder& renderPassBuilder, const AttachmentHashMap& attachments);
        ImageUsage::Bits ResolveImageTransitions(StringId outputName);
        void PreWarmDescriptorSets();
        AttachmentHashMap AllocateAttachments();
    public:
        RenderGraphBuilder& AddRenderPass(RenderPassBuilder&& renderPass);
        RenderGraphBuilder& AddRenderPass(RenderPassBuilder& renderPass);
        RenderGraphBuilder& SetOutputName(StringId name);
        RenderGraphBuilder& AddAttachment(StringId name, Format format);
        RenderGraphBuilder& AddAttachment(StringId name, Format format, uint32_t width, uint32_t height);
        RenderGraph Build();
    };
}