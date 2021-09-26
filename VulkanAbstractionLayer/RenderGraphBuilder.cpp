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

#include "RenderGraphBuilder.h"
#include "CommandBuffer.h"
#include "VulkanContext.h"
#include "GraphicShader.h"
#include "ComputeShader.h"

namespace VulkanAbstractionLayer
{
    vk::VertexInputRate VertexBindingRateToVertexInputRate(VertexBinding::Rate rate)
    {
        switch (rate)
        {
        case VertexBinding::Rate::PER_VERTEX:
            return vk::VertexInputRate::eVertex;
        case VertexBinding::Rate::PER_INSTANCE:
            return vk::VertexInputRate::eInstance;
        default:
            assert(false);
            return vk::VertexInputRate::eVertex;
        }
    }

    vk::ShaderStageFlags PipelineTypeToShaderStages(vk::PipelineBindPoint pipelineType)
    {
        switch (pipelineType)
        {
        case vk::PipelineBindPoint::eGraphics:
            return vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        case vk::PipelineBindPoint::eCompute:
            return vk::ShaderStageFlagBits::eCompute;
        default:
            assert(false);
            return vk::ShaderStageFlags{ };
        }
    }

    vk::AttachmentLoadOp AttachmentStateToLoadOp(AttachmentState state)
    {
        switch (state)
        {
        case AttachmentState::DISCARD_COLOR:
            return vk::AttachmentLoadOp::eDontCare;
        case AttachmentState::DISCARD_DEPTH_SPENCIL:
            return vk::AttachmentLoadOp::eDontCare;
        case AttachmentState::LOAD_COLOR:
            return vk::AttachmentLoadOp::eLoad;
        case AttachmentState::LOAD_DEPTH_SPENCIL:
            return vk::AttachmentLoadOp::eLoad;
        case AttachmentState::CLEAR_COLOR:
            return vk::AttachmentLoadOp::eClear;
        case AttachmentState::CLEAR_DEPTH_SPENCIL:
            return vk::AttachmentLoadOp::eClear;
        default:
            assert(false);
            return vk::AttachmentLoadOp::eDontCare;
        }
    }

    ImageUsage::Bits AttachmentStateToImageUsage(AttachmentState state)
    {
        switch (state)
        {
        case AttachmentState::DISCARD_COLOR:
            return ImageUsage::COLOR_ATTACHMENT;
        case AttachmentState::DISCARD_DEPTH_SPENCIL:
            return ImageUsage::DEPTH_SPENCIL_ATTACHMENT;
        case AttachmentState::LOAD_COLOR:
            return ImageUsage::COLOR_ATTACHMENT;
        case AttachmentState::LOAD_DEPTH_SPENCIL:
            return ImageUsage::DEPTH_SPENCIL_ATTACHMENT;
        case AttachmentState::CLEAR_COLOR:
            return ImageUsage::COLOR_ATTACHMENT;
        case AttachmentState::CLEAR_DEPTH_SPENCIL:
            return ImageUsage::DEPTH_SPENCIL_ATTACHMENT;
        default:
            assert(false);
            return ImageUsage::COLOR_ATTACHMENT;
        }
    }

    vk::PipelineStageFlags BufferUsageToPipelineStage(BufferUsage::Bits layout)
    {
        switch (layout)
        {
        case BufferUsage::UNKNOWN:
            return vk::PipelineStageFlagBits::eTopOfPipe;
        case BufferUsage::TRANSFER_SOURCE:
            return vk::PipelineStageFlagBits::eTransfer;
        case BufferUsage::TRANSFER_DESTINATION:
            return vk::PipelineStageFlagBits::eTransfer;
        case BufferUsage::UNIFORM_TEXEL_BUFFER:
            return vk::PipelineStageFlagBits::eVertexShader; // TODO: from other shader stages?
        case BufferUsage::STORAGE_TEXEL_BUFFER:
            return vk::PipelineStageFlagBits::eComputeShader;
        case BufferUsage::UNIFORM_BUFFER:
            return vk::PipelineStageFlagBits::eVertexShader;
        case BufferUsage::STORAGE_BUFFER:
            return vk::PipelineStageFlagBits::eComputeShader;
        case BufferUsage::INDEX_BUFFER:
            return vk::PipelineStageFlagBits::eVertexInput;
        case BufferUsage::VERTEX_BUFFER:
            return vk::PipelineStageFlagBits::eVertexInput;
        case BufferUsage::INDIRECT_BUFFER:
            return vk::PipelineStageFlagBits::eDrawIndirect;
        case BufferUsage::SHADER_DEVICE_ADDRESS:
            return vk::PipelineStageFlagBits::eFragmentShader; // TODO: what should be here?
        case BufferUsage::TRANSFORM_FEEDBACK_BUFFER:
            return vk::PipelineStageFlagBits::eTransformFeedbackEXT;
        case BufferUsage::TRANSFORM_FEEDBACK_COUNTER_BUFFER:
            return vk::PipelineStageFlagBits::eTransformFeedbackEXT;
        case BufferUsage::CONDITIONAL_RENDERING:
            return vk::PipelineStageFlagBits::eConditionalRenderingEXT;
        case BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READONLY:
            return vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR;
        case BufferUsage::ACCELERATION_STRUCTURE_STORAGE:
            return vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR; // TODO: what should be here?
        case BufferUsage::SHADER_BINDING_TABLE:
            return vk::PipelineStageFlagBits::eFragmentShader; // TODO: what should be here?
        default:
            assert(false);
            return vk::PipelineStageFlags{ };
        }
    }

