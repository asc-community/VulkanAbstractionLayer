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
#include <string>
#include <array>
#include <optional>
#include <unordered_map>

#include "RenderGraph.h"
#include "Shader.h"
#include "DescriptorBinding.h"

namespace VulkanAbstractionLayer
{
    struct ImageTransition
    {
        ImageUsage::Bits InitialUsage;
        ImageUsage::Bits FinalUsage;
    };

    struct BufferTransition
    {
        BufferUsage::Bits InitialUsage;
        BufferUsage::Bits FinalUsage;
    };

    class RenderGraphBuilder
    {
        struct RenderPassReference
        {
            std::string Name;
            std::unique_ptr<RenderPass> Pass;
        };

        using RenderPassName = std::string;

        template<typename ResourceType, typename TransitionType>
        struct ResourceTypeTransitions
        {
            using TransitionHashMap = std::unordered_map<ResourceType, TransitionType>;
            using PerRenderPassTransitionHashMap = std::unordered_map<RenderPassName, TransitionHashMap>;
            using UsageMask = uint32_t;

            PerRenderPassTransitionHashMap Transitions;
            std::unordered_map<ResourceType, UsageMask> TotalUsages;
            std::unordered_map<ResourceType, RenderPassName> FirstUsages;
            std::unordered_map<ResourceType, RenderPassName> LastUsages;
        };

        struct ResourceTransitions
        {
            ResourceTypeTransitions<std::string, BufferTransition> Buffers;
            ResourceTypeTransitions<std::string, ImageTransition> Images;
        };

        using AttachmentHashMap = std::unordered_map<std::string, Image>;
        using PipelineHashMap = std::unordered_map<RenderPassName, Pipeline>;
        using PipelineBarrierCallback = std::function<void(CommandBuffer&, const ResolveInfo&)>;
        using PresentCallback = std::function<void(CommandBuffer&, const Image&, const Image&)>;
        using CreateCallback = std::function<void(CommandBuffer&)>;

        std::vector<RenderPassReference> renderPassReferences;
        std::string outputName;
        
        PassNative BuildRenderPass(const RenderPassReference& renderPassReference, const PipelineHashMap& pipelines, const AttachmentHashMap& attachments, const ResourceTransitions& resourceTransitions);
        PipelineBarrierCallback CreatePipelineBarrierCallback(const std::string& renderPassName, const Pipeline& pipeline, const ResourceTransitions& resourceTransitions);
        PresentCallback CreatePresentCallback(const std::string& outputName, const ResourceTransitions& transitions);
        CreateCallback CreateCreateCallback(const PipelineHashMap& pipelines, const ResourceTransitions& transitions, const AttachmentHashMap& attachments);
        ResourceTransitions ResolveResourceTransitions(const PipelineHashMap& pipelines);
        AttachmentHashMap AllocateAttachments(const PipelineHashMap& pipelines, const ResourceTransitions& transitions);
        void SetupOutputImage(ResourceTransitions& transitions, const std::string& outputImage);
        PipelineHashMap CreatePipelines();
        ImageTransition GetOutputImageFinalTransition(const std::string& outputName, const ResourceTransitions& resourceTransitions);
        std::vector<std::string> GetRenderPassAttachmentNames(const std::string& renderPassName, const PipelineHashMap& pipelines);
        DescriptorBinding GetRenderPassDescriptorBinding(const std::string& renderPassName, const PipelineHashMap& pipelines);
    public:
        RenderGraphBuilder& AddRenderPass(const std::string& name, std::unique_ptr<RenderPass> renderPass);
        RenderGraphBuilder& SetOutputName(const std::string& name);
        std::unique_ptr<RenderGraph> Build();
    };
}