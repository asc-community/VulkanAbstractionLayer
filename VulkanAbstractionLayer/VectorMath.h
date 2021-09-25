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

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/simd/matrix.h>
#include <utility>

namespace VulkanAbstractionLayer
{
    using Vector2 = glm::vec2;
    using Vector3 = glm::vec3;
    using Vector4 = glm::vec4;

    using Matrix2x2 = glm::mat2;
    using Matrix3x3 = glm::mat3;
    using Matrix4x4 = glm::mat4;
    using Matrix3x4 = glm::mat3x4;

    using VectorInt2 = glm::vec<2, int32_t>;
    using VectorInt3 = glm::vec<3, int32_t>;
    using VectorInt4 = glm::vec<4, int32_t>;

    constexpr float HalfPi = glm::half_pi<float>();
    constexpr float Pi = glm::pi<float>();
    constexpr float TwoPi = glm::two_pi<float>();

    template<typename T>
    T Normalize(const T& v)
    {
        return glm::normalize(v);
    }

    template<typename T>
    float Length(const T& v)
    {
        return glm::length(v);
    }

    template<typename T>
    float Dot(const T& v1, const T& v2)
    {
        return glm::dot(v1, v2);
    }

    inline float ToRadians(float degrees)
    {
        return glm::radians(degrees);
    }

    inline float ToDegrees(float radians)
    {
        return glm::degrees(radians);
    }

    inline Matrix4x4 MakeRotationMatrix(const Vector3& rotations)
    {
        return glm::yawPitchRoll(rotations.y, rotations.x, rotations.z);
    }

    inline Vector3 MakeRotationAngles(const Matrix4x4& matrix)
    {
        Vector3 result{ 0.0f, 0.0f, 0.0f };
        glm::extractEulerAngleXYZ(matrix, result.x, result.y, result.z);
        return result;
    }

    inline Matrix4x4 MakeScaleMatrix(const Vector3& scale)
    {
        Matrix4x4 result{ 1.0f };
        result[0][0] = scale.x;
        result[1][1] = scale.y;
        result[2][2] = scale.z;
        return result;
    }

    inline Matrix4x4 MakePerspectiveMatrix(float fov, float aspect, float znear, float zfar)
    {
        return glm::perspective(fov, aspect, znear, zfar);
    }

    inline Matrix4x4 MakeLookAtMatrix(const Vector3& position, const Vector3& direction, const Vector3& up)
    {
        return glm::lookAt(position, position + direction, up);
    }

    inline Matrix4x4 MakeOrthographicMatrix(float xLow, float xHigh, float yLow, float yHigh, float zLow, float zHigh)
    {
        return glm::ortho(xLow, xHigh, yLow, yHigh, zLow, zHigh);
    }

    inline std::pair<Vector3, Vector3> ComputeTangentSpace(
        const Vector3& pos1, const Vector3& pos2, const Vector3& pos3,
        const Vector2& tex1, const Vector2& tex2, const Vector2& tex3
    )
    {
        // Edges of the triangle : postion delta
        auto deltaPos1 = pos2 - pos1;
        auto deltaPos2 = pos3 - pos1;

        // texture delta
        auto deltaT1 = tex2 - tex1;
        auto deltaT2 = tex3 - tex1;

        float r = 1.0f / (deltaT1.x * deltaT2.y - deltaT1.y * deltaT2.x);
        auto tangent = (deltaPos1 * deltaT2.y - deltaPos2 * deltaT1.y) * r;
        auto bitangent = (deltaPos2 * deltaT1.x - deltaPos1 * deltaT2.x) * r;

        return std::make_pair(Normalize(tangent), Normalize(bitangent));
    }
}