    vk::AccessFlags BufferUsageToAccessFlags(BufferUsage::Bits layout)
    {
        switch (layout)
        {
        case BufferUsage::UNKNOWN:
            return vk::AccessFlags{ };
        case BufferUsage::TRANSFER_SOURCE:
            return vk::AccessFlagBits::eTransferRead;
        case BufferUsage::TRANSFER_DESTINATION:
            return vk::AccessFlagBits::eTransferWrite;
        case BufferUsage::UNIFORM_TEXEL_BUFFER:
            return vk::AccessFlagBits::eShaderRead;
        case BufferUsage::STORAGE_TEXEL_BUFFER:
            return vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
        case BufferUsage::UNIFORM_BUFFER:
            return vk::AccessFlagBits::eShaderRead;
        case BufferUsage::STORAGE_BUFFER:
            return vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
        case BufferUsage::INDEX_BUFFER:
            return vk::AccessFlagBits::eIndexRead;
        case BufferUsage::VERTEX_BUFFER:
            return vk::AccessFlagBits::eVertexAttributeRead;
        case BufferUsage::INDIRECT_BUFFER:
            return vk::AccessFlagBits::eIndirectCommandRead;
        case BufferUsage::SHADER_DEVICE_ADDRESS:
            return vk::AccessFlagBits::eShaderRead;
        case BufferUsage::TRANSFORM_FEEDBACK_BUFFER:
            return vk::AccessFlagBits::eTransformFeedbackWriteEXT;
        case BufferUsage::TRANSFORM_FEEDBACK_COUNTER_BUFFER:
            return vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT;
        case BufferUsage::CONDITIONAL_RENDERING:
            return vk::AccessFlagBits::eConditionalRenderingReadEXT;
        case BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READONLY:
            return vk::AccessFlagBits::eAccelerationStructureReadKHR;
        case BufferUsage::ACCELERATION_STRUCTURE_STORAGE:
            return vk::AccessFlagBits::eAccelerationStructureReadKHR;
        case BufferUsage::SHADER_BINDING_TABLE:
            return vk::AccessFlagBits::eShaderRead;
        default:
            assert(false);
            return vk::AccessFlags{ };
        }
    }

    bool HasImageWriteDependency(ImageUsage::Bits usage)
    {
        switch (usage)
        {
        case ImageUsage::TRANSFER_DISTINATION:
        case ImageUsage::STORAGE:
        case ImageUsage::COLOR_ATTACHMENT:
        case ImageUsage::DEPTH_SPENCIL_ATTACHMENT:
        case ImageUsage::FRAGMENT_SHADING_RATE_ATTACHMENT:
            return true;
        default:
            return false;
        }
    }

    bool HasBufferWriteDependency(BufferUsage::Bits usage)
    {
        switch (usage)
        {
        case BufferUsage::TRANSFER_DESTINATION:
        case BufferUsage::UNIFORM_TEXEL_BUFFER:
        case BufferUsage::STORAGE_TEXEL_BUFFER:
        case BufferUsage::STORAGE_BUFFER:
        case BufferUsage::TRANSFORM_FEEDBACK_BUFFER:
        case BufferUsage::TRANSFORM_FEEDBACK_COUNTER_BUFFER:
        case BufferUsage::ACCELERATION_STRUCTURE_STORAGE:
            return true;
        default:
            return false;
        }
    }

    vk::ImageMemoryBarrier CreateImageMemoryBarrier(VkImage image, ImageUsage::Bits oldUsage, ImageUsage::Bits newUsage, Format format, uint32_t mipLevelCount, uint32_t layerCount)
    {
        vk::ImageSubresourceRange subresourceRange;
        subresourceRange
            .setAspectMask(ImageFormatToImageAspect(format))
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLayerCount(layerCount)
            .setLevelCount(mipLevelCount);

        vk::ImageMemoryBarrier imageBarrier;
        imageBarrier
            .setImage(image)
            .setOldLayout(ImageUsageToImageLayout(oldUsage))
            .setNewLayout(ImageUsageToImageLayout(newUsage))
            .setSrcAccessMask(ImageUsageToAccessFlags(oldUsage))
            .setDstAccessMask(ImageUsageToAccessFlags(newUsage))
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSubresourceRange(subresourceRange);

        return imageBarrier;
    }

