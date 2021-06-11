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

#include <vulkan/vulkan.hpp>

#include "ShaderReflection.h"
#include "ArrayUtils.h"

namespace VulkanAbstractionLayer
{
	class DescriptorCache
	{
	public:
		struct Descriptor
		{
			vk::DescriptorSetLayout SetLayout;
			vk::DescriptorSet Set;
		};

	private:
		using LayoutSpecification = std::vector<ShaderUniforms>;

		struct DescriptorCacheEntry
		{
			LayoutSpecification Specification;
			Descriptor SetInfo;
		};

		vk::DescriptorPool descriptorPool;
		std::vector<DescriptorCacheEntry> cache;

		vk::DescriptorSetLayout CreateDescriptorSetLayout(ArrayView<ShaderUniforms> specification);
		vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout layout);
		void DestroyDescriptorSetLayout(vk::DescriptorSetLayout layout);
		void FreeDescriptorSet(vk::DescriptorSet set);
	public:

		void Init();
		void Destroy();

		const auto& GetDescriptorPool() const { return this->descriptorPool; }

		Descriptor GetDescriptor(ArrayView<ShaderUniforms> specification);
	};
}