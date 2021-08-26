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

#include "ShaderReflection.h"
#include "VectorMath.h"

#include <vulkan/vulkan.hpp>

namespace VulkanAbstractionLayer
{
    static vk::Format FormatTable[] = {
        vk::Format::eUndefined,
        vk::Format::eR4G4UnormPack8,
        vk::Format::eR4G4B4A4UnormPack16,
        vk::Format::eB4G4R4A4UnormPack16,
        vk::Format::eR5G6B5UnormPack16,
        vk::Format::eB5G6R5UnormPack16,
        vk::Format::eR5G5B5A1UnormPack16,
        vk::Format::eB5G5R5A1UnormPack16,
        vk::Format::eA1R5G5B5UnormPack16,
        vk::Format::eR8Unorm,
        vk::Format::eR8Snorm,
        vk::Format::eR8Uscaled,
        vk::Format::eR8Sscaled,
        vk::Format::eR8Uint,
        vk::Format::eR8Sint,
        vk::Format::eR8Srgb,
        vk::Format::eR8G8Unorm,
        vk::Format::eR8G8Snorm,
        vk::Format::eR8G8Uscaled,
        vk::Format::eR8G8Sscaled,
        vk::Format::eR8G8Uint,
        vk::Format::eR8G8Sint,
        vk::Format::eR8G8Srgb,
        vk::Format::eR8G8B8Unorm,
        vk::Format::eR8G8B8Snorm,
        vk::Format::eR8G8B8Uscaled,
        vk::Format::eR8G8B8Sscaled,
        vk::Format::eR8G8B8Uint,
        vk::Format::eR8G8B8Sint,
        vk::Format::eR8G8B8Srgb,
        vk::Format::eB8G8R8Unorm,
        vk::Format::eB8G8R8Snorm,
        vk::Format::eB8G8R8Uscaled,
        vk::Format::eB8G8R8Sscaled,
        vk::Format::eB8G8R8Uint,
        vk::Format::eB8G8R8Sint,
        vk::Format::eB8G8R8Srgb,
        vk::Format::eR8G8B8A8Unorm,
        vk::Format::eR8G8B8A8Snorm,
        vk::Format::eR8G8B8A8Uscaled,
        vk::Format::eR8G8B8A8Sscaled,
        vk::Format::eR8G8B8A8Uint,
        vk::Format::eR8G8B8A8Sint,
        vk::Format::eR8G8B8A8Srgb,
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eB8G8R8A8Snorm,
        vk::Format::eB8G8R8A8Uscaled,
        vk::Format::eB8G8R8A8Sscaled,
        vk::Format::eB8G8R8A8Uint,
        vk::Format::eB8G8R8A8Sint,
        vk::Format::eB8G8R8A8Srgb,
        vk::Format::eA8B8G8R8UnormPack32,
        vk::Format::eA8B8G8R8SnormPack32,
        vk::Format::eA8B8G8R8UscaledPack32,
        vk::Format::eA8B8G8R8SscaledPack32,
        vk::Format::eA8B8G8R8UintPack32,
        vk::Format::eA8B8G8R8SintPack32,
        vk::Format::eA8B8G8R8SrgbPack32,
        vk::Format::eA2R10G10B10UnormPack32,
        vk::Format::eA2R10G10B10SnormPack32,
        vk::Format::eA2R10G10B10UscaledPack32,
        vk::Format::eA2R10G10B10SscaledPack32,
        vk::Format::eA2R10G10B10UintPack32,
        vk::Format::eA2R10G10B10SintPack32,
        vk::Format::eA2B10G10R10UnormPack32,
        vk::Format::eA2B10G10R10SnormPack32,
        vk::Format::eA2B10G10R10UscaledPack32,
        vk::Format::eA2B10G10R10SscaledPack32,
        vk::Format::eA2B10G10R10UintPack32,
        vk::Format::eA2B10G10R10SintPack32,
        vk::Format::eR16Unorm,
        vk::Format::eR16Snorm,
        vk::Format::eR16Uscaled,
        vk::Format::eR16Sscaled,
        vk::Format::eR16Uint,
        vk::Format::eR16Sint,
        vk::Format::eR16Sfloat,
        vk::Format::eR16G16Unorm,
        vk::Format::eR16G16Snorm,
        vk::Format::eR16G16Uscaled,
        vk::Format::eR16G16Sscaled,
        vk::Format::eR16G16Uint,
        vk::Format::eR16G16Sint,
        vk::Format::eR16G16Sfloat,
        vk::Format::eR16G16B16Unorm,
        vk::Format::eR16G16B16Snorm,
        vk::Format::eR16G16B16Uscaled,
        vk::Format::eR16G16B16Sscaled,
        vk::Format::eR16G16B16Uint,
        vk::Format::eR16G16B16Sint,
        vk::Format::eR16G16B16Sfloat,
        vk::Format::eR16G16B16A16Unorm,
        vk::Format::eR16G16B16A16Snorm,
        vk::Format::eR16G16B16A16Uscaled,
        vk::Format::eR16G16B16A16Sscaled,
        vk::Format::eR16G16B16A16Uint,
        vk::Format::eR16G16B16A16Sint,
        vk::Format::eR16G16B16A16Sfloat,
        vk::Format::eR32Uint,
        vk::Format::eR32Sint,
        vk::Format::eR32Sfloat,
        vk::Format::eR32G32Uint,
        vk::Format::eR32G32Sint,
        vk::Format::eR32G32Sfloat,
        vk::Format::eR32G32B32Uint,
        vk::Format::eR32G32B32Sint,
        vk::Format::eR32G32B32Sfloat,
        vk::Format::eR32G32B32A32Uint,
        vk::Format::eR32G32B32A32Sint,
        vk::Format::eR32G32B32A32Sfloat,
        vk::Format::eR64Uint,
        vk::Format::eR64Sint,
        vk::Format::eR64Sfloat,
        vk::Format::eR64G64Uint,
        vk::Format::eR64G64Sint,
        vk::Format::eR64G64Sfloat,
        vk::Format::eR64G64B64Uint,
        vk::Format::eR64G64B64Sint,
        vk::Format::eR64G64B64Sfloat,
        vk::Format::eR64G64B64A64Uint,
        vk::Format::eR64G64B64A64Sint,
        vk::Format::eR64G64B64A64Sfloat,
        vk::Format::eB10G11R11UfloatPack32,
        vk::Format::eE5B9G9R9UfloatPack32,
        vk::Format::eD16Unorm,
        vk::Format::eX8D24UnormPack32,
        vk::Format::eD32Sfloat,
        vk::Format::eS8Uint,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD32SfloatS8Uint,
    };

