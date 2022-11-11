#include "lab_m1/lab1/lab1.h"

#include <vector>
#include <iostream>

using namespace std;
using namespace m1;


MeshInfo Lab1::m_meshes[4] {
    { "bunny", "animals", glm::vec3{ 0.0f, 0.75f, -3.0f }, glm::vec3{ 0.05f } },
    { "teapot", "primitives", glm::vec3{ 0.0f, 0.75f, -3.0f }, glm::vec3{ 1.0f } },
    { "sphere", "primitives", glm::vec3{ 0.0f, 0.75f, -3.0f }, glm::vec3{ 1.0f } },
    { "oildrum", "props", glm::vec3{ 0.0f, 0.75f, -3.0f }, glm::vec3{ 1.0f } }
};


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


void Lab1::Init()
{
    // Load a mesh from file into GPU memory. We only need to do it once,
    // no matter how many times we want to draw this mesh.
    {
        Mesh* mesh = new Mesh("box");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    // TODO(student): Load some more meshes. The value of RESOURCE_PATH::MODELS
    // is actually a path on disk, go there and you will find more meshes.
    for (int i = 0; i < 4; ++i)
    {
        Mesh* mesh = new Mesh(m_meshes[i].name);
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, m_meshes[i].group), m_meshes[i].name + ".obj");
        meshes[mesh->GetMeshID()] = mesh;
    }
}


void Lab1::FrameStart()
{
}


void Lab1::Update(float deltaTimeSeconds)
{
    glm::ivec2 resolution = window->props.resolution;

    // Sets the clear color for the color buffer

    // TODO(student): Generalize the arguments of `glClearColor`.
    // You can, for example, declare three variables in the class header,
    // that will store the color components (red, green, blue).
    if (m_randomizeColor)
    {
        m_bgColor.r = 1.0f;
        m_bgColor.g = (sinf(static_cast<float>(glfwGetTime())) + 1.0f) * 0.5f; 
        m_bgColor.b = (cosf(static_cast<float>(glfwGetTime())) + 1.0f) * 0.5f;
    }
    glClearColor(m_bgColor.r, m_bgColor.g, m_bgColor.b, 1.0f);

    // Clears the color buffer (using the previously set color) and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Sets the screen area where to draw
    glViewport(0, 0, resolution.x, resolution.y);

    // Render the object
    RenderMesh(meshes["box"], glm::vec3(1.0f, 0.5f, 0.0f), glm::vec3(0.5f));

    // Render the object again but with different properties
    RenderMesh(meshes["box"], glm::vec3(-1.0f, 0.5f, 0.0f));

    // TODO(student): We need to render (a.k.a. draw) the mesh that
    // was previously loaded. We do this using `RenderMesh`. Check the
    // signature of this function to see the meaning of its parameters.
    // You can draw the same mesh any number of times.
    if (m_playModelRoulette)
    {
        if ((glfwGetTime() - m_rouletteStartTime) > 1.0)
        {
            m_playModelRoulette = false;
            m_currModelIndex = rand() % 4;
        }
        else
        {
            m_currModelIndex = (m_currModelIndex + 1) % 4;
        }
    }
    RenderMesh(meshes[m_meshes[m_currModelIndex].name], m_meshes[m_currModelIndex].pos, m_meshes[m_currModelIndex].scale);
}


void Lab1::FrameEnd()
{
    DrawCoordinateSystem();
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Lab1::OnInputUpdate(float deltaTime, int mods)
{
    // Treat continuous update based on input
    if (m_playModelRoulette)
        return;

    // TODO(student): Add some key hold events that will let you move
    // a mesh instance on all three axes. You will also need to
    // generalize the position used by `RenderMesh`.
    if (window->KeyHold(GLFW_KEY_A))
    {
        m_meshes[m_currModelIndex].pos.x += 1.0f * deltaTime;
    }
    if (window->KeyHold(GLFW_KEY_D))
    {
        m_meshes[m_currModelIndex].pos.x -= 1.0f * deltaTime;
    }
    if (window->KeyHold(GLFW_KEY_W))
    {
        m_meshes[m_currModelIndex].pos.z += 1.0f * deltaTime;
    }
    if (window->KeyHold(GLFW_KEY_S))
    {
        m_meshes[m_currModelIndex].pos.z -= 1.0f * deltaTime;
    }
    if (window->KeyHold(GLFW_KEY_SPACE))
    {
        m_meshes[m_currModelIndex].pos.y += 1.0f * deltaTime;
    }
    if (window->KeyHold(GLFW_KEY_LEFT_CONTROL))
    {
        m_meshes[m_currModelIndex].pos.y -= 1.0f * deltaTime;
    }
}


void Lab1::OnKeyPress(int key, int mods)
{
    // Add key press events
    // Press R to cycle through colors using sin functions.
    if (key == GLFW_KEY_R)
    {
        m_randomizeColor ^= true; // toggle randomization.
    }

    if (key == GLFW_KEY_F)
    {
        m_randomizeColor = false;
        // TODO(student): Change the values of the color components.
        m_bgColor.r = static_cast<float>((rand() % 256)) / 255.0f;
        m_bgColor.g = static_cast<float>((rand() % 256)) / 255.0f;
        m_bgColor.b = static_cast<float>((rand() % 256)) / 255.0f;
    }

    // TODO(student): Add a key press event that will let you cycle
    // through at least two meshes, rendered at the same position.
    // You will also need to generalize the mesh name used by `RenderMesh`.
    if (key == GLFW_KEY_M && (! m_playModelRoulette))
    {
        if ((mods & GLFW_MOD_CONTROL))
        {
            m_playModelRoulette = true;
            m_rouletteStartTime = glfwGetTime();
        }
        else
        {
            m_currModelIndex = (m_currModelIndex + 1) % 3;
        }
    }
}


void Lab1::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Lab1::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Lab1::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Lab1::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Lab1::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
    // Treat mouse scroll event
}


void Lab1::OnWindowResize(int width, int height)
{
    // Treat window resize event
}
