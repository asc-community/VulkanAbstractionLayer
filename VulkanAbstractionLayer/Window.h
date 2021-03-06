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

#include "VectorMath.h"
#include <vector>
#include <string>
#include <functional>

struct GLFWwindow;

namespace VulkanAbstractionLayer
{
    enum class KeyCode
    {
        UNKNOWN       =  -1,
                           
        SPACE         =  32,
        APOSTROPHE    =  39,
        COMMA         =  44,
        MINUS         =  45,
        PERIOD        =  46,
        SLASH         =  47,
        _0            =  48,
        _1            =  49,
        _2            =  50,
        _3            =  51,
        _4            =  52,
        _5            =  53,
        _6            =  54,
        _7            =  55,
        _8            =  56,
        _9            =  57,
        SEMICOLON     =  59,
        EQUAL         =  61,
        A             =  65,
        B             =  66,
        C             =  67,
        D             =  68,
        E             =  69,
        F             =  70,
        G             =  71,
        H             =  72,
        I             =  73,
        J             =  74,
        K             =  75,
        L             =  76,
        M             =  77,
        N             =  78,
        O             =  79,
        P             =  80,
        Q             =  81,
        R             =  82,
        S             =  83,
        T             =  84,
        U             =  85,
        V             =  86,
        W             =  87,
        X             =  88,
        Y             =  89,
        Z             =  90,
        LEFT_BRACKET  =  91,
        BACKSLASH     =  92,
        RIGHT_BRACKET =  93,
        GRAVE_ACCENT  =  96,
        WORLD_1       = 161,
        WORLD_2       = 162,

        ESCAPE        = 256,
        ENTER         = 257,
        TAB           = 258,
        BACKSPACE     = 259,
        INSERT        = 260,
        DELETE        = 261,
        RIGHT         = 262,
        LEFT          = 263,
        DOWN          = 264,
        UP            = 265,
        PAGE_UP       = 266,
        PAGE_DOWN     = 267,
        HOME          = 268,
        END           = 269,
        CAPS_LOCK     = 280,
        SCROLL_LOCK   = 281,
        NUM_LOCK      = 282,
        PRINT_SCREEN  = 283,
        PAUSE         = 284,
        F1            = 290,
        F2            = 291,
        F3            = 292,
        F4            = 293,
        F5            = 294,
        F6            = 295,
        F7            = 296,
        F8            = 297,
        F9            = 298,
        F10           = 299,
        F11           = 300,
        F12           = 301,
        F13           = 302,
        F14           = 303,
        F15           = 304,
        F16           = 305,
        F17           = 306,
        F18           = 307,
        F19           = 308,
        F20           = 309,
        F21           = 310,
        F22           = 311,
        F23           = 312,
        F24           = 313,
        F25           = 314,
        KP_0          = 320,
        KP_1          = 321,
        KP_2          = 322,
        KP_3          = 323,
        KP_4          = 324,
        KP_5          = 325,
        KP_6          = 326,
        KP_7          = 327,
        KP_8          = 328,
        KP_9          = 329,
        KP_DECIMAL    = 330,
        KP_DIVIDE     = 331,
        KP_MULTIPLY   = 332,
        KP_SUBTRACT   = 333,
        KP_ADD        = 334,
        KP_ENTER      = 335,
        KP_EQUAL      = 336,
        LEFT_SHIFT    = 340,
        LEFT_CONTROL  = 341,
        LEFT_ALT      = 342,
        LEFT_SUPER    = 343,
        RIGHT_SHIFT   = 344,
        RIGHT_CONTROL = 345,
        RIGHT_ALT     = 346,
        RIGHT_SUPER   = 347,
        MENU          = 348,
    };

    enum class MouseButton
    {
        _1     =  0,
        _2     =  1,
        _3     =  2,
        _4     =  3,
        _5     =  4,
        _6     =  5,
        _7     =  6,
        _8     =  7,
        LAST   = MouseButton::_8,
        LEFT   = MouseButton::_1,
        RIGHT  = MouseButton::_2,
        MIDDLE = MouseButton::_3,
    };

    enum class CursorMode
    {
        NORMAL = 0x00034001,
        HIDDEN = 0x00034002,
        DISABLED = 0x00034003,
    };

    inline void DefaultWindowCallback(const std::string&) { }

    class VulkanContext;
    struct WindowSurface;

    struct WindowCreateOptions
    {
        bool TransparentFramebuffer = false;
        bool Resizeable = true;
        bool TileBar = true;
        std::function<void(const std::string&)> ErrorCallback = DefaultWindowCallback;
        Vector2 Size{ 800.0f, 600.0f };
        Vector2 Position{ 0.0f, 0.0f };
        const char* Title = "VulkanAbstractionLayer";
    };

    class Window
    {
        GLFWwindow* handle = nullptr;
        std::function<void(Window&, Vector2)> onResize;
        std::function<void(Window&, KeyCode, bool)> onKeyChanged;
        std::function<void(Window&, MouseButton, bool)> onMouseChanged;
    public:
        GLFWwindow* GetNativeHandle() const { return this->handle; }
        std::vector<const char*> GetRequiredExtensions() const;

        Window(const WindowCreateOptions& options);
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;
        ~Window();

        void PollEvents() const;
        bool ShouldClose() const;
        void Close();

        void SetSize(Vector2 size);
        Vector2 GetSize() const;
        void SetPosition(Vector2 position);
        Vector2 GetPosition() const;
        void SetTitle(const char* title);

        Vector2 GetCursorPosition() const;
        void SetCursorPosition(Vector2 position);

        CursorMode GetCursorMode() const;
        void SetCursorMode(CursorMode mode);

        bool IsKeyPressed(KeyCode key) const;
        bool IsKeyReleased(KeyCode key) const;

        bool IsMousePressed(MouseButton button) const;
        bool IsMouseReleased(MouseButton button) const;

        float GetTimeSinceCreation() const;
        void SetTimeSinceCreation(float time);

        void OnResize(std::function<void(Window&, Vector2)> callback);
        void OnKeyChanged(std::function<void(Window&, KeyCode, bool)> callback);
        void OnMouseChanged(std::function<void(Window&, MouseButton, bool)> callback);

        const WindowSurface& CreateWindowSurface(const VulkanContext& context);

        void SetContext(GLFWwindow* window);
    };
}