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

#include "DescriptorCache.h"
#include "VulkanContext.h"

namespace VulkanAbstractionLayer
{
	void DescriptorCache::Init()
	{
        auto& vulkan = GetCurrentVulkanContext();
        // TODO: rework
        std::array descriptorPoolSizes = {
            vk::DescriptorPoolSize { vk::DescriptorType::eSampler,              1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, 1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eSampledImage,         1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageImage,         1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformTexelBuffer,   1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageTexelBuffer,   1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer,        1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer,        1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBufferDynamic, 1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageBufferDynamic, 1024 },
            vk::DescriptorPoolSize { vk::DescriptorType::eInputAttachment,      1024 },
        };
        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
            .setPoolSizes(descriptorPoolSizes)
            .setMaxSets(2048 * (uint32_t)descriptorPoolSizes.size());

        this->descriptorPool = vulkan.GetDevice().createDescriptorPool(descriptorPoolCreateInfo);
	}

    void DescriptorCache::Destroy()
    {
        auto& vulkan = GetCurrentVulkanContext();
        if ((bool)this->descriptorPool) vulkan.GetDevice().destroyDescriptorPool(this->descriptorPool);

        for (const auto& descriptor : this->cache)
        {
            this->DestroyDescriptorSetLayout(descriptor.SetLayout);
            // descriptor sets are already freed when pool is destroyed
        }
        this->cache.clear();
    }

    vk::DescriptorSetLayout DescriptorCache::CreateDescriptorSetLayout(ArrayView<const ShaderUniforms> specification)
    {
        auto& vulkan = GetCurrentVulkanContext();

        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
        std::vector<vk::DescriptorBindingFlags> bindingFlags;
        size_t totalUniformCount = 0;
        for (const auto& uniformsPerStage : specification)
            totalUniformCount += uniformsPerStage.Uniforms.size();
        layoutBindings.reserve(totalUniformCount);
        bindingFlags.reserve(totalUniformCount);

        for (const auto& uniformsPerStage : specification)
        {
            for (const auto& uniform : uniformsPerStage.Uniforms)
            {
                auto layoutIt = std::find_if(layoutBindings.begin(), layoutBindings.end(),
                    [&uniform](const auto& layout) { return layout.binding == uniform.Binding; });

                if (layoutIt != layoutBindings.end())
                {
                    assert(layoutIt->descriptorType == ToNative(uniform.Type));
                    assert(layoutIt->descriptorCount == uniform.Count);
                    layoutIt->stageFlags |= ToNative(uniformsPerStage.ShaderStage);
                    continue; // do not add new binding
                }

                layoutBindings.push_back(vk::DescriptorSetLayoutBinding{
                    uniform.Binding,
                    ToNative(uniform.Type),
                    uniform.Count,
                    ToNative(uniformsPerStage.ShaderStage)
                });

                vk::DescriptorBindingFlags descriptorBindingFlags = { };
                descriptorBindingFlags |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;
                if (uniform.Count > 1)
                    descriptorBindingFlags |= vk::DescriptorBindingFlagBits::ePartiallyBound;
                bindingFlags.push_back(descriptorBindingFlags);
            }
        }

        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo;
        bindingFlagsCreateInfo.setBindingFlags(bindingFlags);

        vk::DescriptorSetLayoutCreateInfo layoutCreateInfo;
        layoutCreateInfo.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
        layoutCreateInfo.setBindings(layoutBindings);
        layoutCreateInfo.setPNext(&bindingFlagsCreateInfo);

        return vulkan.GetDevice().createDescriptorSetLayout(layoutCreateInfo);
    }

    vk::DescriptorSet DescriptorCache::AllocateDescriptorSet(vk::DescriptorSetLayout layout)
    {
        auto& vulkan = GetCurrentVulkanContext();

        vk::DescriptorSetAllocateInfo descriptorAllocateInfo;
        descriptorAllocateInfo
            .setDescriptorPool(this->descriptorPool)
            .setSetLayouts(layout);

        return vulkan.GetDevice().allocateDescriptorSets(descriptorAllocateInfo).front();
    }

    void DescriptorCache::DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout)
    {
        GetCurrentVulkanContext().GetDevice().destroyDescriptorSetLayout(layout);
    }

    void DescriptorCache::FreeDescriptorSet(vk::DescriptorSet set)
    {
        GetCurrentVulkanContext().GetDevice().freeDescriptorSets(this->descriptorPool, set);
    }

    DescriptorCache::Descriptor DescriptorCache::GetDescriptor(ArrayView<const ShaderUniforms> specification)
    {
        auto descriptorSetLayout = this->CreateDescriptorSetLayout(specification);
        auto descriptorSet = this->AllocateDescriptorSet(descriptorSetLayout);
        return this->cache.emplace_back(Descriptor{ descriptorSetLayout, descriptorSet });
    }
}