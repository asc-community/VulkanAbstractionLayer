#include "ImageLoader.h"
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

#include "ImageLoader.h"

#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>

namespace VulkanAbstractionLayer
{
    static bool IsDDSImage(const std::string& filepath)
    {
        std::filesystem::path filename{ filepath };
        return filename.extension() == ".dds";
    }

    struct ChannelInfo
    {
        uint32_t Count = 0;
        uint32_t ByteSize = 0;
    };

    static ChannelInfo GetChannelInfo(tinyddsloader::DDSFile::DXGIFormat format)
    {
        switch (format)
        {
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Float:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_SInt:
            return ChannelInfo{ 4, 4 };
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Float:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_SInt:
            return ChannelInfo{ 3, 4 };
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Float:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SInt:
            return ChannelInfo{ 4, 2 };
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_Float:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_SInt:
            return ChannelInfo{ 2, 4 };
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SInt:
            return ChannelInfo{ 4, 1 };
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_Float:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_UNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_SNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_SInt:
            return ChannelInfo{ 2, 2 };
        case tinyddsloader::DDSFile::DXGIFormat::R32_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::D32_Float:
        case tinyddsloader::DDSFile::DXGIFormat::R32_Float:
        case tinyddsloader::DDSFile::DXGIFormat::R32_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R32_SInt:
            return ChannelInfo{ 1, 4 };
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_UNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_SNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_SInt:
            return ChannelInfo{ 2, 1 };
        case tinyddsloader::DDSFile::DXGIFormat::R16_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R16_Float:
        case tinyddsloader::DDSFile::DXGIFormat::D16_UNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R16_UNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R16_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R16_SNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R16_SInt:
            return ChannelInfo{ 1, 2 };
        case tinyddsloader::DDSFile::DXGIFormat::R8_Typeless:
        case tinyddsloader::DDSFile::DXGIFormat::R8_UNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R8_UInt:
        case tinyddsloader::DDSFile::DXGIFormat::R8_SNorm:
        case tinyddsloader::DDSFile::DXGIFormat::R8_SInt:
        case tinyddsloader::DDSFile::DXGIFormat::A8_UNorm:
            return ChannelInfo{ 1, 1 };
        default:
            assert(false); // unsupported format
            return ChannelInfo{ 0, 0 };
        }
    }

    static ImageData LoadImageUsingDDSLoader(const std::string& filepath)
    {
        ImageData image;
        tinyddsloader::DDSFile dds;
        auto result = dds.Load(filepath.c_str());
        if (result != tinyddsloader::Result::Success)
            return image;

        dds.Flip();
        auto imageData = dds.GetImageData();
        auto channelInfo = GetChannelInfo(dds.GetFormat());
        image.Width = imageData->m_height;
        image.Height = imageData->m_height;
        image.Channels = channelInfo.Count;
        image.ChannelSize = channelInfo.ByteSize;
        image.ByteData.resize(imageData->m_memSlicePitch);
        std::copy((const uint8_t*)imageData->m_mem, (const uint8_t*)imageData->m_mem + imageData->m_memSlicePitch, image.ByteData.begin());

        return image;
    }

    static ImageData LoadImageUsingSTBLoader(const std::string& filepath)
    {
        int width = 0, height = 0, channels = 0;
        const uint32_t actualChannels = 4;
        stbi_set_flip_vertically_on_load(true);
        uint8_t* data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        std::vector<uint8_t> vecData;
        vecData.resize(width * height * actualChannels * sizeof(uint8_t));
        std::copy(data, data + vecData.size(), vecData.begin());
        stbi_image_free(data);
        return ImageData{ std::move(vecData), (uint32_t)width, (uint32_t)height, (uint32_t)actualChannels, sizeof(uint8_t) };
    }

    ImageData ImageLoader::LoadFromFile(const std::string& filepath)
    {
        if (IsDDSImage(filepath))
            return LoadImageUsingDDSLoader(filepath);
        else
            return LoadImageUsingSTBLoader(filepath);
    }
}