#include "lab_m1/Tema1/Tema1.h"

#include <vector>
#include <iostream>
#include <fstream>

#include "core/engine.h"
#include "utils/gl_utils.h"

using namespace std;
using namespace m1;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


Tema1::Tema1()
{
}


Tema1::~Tema1()
{
}


void Tema1::Init()
{
    // Load and generate track
    std::ifstream in("sample_track.txt");

    GLfloat a = 0.0f, b = 0.0f;
    while (in >> a >> b)
    {
        std::cout << a << ' ' << b << '\n';
    }

}

void Tema1::FrameStart()
{
    // Clears the color buffer (using the previously set color) and depth buffer
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::ivec2 resolution = window->GetResolution();

    // Sets the screen area where to draw
    glViewport(0, 0, resolution.x, resolution.y);
}


void Tema1::Update(float deltaTimeSeconds)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Draw the circle
    RenderMesh(meshes["circle"], shaders["VertexColor"], glm::vec3(1.5f, 1.75f, 1.5f), glm::vec3(1.0f));

    glDisable(GL_CULL_FACE);
}


void Tema1::FrameEnd()
{
    DrawCoordinateSystem();
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Tema1::OnInputUpdate(float deltaTime, int mods)
{
}


void Tema1::OnKeyPress(int key, int mods)
{
    // Add key press event
}


void Tema1::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Tema1::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Tema1::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Tema1::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Tema1::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
    // Add mouse scroll event
}


void Tema1::OnWindowResize(int width, int height)
{
    // Add window resize event
}
