#pragma once

#include "components/simple_scene.h"


namespace m1
{
    struct MeshInfo
    {
        std::string name{ "box" };
        std::string group{ "primitives" };
        glm::vec3 pos{ 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
    };

    class Lab1 : public gfxc::SimpleScene
    {
     public:
         Lab1() = default;

        void Init() override;

     private:
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;

        void OnInputUpdate(float deltaTime, int mods) override;
        void OnKeyPress(int key, int mods) override;
        void OnKeyRelease(int key, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
        void OnWindowResize(int width, int height) override;

        // TODO(student): Class variables go here
        glm::vec3 m_bgColor{ 0.0f, 0.0f, 0.0f };
        bool m_randomizeColor{ false };
        int m_currModelIndex{ 0 };
        static MeshInfo m_meshes[4];
        bool m_playModelRoulette{ false };
        double m_rouletteStartTime{ 0.0 };
    };
}   // namespace m1
