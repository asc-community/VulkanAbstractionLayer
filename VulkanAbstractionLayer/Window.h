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

#include "VectorMath.h"
#include <vector>
#include <functional>

struct GLFWwindow;

namespace VulkanAbstractionLayer
{
    class VulkanContext;
    struct WindowSurface;

    struct WindowCreateOptions
    {
        bool TransparentFramebuffer = false;
        bool Resizeable = true;
        bool TileBar = true;
        void (*ErrorCallback)(const char*) = nullptr;
        Vector2 Size{ 800.0f, 600.0f };
        Vector2 Position{ 0.0f, 0.0f };
        const char* Title = "VulkanAbstractionLayer";
    };

    class Window
    {
        GLFWwindow* handle = nullptr;
        std::function<void(Window&, Vector2)> onResize;
    public:
        GLFWwindow* GetNativeHandle() { return this->handle; }
        const GLFWwindow* GetNativeHandle() const { return this->handle; }
        std::vector<const char*> GetRequiredExtensions() const;

        Window(const WindowCreateOptions& options);
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;
        ~Window();

        void PollEvents() const;
        bool ShouldClose() const;

        void SetSize(const Vector2& size);
        void SetPosition(const Vector3& position);

        void OnResize(std::function<void(Window&, Vector2)> callback);

        const WindowSurface& CreateWindowSurface(const VulkanContext& context);
    };
}