    vk::ShaderStageFlagBits ShaderStageTable[] = {
        vk::ShaderStageFlagBits::eVertex,
        vk::ShaderStageFlagBits::eTessellationControl,
        vk::ShaderStageFlagBits::eTessellationEvaluation,
        vk::ShaderStageFlagBits::eGeometry,
        vk::ShaderStageFlagBits::eFragment,
        vk::ShaderStageFlagBits::eCompute,
        vk::ShaderStageFlagBits::eRaygenKHR,
        vk::ShaderStageFlagBits::eIntersectionKHR,
        vk::ShaderStageFlagBits::eAnyHitKHR,
        vk::ShaderStageFlagBits::eClosestHitKHR,
        vk::ShaderStageFlagBits::eMissKHR,
        vk::ShaderStageFlagBits::eCallableKHR,
        vk::ShaderStageFlagBits::eTaskNV,
        vk::ShaderStageFlagBits::eMeshNV,
    };

    static vk::DescriptorType DescriptorTypeTable[] = {
        vk::DescriptorType::eSampler,
        vk::DescriptorType::eCombinedImageSampler,
        vk::DescriptorType::eSampledImage,
        vk::DescriptorType::eStorageImage,
        vk::DescriptorType::eUniformTexelBuffer,
        vk::DescriptorType::eStorageTexelBuffer,
        vk::DescriptorType::eUniformBuffer,
        vk::DescriptorType::eStorageBuffer,
        vk::DescriptorType::eUniformBufferDynamic,
        vk::DescriptorType::eStorageBufferDynamic,
        vk::DescriptorType::eInputAttachment,
        vk::DescriptorType::eInlineUniformBlockEXT,
        vk::DescriptorType::eAccelerationStructureKHR,
    };

    static vk::ImageLayout ImageLayoutTable[] = {
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eAttachmentOptimalKHR,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eUndefined,
    };

    template<>
    TypeSPIRV TypeSPIRV::As<float>()
    {
        return { Format::R32_SFLOAT, 1, sizeof(float) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<Vector2>()
    {
        return { Format::R32G32_SFLOAT, 1, sizeof(Vector2) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<Vector3>()
    {
        return { Format::R32G32B32_SFLOAT, 1, sizeof(Vector3) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<Vector4>()
    {
        return { Format::R32G32B32A32_SFLOAT, 1, sizeof(Vector4) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<int32_t>()
    {
        return { Format::R32_SINT, 1, sizeof(int32_t) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<uint32_t>()
    {
        return { Format::R32_UINT, 1, sizeof(uint32_t) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<VectorInt2>()
    {
        return { Format::R32G32_SINT, 1, sizeof(VectorInt2) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<VectorInt3>()
    {
        return { Format::R32G32B32_SINT, 1, sizeof(VectorInt3) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<VectorInt4>()
    {
        return { Format::R32G32B32A32_SINT, 1, sizeof(VectorInt4) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<Matrix2x2>()
    {
        return { Format::R32G32_SFLOAT, 2, sizeof(Matrix2x2) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<Matrix3x3>()
    {
        return { Format::R32G32B32_SFLOAT, 3, sizeof(Matrix3x3) };
    }

    template<>
    TypeSPIRV TypeSPIRV::As<Matrix4x4>()
    {
        return { Format::R32G32B32A32_SFLOAT, 4, sizeof(Matrix4x4) };
    }


    const vk::Format& ToNative(Format format)
    {
        return FormatTable[(size_t)format];
    }

    Format FromNative(const vk::Format& format)
    {
        for (size_t i = 0; i < std::size(FormatTable); i++)
        {
            if (FormatTable[i] == format)
                return (Format)i;
        }
        assert(false);
        return (Format)0;
    }

    const vk::DescriptorType& ToNative(UniformType type)
    {
        return DescriptorTypeTable[(size_t)type];
    }

    UniformType FromNative(const vk::DescriptorType& type)
    {
        for (size_t i = 0; i < std::size(DescriptorTypeTable); i++)
        {
            if (DescriptorTypeTable[i] == type)
                return (UniformType)i;
        }
        assert(false);
        return (UniformType)0;
    }

    const vk::ImageLayout& UniformTypeToImageLayout(UniformType type)
    {
        return ImageLayoutTable[(size_t)type];
    }

    const vk::ShaderStageFlagBits& ToNative(ShaderType type)
    {
        return ShaderStageTable[(size_t)type];
    }

    ShaderType FromNative(const vk::ShaderStageFlagBits& type)
    {
        for (size_t i = 0; i < std::size(ShaderStageTable); i++)
        {
            if (ShaderStageTable[i] == type)
                return (ShaderType)i;
        }
        assert(false);
        return (ShaderType)0;
    }
}