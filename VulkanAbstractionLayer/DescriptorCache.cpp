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
    bool IsLayoutSubSet(const std::vector<Uniform>& uniforms, const std::vector<Uniform>& subuniforms)
    {
        return std::all_of(subuniforms.begin(), subuniforms.end(), [&uniforms](const Uniform& uniform)
        {
            return std::find(uniforms.begin(), uniforms.end(), uniform) != uniforms.end();
        });
    }

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
            .setPoolSizes(descriptorPoolSizes)
            .setMaxSets(1024 * (uint32_t)descriptorPoolSizes.size());

        this->descriptorPool = vulkan.GetDevice().createDescriptorPool(descriptorPoolCreateInfo);
	}

    void DescriptorCache::Destroy()
    {
        auto& vulkan = GetCurrentVulkanContext();
        if ((bool)this->descriptorPool) vulkan.GetDevice().destroyDescriptorPool(this->descriptorPool);

        for (const auto& [specification, descriptor] : this->cache)
        {
            vulkan.GetDevice().destroyDescriptorSetLayout(descriptor.SetLayout);
        }
        this->cache.clear();
    }

    vk::DescriptorSetLayout DescriptorCache::CreateDescriptorSetLayout(ArrayView<ShaderUniforms> specification)
    {
        auto& vulkan = GetCurrentVulkanContext();

        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
        size_t totalUniformCount = 0;
        for (const auto& uniformsPerStage : specification)
            totalUniformCount += uniformsPerStage.Uniforms.size();
        layoutBindings.reserve(totalUniformCount);

        for (const auto& uniformsPerStage : specification)
        {
            for (const auto& uniform : uniformsPerStage.Uniforms)
            {
                layoutBindings.push_back(vk::DescriptorSetLayoutBinding{
                    uniform.Binding,
                    ToNative(uniform.Type),
                    uniform.Count,
                    ToNative(uniformsPerStage.ShaderStage)
                });
            }
        }

        vk::DescriptorSetLayoutCreateInfo layoutCreateInfo;
        layoutCreateInfo.setBindings(layoutBindings);

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

    DescriptorCache::Descriptor DescriptorCache::GetDescriptor(ArrayView<ShaderUniforms> specification)
    {
        auto searchIt = std::find_if(this->cache.begin(), this->cache.end(), [&specification](const DescriptorCacheEntry& e)
        {
            return std::equal(e.Specification.begin(), e.Specification.end(), specification.begin(), specification.end(),
                [](const ShaderUniforms& u1, const ShaderUniforms& u2) 
                {
                    return u1.ShaderStage == u2.ShaderStage && IsLayoutSubSet(u1.Uniforms, u2.Uniforms);
                });
        });
        if (searchIt != this->cache.end()) return searchIt->Set;

        auto removeIt = std::remove_if(this->cache.begin(), this->cache.end(), [&specification](const DescriptorCacheEntry& e)
        {
            return std::equal(e.Specification.begin(), e.Specification.end(), specification.begin(), specification.end(),
                [](const ShaderUniforms& u1, const ShaderUniforms& u2)
                {
                    return u1.ShaderStage == u2.ShaderStage && IsLayoutSubSet(u2.Uniforms, u1.Uniforms);
                });
        });
        this->cache.erase(removeIt, this->cache.end());

        auto descriptorSetLayout = this->CreateDescriptorSetLayout(specification);
        auto descriptorSet = this->AllocateDescriptorSet(descriptorSetLayout);
        this->cache.push_back({ 
            LayoutSpecification{ specification.begin(), specification.end() }, 
            Descriptor{ descriptorSetLayout, descriptorSet }
        });

        return Descriptor{ descriptorSetLayout, descriptorSet };
    }
}