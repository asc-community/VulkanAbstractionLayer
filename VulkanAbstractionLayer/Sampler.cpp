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

#include "Sampler.h"
#include "VulkanContext.h"

namespace VulkanAbstractionLayer
{
	const vk::Sampler& Sampler::GetNativeHandle() const
	{
		return this->handle;
	}

	vk::Filter FilterToNative(Sampler::Filter filter)
	{
		switch (filter)
		{
		case Sampler::Filter::NEAREST:
			return vk::Filter::eNearest;
		case Sampler::Filter::LINEAR:
			return vk::Filter::eLinear;
		default:
			assert(false);
			return vk::Filter::eNearest;
		}
	}

	vk::SamplerMipmapMode MipmapToNative(Sampler::Filter filter)
	{
		switch (filter)
		{
		case Sampler::Filter::NEAREST:
			return vk::SamplerMipmapMode::eNearest;
		case Sampler::Filter::LINEAR:
			return vk::SamplerMipmapMode::eLinear;
		default:
			assert(false);
			return vk::SamplerMipmapMode::eNearest;
		}
	}

	vk::SamplerAddressMode AddressToNative(Sampler::AddressMode address)
	{
		switch (address)
		{
		case Sampler::AddressMode::REPEAT:
			return vk::SamplerAddressMode::eRepeat;
		case Sampler::AddressMode::MIRRORED_REPEAT:
			return vk::SamplerAddressMode::eMirroredRepeat;
		case Sampler::AddressMode::CLAMP_TO_EDGE:
			return vk::SamplerAddressMode::eClampToEdge;
		case Sampler::AddressMode::CLAMP_TO_BORDER:
			return vk::SamplerAddressMode::eClampToBorder;
		default:
			assert(false);
			return vk::SamplerAddressMode::eClampToEdge;
		}
	}

	Sampler::Sampler(MinFilter minFilter, MagFilter magFilter, AddressMode uvwAddress, MipFilter mipFilter)
	{
		this->Init(minFilter, magFilter, uvwAddress, mipFilter);
	}

	void Sampler::Init(MinFilter minFilter, MagFilter magFilter, AddressMode uvwAddress, MipFilter mipFilter)
	{
		vk::SamplerCreateInfo samplerCreateInfo;
		samplerCreateInfo
			.setMinFilter(FilterToNative(minFilter))
			.setMagFilter(FilterToNative(magFilter))
			.setAddressModeU(AddressToNative(uvwAddress))
			.setAddressModeV(AddressToNative(uvwAddress))
			.setAddressModeW(AddressToNative(uvwAddress))
			.setMipmapMode(MipmapToNative(mipFilter));

		this->handle = GetCurrentVulkanContext().GetDevice().createSampler(samplerCreateInfo);
	}

	void Sampler::Destroy()
	{
		if ((bool)this->handle) GetCurrentVulkanContext().GetDevice().destroySampler(this->handle);
	}

	Sampler::~Sampler()
	{
		this->Destroy();
	}

	Sampler::Sampler(Sampler&& other) noexcept
	{
		this->handle = other.handle;
		other.handle = vk::Sampler{ };
	}

	Sampler& Sampler::operator=(Sampler&& other) noexcept
	{
		this->Destroy();
		this->handle = other.handle;
		other.handle = vk::Sampler{ };
		return *this;
	}
}