    vk::BufferMemoryBarrier CreateBufferMemoryBarrier(VkBuffer buffer, BufferUsage::Bits oldUsage, BufferUsage::Bits newUsage)
    {
        vk::BufferMemoryBarrier bufferBarrier;
        bufferBarrier
            .setBuffer(buffer)
            .setSrcAccessMask(BufferUsageToAccessFlags(oldUsage))
            .setDstAccessMask(BufferUsageToAccessFlags(newUsage))
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSize(VK_WHOLE_SIZE)
            .setOffset(0);

        return bufferBarrier;
    }

    void EmitPipelineBarrier(CommandBuffer& commandBuffer, const ResolveInfo& resolveInfo, const std::unordered_map<std::string, BufferTransition>& bufferTransitions, const std::unordered_map<std::string, ImageTransition>& imageTransitions)
    {
        vk::PipelineStageFlags pipelineSourceFlags = { };
        vk::PipelineStageFlags pipelineDistanceFlags = { };

        std::vector<vk::BufferMemoryBarrier> bufferBarriers;
        for (const auto& [bufferName, bufferTransition] : bufferTransitions)
        {
            if (!HasBufferWriteDependency(bufferTransition.InitialUsage))
                continue;

            pipelineSourceFlags |= BufferUsageToPipelineStage(bufferTransition.InitialUsage);
            pipelineDistanceFlags |= BufferUsageToPipelineStage(bufferTransition.FinalUsage);

            auto buffers = resolveInfo.GetBuffers().at(bufferName);
            for (const auto& buffer : buffers)
            {
                bufferBarriers.push_back(
                    CreateBufferMemoryBarrier(buffer.get().GetNativeHandle(), bufferTransition.InitialUsage, bufferTransition.FinalUsage)
                );
            }
        }

        std::vector<vk::ImageMemoryBarrier> imageBarriers;
        for (const auto& [imageName, imageTransition] : imageTransitions)
        {
            if (imageTransition.InitialUsage == imageTransition.FinalUsage && !HasImageWriteDependency(imageTransition.InitialUsage))
                continue;

            pipelineSourceFlags |= ImageUsageToPipelineStage(imageTransition.InitialUsage);
            pipelineDistanceFlags |= ImageUsageToPipelineStage(imageTransition.FinalUsage);

            auto images = resolveInfo.GetImages().at(imageName);
            for (const auto& image : images)
            {
                imageBarriers.push_back(
                    CreateImageMemoryBarrier(image.get().GetNativeHandle(), imageTransition.InitialUsage, imageTransition.FinalUsage, image.get().GetFormat(), image.get().GetMipLevelCount(), image.get().GetLayerCount())
                );
            }
        }

        if (bufferBarriers.empty() && imageBarriers.empty())
            return;

        commandBuffer.GetNativeHandle().pipelineBarrier(
            pipelineSourceFlags,
            pipelineDistanceFlags,
            { },
            { },
            bufferBarriers,
            imageBarriers
        );
    }

    RenderGraphBuilder::PipelineBarrierCallback RenderGraphBuilder::CreatePipelineBarrierCallback(const std::string& renderPassName, const Pipeline& pipeline, const ResourceTransitions& resourceTransitions)
    {
        auto& bufferTransitions = resourceTransitions.Buffers.Transitions.at(renderPassName);
        auto& imageTransitions = resourceTransitions.Images.Transitions.at(renderPassName);

        return [bufferTransitions, imageTransitions](CommandBuffer& commandBuffer, const ResolveInfo& resolveInfo)
        {
            EmitPipelineBarrier(commandBuffer, resolveInfo, bufferTransitions, imageTransitions);
        };
    }

    RenderGraphBuilder::CreateCallback RenderGraphBuilder::CreateCreateCallback(const PipelineHashMap& pipelines, const ResourceTransitions& transitions, const AttachmentHashMap& attachments)
    {
        ResolveInfo resolveInfo;
        std::unordered_map<std::string, ImageTransition> attachmentTransitions;
        for (const auto& [renderPassName, pipeline] : pipelines)
        {
            for (const auto& attachment : pipeline.GetOutputAttachments())
            {
                auto& attachmentTransition = transitions.Images.Transitions.at(renderPassName).at(attachment.Name);
                if (transitions.Images.FirstUsages.at(attachment.Name) == renderPassName)
                {
                    attachmentTransitions[attachment.Name] = ImageTransition{
                        ImageUsage::UNKNOWN,
                        attachmentTransition.InitialUsage
                    };
                    resolveInfo.Resolve(attachment.Name, attachments.at(attachment.Name));
                }
            }
        }
        return [resolve = std::move(resolveInfo), transitions = std::move(attachmentTransitions)](CommandBuffer& commandBuffer)
        {
            EmitPipelineBarrier(commandBuffer, resolve, { }, transitions);
        };
    }

    static void DefaultPresentCallback(CommandBuffer&, const Image&, const Image&) { }

