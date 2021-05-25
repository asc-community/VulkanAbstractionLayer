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

namespace VulkanAbstractionLayer
{
    template<>
    VertexAttribute VertexAttribute::OfType<float>()
    {
        return { Format::R32_SFLOAT, 1, sizeof(float) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Vector2>()
    {
        return { Format::R32G32_SFLOAT, 1, sizeof(Vector2) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Vector3>()
    {
        return { Format::R32G32B32_SFLOAT, 1, sizeof(Vector3) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Vector4>()
    {
        return { Format::R32G32B32A32_SFLOAT, 1, sizeof(Vector4) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<int32_t>()
    {
        return { Format::R32_SINT, 1, sizeof(int32_t) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<VectorInt2>()
    {
        return { Format::R32G32_SINT, 1, sizeof(VectorInt2) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<VectorInt3>()
    {
        return { Format::R32G32B32_SINT, 1, sizeof(VectorInt3) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<VectorInt4>()
    {
        return { Format::R32G32B32A32_SINT, 1, sizeof(VectorInt4) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Matrix2x2>()
    {
        return { Format::R32G32_SFLOAT, 2, sizeof(Matrix2x2) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Matrix3x3>()
    {
        return { Format::R32G32B32_SFLOAT, 3, sizeof(Matrix3x3) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Matrix4x4>()
    {
        return { Format::R32G32B32A32_SFLOAT, 4, sizeof(Matrix4x4) };
    }
}