    RenderGraphBuilder::PresentCallback RenderGraphBuilder::CreatePresentCallback(const std::string& outputName, const ResourceTransitions& transitions)
    {
        auto outputImageTransition = this->GetOutputImageFinalTransition(this->outputName, transitions);
        return [outputImageTransition](CommandBuffer& commandBuffer, const Image& outputImage, const Image& presentImage)
        {
            commandBuffer.BlitImage(outputImage, outputImageTransition.InitialUsage, presentImage, ImageUsage::UNKNOWN, BlitFilter::LINEAR);
            auto subresourceRange = GetDefaultImageSubresourceRange(outputImage);
            if (outputImageTransition.FinalUsage != ImageUsage::TRANSFER_SOURCE)
            {
                commandBuffer.GetNativeHandle().pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    ImageUsageToPipelineStage(outputImageTransition.FinalUsage),
                    { }, // dependencies
                    { }, // memory barriers
                    { }, // buffer barriers
                    vk::ImageMemoryBarrier{
                        vk::AccessFlagBits::eTransferRead,
                        ImageUsageToAccessFlags(outputImageTransition.FinalUsage),
                        vk::ImageLayout::eTransferSrcOptimal,
                        ImageUsageToImageLayout(outputImageTransition.FinalUsage),
                        VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED,
                        outputImage.GetNativeHandle(),
                        subresourceRange
                    }
                );
            }
        };
    }

    static vk::Pipeline CreateComputePipeline(const Shader& shader, const vk::PipelineLayout& layout)
    {
        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{ },
            ToNative(ShaderType::COMPUTE),
            shader.GetNativeShader(ShaderType::COMPUTE),
            "main"
        };

        vk::ComputePipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo
            .setStage(shaderStageCreateInfo)
            .setLayout(layout)
            .setBasePipelineHandle(vk::Pipeline{ })
            .setBasePipelineIndex(0);

        return GetCurrentVulkanContext().GetDevice().createComputePipeline(vk::PipelineCache{ }, pipelineCreateInfo).value;
    }

    static vk::Pipeline CreateGraphicPipeline(const Shader& shader, const vk::PipelineLayout& layout, ArrayView<const VertexBinding> vertexBindings, const vk::RenderPass& renderPass)
    {
        std::array shaderStageCreateInfos = {
            vk::PipelineShaderStageCreateInfo {
                vk::PipelineShaderStageCreateFlags{ },
                ToNative(ShaderType::VERTEX),
                shader.GetNativeShader(ShaderType::VERTEX),
                "main"
            },
            vk::PipelineShaderStageCreateInfo {
                vk::PipelineShaderStageCreateFlags{ },
                ToNative(ShaderType::FRAGMENT),
                shader.GetNativeShader(ShaderType::FRAGMENT),
                "main"
            }
        };

        auto vertexAttributes = shader.GetInputAttributes();

        std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
        uint32_t vertexBindingId = 0, attributeLocationId = 0, vertexBindingOffset = 0;
        uint32_t attributeLocationOffset = 0;

        for (const auto& attribute : vertexAttributes)
        {
            for (size_t i = 0; i < attribute.ComponentCount; i++)
            {
                vertexAttributeDescriptions.push_back(
                    vk::VertexInputAttributeDescription{
                        attributeLocationId,
                        vertexBindingId,
                        ToNative(attribute.LayoutFormat),
                        vertexBindingOffset,
                    }
                );
                attributeLocationId++;
                vertexBindingOffset += attribute.ByteSize / attribute.ComponentCount;
            }

            if (attributeLocationId == attributeLocationOffset + vertexBindings[vertexBindingId].BindingRange)
            {
                vertexBindingDescriptions.push_back(
                    vk::VertexInputBindingDescription{
                        vertexBindingId,
                        vertexBindingOffset,
                        VertexBindingRateToVertexInputRate(vertexBindings[vertexBindingId].InputRate)
                    });
                vertexBindingId++;
                attributeLocationOffset = attributeLocationId;
                vertexBindingOffset = 0; // vertex binding offset is local to binding
            }
        }
        // move everything else to last vertex buffer
        if (attributeLocationId != attributeLocationOffset)
        {
            vertexBindingDescriptions.push_back(
                vk::VertexInputBindingDescription{
                    vertexBindingId,
                    vertexBindingOffset,
                    VertexBindingRateToVertexInputRate(vertexBindings[vertexBindingId].InputRate)
                }
            );
        }

        vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
        vertexInputStateCreateInfo
            .setVertexBindingDescriptions(vertexBindingDescriptions)
            .setVertexAttributeDescriptions(vertexAttributeDescriptions);

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
        inputAssemblyStateCreateInfo
            .setPrimitiveRestartEnable(false)
            .setTopology(vk::PrimitiveTopology::eTriangleList);

        vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
        viewportStateCreateInfo
            .setViewportCount(1) // defined dynamic
            .setScissorCount(1); // defined dynamic

        vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
        rasterizationStateCreateInfo
            .setPolygonMode(vk::PolygonMode::eFill)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eCounterClockwise)
            .setLineWidth(1.0f);

        vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
        multisampleStateCreateInfo
            .setRasterizationSamples(vk::SampleCountFlagBits::e1)
            .setMinSampleShading(1.0f);

        vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
        colorBlendAttachmentState
            .setBlendEnable(true)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
            .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd)
            .setColorWriteMask(
                vk::ColorComponentFlagBits::eR |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA
            );

        vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
        colorBlendStateCreateInfo
            .setLogicOpEnable(false)
            .setLogicOp(vk::LogicOp::eCopy)
            .setAttachments(colorBlendAttachmentState)
            .setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

        std::array dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };

        vk::PipelineDepthStencilStateCreateInfo depthSpencilStateCreateInfo;
        depthSpencilStateCreateInfo
            .setStencilTestEnable(false)
            .setDepthTestEnable(true)
            .setDepthWriteEnable(true)
            .setDepthBoundsTestEnable(false)
            .setDepthCompareOp(vk::CompareOp::eLess)
            .setMinDepthBounds(0.0f)
            .setMaxDepthBounds(1.0f);

        vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;
        dynamicStateCreateInfo.setDynamicStates(dynamicStates);

        vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo
            .setStages(shaderStageCreateInfos)
            .setPVertexInputState(&vertexInputStateCreateInfo)
            .setPInputAssemblyState(&inputAssemblyStateCreateInfo)
            .setPViewportState(&viewportStateCreateInfo)
            .setPRasterizationState(&rasterizationStateCreateInfo)
            .setPMultisampleState(&multisampleStateCreateInfo)
            .setPDepthStencilState(&depthSpencilStateCreateInfo)
            .setPColorBlendState(&colorBlendStateCreateInfo)
            .setPDynamicState(&dynamicStateCreateInfo)
            .setLayout(layout)
            .setRenderPass(renderPass)
            .setSubpass(0)
            .setBasePipelineHandle(vk::Pipeline{ })
            .setBasePipelineIndex(0);

        return GetCurrentVulkanContext().GetDevice().createGraphicsPipeline(vk::PipelineCache{ }, pipelineCreateInfo).value;
    }

    static vk::PipelineLayout CreatePipelineLayout(const vk::DescriptorSetLayout& descriptorSetLayout, vk::PipelineBindPoint pipelineType)
    {
        vk::PushConstantRange pushConstantRange;
        pushConstantRange
            .setOffset(0)
            .setSize(128)
            .setStageFlags(PipelineTypeToShaderStages(pipelineType));

        vk::PipelineLayoutCreateInfo layoutCreateInfo;
        layoutCreateInfo
            .setSetLayouts(descriptorSetLayout)
            .setPushConstantRanges(pushConstantRange);

        return GetCurrentVulkanContext().GetDevice().createPipelineLayout(layoutCreateInfo);
    }

    PassNative RenderGraphBuilder::BuildRenderPass(const RenderPassReference& renderPassReference, const PipelineHashMap& pipelines, const AttachmentHashMap& attachments, const ResourceTransitions& resourceTransitions)
    {
        PassNative passNative;

        std::vector<vk::AttachmentDescription> attachmentDescriptions;
        std::vector<vk::AttachmentReference> attachmentReferences;
        std::vector<vk::ImageView> attachmentViews;

        uint32_t renderAreaWidth = 0, renderAreaHeight = 0;

        auto& pass = pipelines.at(renderPassReference.Name);
        auto& renderPassAttachments = pass.GetOutputAttachments();
        auto& imageTransitions = resourceTransitions.Images.Transitions.at(renderPassReference.Name);

        // should render pass be created?
        if (!renderPassAttachments.empty())
        {
            vk::AttachmentReference depthStencilAttachmentReference;
            for (size_t attachmentIndex = 0; attachmentIndex < renderPassAttachments.size(); attachmentIndex++)
            {
                const auto& attachment = renderPassAttachments[attachmentIndex];
                const auto& imageReference = attachments.at(attachment.Name);
                const auto& attachmentTransition = imageTransitions.at(attachment.Name);

                if (renderAreaWidth == 0 && renderAreaHeight == 0)
                {
                    renderAreaWidth = std::max(renderAreaWidth, (uint32_t)imageReference.GetWidth());
                    renderAreaHeight = std::max(renderAreaHeight, (uint32_t)imageReference.GetHeight());
                }

                vk::AttachmentDescription attachmentDescription;
                attachmentDescription
                    .setFormat(ToNative(imageReference.GetFormat()))
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(AttachmentStateToLoadOp(attachment.OnLoad))
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(ImageUsageToImageLayout(attachmentTransition.FinalUsage))
                    .setFinalLayout(ImageUsageToImageLayout(attachmentTransition.FinalUsage));

                attachmentDescriptions.push_back(std::move(attachmentDescription));
                if (attachment.Layer == Pipeline::OutputAttachment::ALL_LAYERS)
                    attachmentViews.push_back(imageReference.GetNativeView(ImageView::NATIVE));
                else
                    attachmentViews.push_back(imageReference.GetNativeView(ImageView::NATIVE, attachment.Layer));

                vk::AttachmentReference attachmentReference;
                attachmentReference
                    .setAttachment((uint32_t)attachmentIndex)
                    .setLayout(ImageUsageToImageLayout(attachmentTransition.FinalUsage));

                if (attachmentTransition.FinalUsage == ImageUsage::DEPTH_SPENCIL_ATTACHMENT)
                {
                    depthStencilAttachmentReference = std::move(attachmentReference);
                    passNative.ClearValues.push_back(vk::ClearDepthStencilValue{
                        attachment.DepthSpencilClear.Depth, attachment.DepthSpencilClear.Stencil
                    });
                }
                else
                {
                    attachmentReferences.push_back(std::move(attachmentReference));
                    passNative.ClearValues.push_back(vk::ClearColorValue{
                        std::array{ attachment.ColorClear.R, attachment.ColorClear.G, attachment.ColorClear.B, attachment.ColorClear.A }
                    });
                }
            }

            vk::SubpassDescription subpassDescription;
            subpassDescription
                .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                .setColorAttachments(attachmentReferences)
                .setPDepthStencilAttachment(depthStencilAttachmentReference != vk::AttachmentReference{ } ?
                    std::addressof(depthStencilAttachmentReference) : nullptr
                );

            // TODO
            std::array subpassDependencies = {
                vk::SubpassDependency {
                    VK_SUBPASS_EXTERNAL,
                    0,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    vk::AccessFlagBits::eMemoryRead,
                    vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                    vk::DependencyFlagBits::eByRegion
                },
                vk::SubpassDependency {
                    0,
                    VK_SUBPASS_EXTERNAL,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    vk::PipelineStageFlagBits::eBottomOfPipe,
                    vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                    vk::AccessFlagBits::eMemoryRead,
                    vk::DependencyFlagBits::eByRegion
                },
            };

            vk::RenderPassCreateInfo renderPassCreateInfo;
            renderPassCreateInfo
                .setAttachments(attachmentDescriptions)
                .setSubpasses(subpassDescription)
                .setDependencies(subpassDependencies);

            vk::RenderPassMultiviewCreateInfo renderPassMultiViewCreateInfo;
            if (!renderPassAttachments.empty())
            {
                auto& layeredAttachment = renderPassAttachments.front();
                uint32_t layerCount = attachments.at(layeredAttachment.Name).GetLayerCount();

                if (layerCount > 1 && layeredAttachment.Layer == Pipeline::OutputAttachment::ALL_LAYERS)
                {
                    uint32_t viewMask = (1u << layerCount) - 1; // bits of mask: for example 0b0...011 for 2 views
                    renderPassMultiViewCreateInfo
                        .setSubpassCount(1)
                        .setViewMasks(viewMask)
                        .setCorrelationMasks(viewMask);
                    renderPassCreateInfo.setPNext(&renderPassMultiViewCreateInfo);
                }
            }
            passNative.RenderPassHandle = GetCurrentVulkanContext().GetDevice().createRenderPass(renderPassCreateInfo);

            vk::FramebufferCreateInfo framebufferCreateInfo;
            framebufferCreateInfo
                .setRenderPass(passNative.RenderPassHandle)
                .setAttachments(attachmentViews)
                .setWidth(renderAreaWidth)
                .setHeight(renderAreaHeight)
                .setLayers(1);
            passNative.Framebuffer = GetCurrentVulkanContext().GetDevice().createFramebuffer(framebufferCreateInfo);

            passNative.RenderArea = vk::Rect2D{ vk::Offset2D{ 0u, 0u }, vk::Extent2D{ renderAreaWidth, renderAreaHeight } };
        }

        if (dynamic_cast<GraphicShader*>(pass.Shader.get()) != nullptr)
            passNative.PipelineType = vk::PipelineBindPoint::eGraphics;
        else if (dynamic_cast<ComputeShader*>(pass.Shader.get()) != nullptr)
            passNative.PipelineType = vk::PipelineBindPoint::eCompute;

        if ((bool)pass.Shader)
        {
            auto descriptor = GetCurrentVulkanContext().GetDescriptorCache().GetDescriptor(pass.Shader->GetShaderUniforms());
            passNative.DescriptorSet = descriptor.Set;
            passNative.PipelineLayout = CreatePipelineLayout(descriptor.SetLayout, passNative.PipelineType);

            if(passNative.PipelineType == vk::PipelineBindPoint::eGraphics)
                passNative.Pipeline = CreateGraphicPipeline(*pass.Shader, passNative.PipelineLayout, pass.VertexBindings, passNative.RenderPassHandle);
            if(passNative.PipelineType == vk::PipelineBindPoint::eCompute)
                passNative.Pipeline = CreateComputePipeline(*pass.Shader, passNative.PipelineLayout);
        }

        return passNative;
    }

    RenderGraphBuilder::ResourceTransitions RenderGraphBuilder::ResolveResourceTransitions(const PipelineHashMap& pipelines)
    {
        ResourceTransitions resourceTransitions;
        std::unordered_map<std::string, BufferUsage::Bits> lastBufferUsages;
        std::unordered_map<std::string, ImageUsage::Bits> lastImageUsages;

        for (const auto& renderPassReference : this->renderPassReferences)
        {
            auto& pipeline = pipelines.at(renderPassReference.Name);
            auto& bufferTransitions = resourceTransitions.Buffers.Transitions[renderPassReference.Name];
            auto& imageTransitions = resourceTransitions.Images.Transitions[renderPassReference.Name];

            for (const auto& bufferDependency : pipeline.GetBufferDependencies())
            {
                if (lastBufferUsages.find(bufferDependency.Name) == lastBufferUsages.end())
                {
                    resourceTransitions.Buffers.FirstUsages[bufferDependency.Name] = renderPassReference.Name;
                    lastBufferUsages[bufferDependency.Name] = BufferUsage::UNKNOWN; // resolve at the end
                }
                auto lastBufferUsage = lastBufferUsages.at(bufferDependency.Name);
                bufferTransitions[bufferDependency.Name].InitialUsage = lastBufferUsage;
                bufferTransitions[bufferDependency.Name].FinalUsage = bufferDependency.Usage;
                resourceTransitions.Buffers.TotalUsages[bufferDependency.Name] |= bufferDependency.Usage;
                lastBufferUsages[bufferDependency.Name] = bufferDependency.Usage;
                resourceTransitions.Buffers.LastUsages[bufferDependency.Name] = renderPassReference.Name;
            }

            for (const auto& imageDependency : pipeline.GetImageDependencies())
            {
                if (lastImageUsages.find(imageDependency.Name) == lastImageUsages.end())
                {
                    resourceTransitions.Images.FirstUsages[imageDependency.Name] = renderPassReference.Name;
                    lastImageUsages[imageDependency.Name] = ImageUsage::UNKNOWN; // resolve at the end
                }
                auto lastImageUsage = lastImageUsages.at(imageDependency.Name);
                imageTransitions[imageDependency.Name].InitialUsage = lastImageUsage;
                imageTransitions[imageDependency.Name].FinalUsage = imageDependency.Usage;
                resourceTransitions.Images.TotalUsages[imageDependency.Name] |= imageDependency.Usage;
                lastImageUsages[imageDependency.Name] = imageDependency.Usage;
                resourceTransitions.Images.LastUsages[imageDependency.Name] = renderPassReference.Name;
            }

            for (const auto& attachmentDependency : pipeline.GetOutputAttachments())
            {
                auto attachmentDependencyUsage = AttachmentStateToImageUsage(attachmentDependency.OnLoad);

                if (lastImageUsages.find(attachmentDependency.Name) == lastImageUsages.end())
                {
                    resourceTransitions.Images.FirstUsages[attachmentDependency.Name] = renderPassReference.Name;
                    lastImageUsages[attachmentDependency.Name] = ImageUsage::UNKNOWN; // resolve at the end
                }
                auto lastImageUsage = lastImageUsages.at(attachmentDependency.Name);
                imageTransitions[attachmentDependency.Name].InitialUsage = lastImageUsage;
                imageTransitions[attachmentDependency.Name].FinalUsage = attachmentDependencyUsage;
                resourceTransitions.Images.TotalUsages[attachmentDependency.Name] |= attachmentDependencyUsage;
                lastImageUsages[attachmentDependency.Name] = attachmentDependencyUsage;
                resourceTransitions.Images.LastUsages[attachmentDependency.Name] = renderPassReference.Name;
            }
        }

        // resolve first usages using last usages (as render graph is repeated per-frame)
        for (const auto& [bufferNativeHandle, renderPassName] : resourceTransitions.Buffers.FirstUsages)
        {
            auto& firstBufferTransition = resourceTransitions.Buffers.Transitions[renderPassName][bufferNativeHandle];
            firstBufferTransition.InitialUsage = lastBufferUsages[bufferNativeHandle];
        }
        for (const auto& [imageNativeHandle, renderPassName] : resourceTransitions.Images.FirstUsages)
        {
            auto& firstImageTransition = resourceTransitions.Images.Transitions[renderPassName][imageNativeHandle];
            firstImageTransition.InitialUsage = lastImageUsages[imageNativeHandle];
        }

        return resourceTransitions;
    }

    RenderGraphBuilder::AttachmentHashMap RenderGraphBuilder::AllocateAttachments(const PipelineHashMap& pipelines, const ResourceTransitions& transitions)
    {
        AttachmentHashMap attachments;

        auto [surfaceWidth, surfaceHeight] = GetCurrentVulkanContext().GetSurfaceExtent();

        for (const auto& [renderPassName, pipeline] : pipelines)
        {
            auto& attachmentDeclarations = pipeline.GetAttachmentDeclarations();
            for (const auto& attachment : attachmentDeclarations)
            {
                auto attachmentUsage = transitions.Images.TotalUsages.at(attachment.Name);

                attachments.emplace(attachment.Name, Image(
                    attachment.Width == 0 ? surfaceWidth : attachment.Width,
                    attachment.Height == 0 ? surfaceHeight : attachment.Height,
                    attachment.ImageFormat,
                    attachmentUsage,
                    MemoryUsage::GPU_ONLY,
                    attachment.Options
                ));
            }
        }
        return attachments;
    }

    void RenderGraphBuilder::SetupOutputImage(ResourceTransitions& transitions, const std::string& outputImage)
    {
        transitions.Images.TotalUsages.at(outputImage) |= ImageUsage::TRANSFER_SOURCE;
    }

    RenderGraphBuilder& RenderGraphBuilder::AddRenderPass(const std::string& name, std::unique_ptr<RenderPass> renderPass)
    {
        this->renderPassReferences.push_back({ name, std::move(renderPass) });
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::SetOutputName(const std::string& name)
    {
        this->outputName = name;
        return *this;
    }

    RenderGraphBuilder::PipelineHashMap RenderGraphBuilder::CreatePipelines()
    {
        PipelineHashMap pipelines;
        for (const auto& renderPassReference : this->renderPassReferences)
        {  
            auto& pipeline = pipelines[renderPassReference.Name];
            renderPassReference.Pass->SetupPipeline(pipeline);

            for (const auto& boundBuffer : pipeline.DescriptorBindings.GetBoundBuffers())
                pipeline.AddDependency(boundBuffer.Name, boundBuffer.Usage);
            for (const auto& boundImage : pipeline.DescriptorBindings.GetBoundImages())
                pipeline.AddDependency(boundImage.Name, boundImage.Usage);
        }
        return pipelines;
    }

    ImageTransition RenderGraphBuilder::GetOutputImageFinalTransition(const std::string& outputName, const ResourceTransitions& resourceTransitions)
    {
        const auto& firstRenderPassName = resourceTransitions.Images.FirstUsages.at(outputName);
        const auto& lastRenderPassName = resourceTransitions.Images.LastUsages.at(outputName);

        ImageTransition transition;
        transition.InitialUsage = resourceTransitions.Images.Transitions.at(lastRenderPassName).at(outputName).FinalUsage;
        transition.FinalUsage = resourceTransitions.Images.Transitions.at(firstRenderPassName).at(outputName).InitialUsage;
        return transition;
    }

    std::vector<std::string> RenderGraphBuilder::GetRenderPassAttachmentNames(const std::string& renderPassName, const PipelineHashMap& pipelines)
    {
        std::vector<std::string> attachmentNames;
        auto& outputAttachments = pipelines.at(renderPassName).GetOutputAttachments();

        attachmentNames.reserve(outputAttachments.size());
        for (const auto& outputAttachment : outputAttachments)
            attachmentNames.push_back(outputAttachment.Name);

        return attachmentNames;
    }

    DescriptorBinding RenderGraphBuilder::GetRenderPassDescriptorBinding(const std::string& renderPassName, const PipelineHashMap& pipelines)
    {
        return pipelines.at(renderPassName).DescriptorBindings;
    }

    std::unique_ptr<RenderGraph> RenderGraphBuilder::Build()
    {
        PipelineHashMap pipelines = this->CreatePipelines();
        ResourceTransitions resourceTransitions = this->ResolveResourceTransitions(pipelines);
        if (!this->outputName.empty()) this->SetupOutputImage(resourceTransitions, this->outputName);
        AttachmentHashMap attachments = this->AllocateAttachments(pipelines, resourceTransitions);

        std::vector<RenderGraphNode> nodes;

        for (auto& renderPassReference : this->renderPassReferences)
        {
            auto renderPass = this->BuildRenderPass(renderPassReference, pipelines, attachments, resourceTransitions);

            nodes.push_back(RenderGraphNode{
                renderPassReference.Name,
                renderPass,
                std::move(renderPassReference.Pass),
                this->GetRenderPassAttachmentNames(renderPassReference.Name, pipelines),
                this->CreatePipelineBarrierCallback(renderPassReference.Name, pipelines.at(renderPassReference.Name), resourceTransitions),
                this->GetRenderPassDescriptorBinding(renderPassReference.Name, pipelines),
            });
        }

        auto OnCreate = this->CreateCreateCallback(pipelines, resourceTransitions, attachments);

        auto OnPresent = !this->outputName.empty() ?
            this->CreatePresentCallback(this->outputName, resourceTransitions) :
            DefaultPresentCallback;

        return std::make_unique<RenderGraph>(
            std::move(nodes), 
            std::move(attachments), 
            std::move(this->outputName), 
            std::move(OnPresent),
            std::move(OnCreate)
        );
    }
}