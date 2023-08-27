#include "lab_m1/Tema2/Tema2.h"

#include <vector>
#include <iostream>
#include <fstream>

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/engine.h"
#include "components/transform.h"
#include "utils/gl_utils.h"

using namespace std;
using namespace m1;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */

static vector<glm::vec3> trees_pos{};
static vector<glm::vec3> cpu_track{};
static glm::vec3 player_pos{};
static glm::vec3 player_dir{};
static glm::vec3 cpu_pos{};
static unsigned int next_point{};
static float cpu_speed = 1.9f;


Tema2::Tema2()
{
}


Tema2::~Tema2()
{
}


void Tema2::Init()
{
    // Load and generate track
    {
        std::ifstream in("sample_track_new_doubled.txt");

        vector<VertexFormat> vertices{};
        vector<unsigned int> indices{};

        GLfloat x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
        int i = 0;
        while (in >> x1 >> y1 >> x2 >> y2)
        {
            std::cout << x1 << ' ' << y1 << '\n';
            std::cout << x2 << ' ' << y2 << '\n';

            glm::vec3 p1 { x1, 0.0f, y1 };
            glm::vec3 p2 { x2, 0.0f, y2 };

            cpu_track.push_back(p1);
            cpu_track.push_back(p2);

            glm::vec3 segment = p2 - p1;
            glm::vec3 side_dir = glm::normalize(glm::cross(segment, glm::vec3_up));

            glm::vec3 r1 = p1 - 1.0f * side_dir;
            glm::vec3 a1 = p1 + 1.0f * side_dir;

            glm::vec3 r2 = p2 - 1.0f * side_dir;
            glm::vec3 a2 = p2 + 1.0f * side_dir;

            trees_pos.push_back(a1 + 1.0f * side_dir);
            trees_pos.push_back(r1 - 1.0f * side_dir);
            trees_pos.push_back(a2 + 1.0f * side_dir);
            trees_pos.push_back(r2 - 1.0f * side_dir);

            vertices.emplace_back(VertexFormat{ a1, glm::vec3 { 0.1f, 0.1f, 0.1f } });
            indices.push_back(i++);
            vertices.emplace_back(VertexFormat{ r1, glm::vec3 { 0.1f, 0.1f, 0.1f } });
            indices.push_back(i++);
            vertices.emplace_back(VertexFormat{ a2, glm::vec3 { 0.1f, 0.1f, 0.1f } });
            indices.push_back(i++);
            vertices.emplace_back(VertexFormat{ r2, glm::vec3 { 0.1f, 0.1f, 0.1f } });
            indices.push_back(i++);
        }

        // Complete track
        indices.push_back(0);
        indices.push_back(1);

        for (int idx : indices)
            std::cout << idx << ' ';
        std::cout << '\n';

        meshes["track"] = new Mesh("track");
        meshes["track"]->InitFromData(vertices, indices);
        meshes["track"]->SetDrawMode(GL_TRIANGLE_STRIP);

        // Create start/finish line somewhere
        int start_index = 28;
        glm::vec3 astart = vertices[start_index].position;
        glm::vec3 rstart = vertices[start_index + 1].position;
        glm::vec3 anext = vertices[start_index + 2].position;
        glm::vec3 rnext = vertices[start_index + 3].position;
        glm::vec3 dir = glm::normalize(anext - astart);
        glm::vec3 aend = astart + 0.2f * dir;
        glm::vec3 rend = rstart + 0.2f * dir;

        vector<VertexFormat> sf_vertices
        {
            VertexFormat{ astart },
            VertexFormat{ rstart },
            VertexFormat{ aend },
            VertexFormat{ rend },
        };
        vector<unsigned int> sf_indices { 0, 1, 2, 3 };

        meshes["start"] = new Mesh("start");
        meshes["start"]->InitFromData(sf_vertices, sf_indices);
        meshes["start"]->SetDrawMode(GL_TRIANGLE_STRIP);
    }

    // Create CPU-controlled car
    {
        vector<VertexFormat> vertices
        {
            VertexFormat{ glm::vec3{ -0.25f,  0.02f, -0.10f }, glm::vec3{ 0.824f, 0.871f, 0.145f } },
            VertexFormat{ glm::vec3{ -0.25f,  0.02f,  0.10f }, glm::vec3{ 0.824f, 0.871f, 0.145f } },
            VertexFormat{ glm::vec3{ -0.25f,  0.20f, -0.10f }, glm::vec3{ 0.824f, 0.871f, 0.145f } },
            VertexFormat{ glm::vec3{ -0.25f,  0.20f,  0.10f }, glm::vec3{ 0.824f, 0.871f, 0.145f } },
            VertexFormat{ glm::vec3{ 0.25f,  0.02f, -0.10f }, glm::vec3{ 0.824f, 0.871f, 0.145f } },
            VertexFormat{ glm::vec3{ 0.25f,  0.02f,  0.10f }, glm::vec3{ 0.824f, 0.871f, 0.145f } },
            VertexFormat{ glm::vec3{ 0.25f,  0.20f, -0.10f }, glm::vec3{ 0.824f, 0.871f, 0.145f } },
            VertexFormat{ glm::vec3{ 0.25f,  0.20f,  0.10f }, glm::vec3{ 0.824f, 0.871f, 0.145f } },
        };

        vector<unsigned int> indices
        {
            4, 6, 5, 7, 3, 6, 2, 4, 0, 5, 1, 3, 0, 2,
        };

        meshes["cpu"] = new Mesh("cpu");
        meshes["cpu"]->InitFromData(vertices, indices);
        meshes["cpu"]->SetDrawMode(GL_TRIANGLE_STRIP);
    }

    // Create player-controlled car
    {
        vector<VertexFormat> vertices
        {
            VertexFormat{ glm::vec3{ -0.10f,  0.02f, -0.25f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ -0.10f,  0.02f,  0.25f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ -0.10f,  0.20f, -0.25f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ -0.10f,  0.20f,  0.25f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ 0.10f,  0.02f, -0.25f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ 0.10f,  0.02f,  0.25f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ 0.10f,  0.20f, -0.25f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ 0.10f,  0.20f,  0.25f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
        };

        vector<unsigned int> indices
        {
            4, 6, 5, 7, 3, 6, 2, 4, 0, 5, 1, 3, 0, 2,
        };

        meshes["player"] = new Mesh("player");
        meshes["player"]->InitFromData(vertices, indices);
        meshes["player"]->SetDrawMode(GL_TRIANGLE_STRIP);
    }

    // Create tree
    {
        vector<VertexFormat> vertices
        {
            // trunk
            VertexFormat{ glm::vec3{ -0.15f,  0.0f, -0.15f }, glm::vec3{ 0.212f, 0.118f, 0.043f } },
            VertexFormat{ glm::vec3{ -0.15f,  0.0f,  0.15f }, glm::vec3{ 0.212f, 0.118f, 0.043f } },
            VertexFormat{ glm::vec3{ -0.15f,  1.0f, -0.15f }, glm::vec3{ 0.212f, 0.118f, 0.043f } },
            VertexFormat{ glm::vec3{ -0.15f,  1.0f,  0.15f }, glm::vec3{ 0.212f, 0.118f, 0.043f } },
            VertexFormat{ glm::vec3{ 0.15f,  0.0f, -0.15f }, glm::vec3{ 0.212f, 0.118f, 0.043f } },
            VertexFormat{ glm::vec3{ 0.15f,  0.0f,  0.15f }, glm::vec3{ 0.212f, 0.118f, 0.043f } },
            VertexFormat{ glm::vec3{ 0.15f,  1.0f, -0.15f }, glm::vec3{ 0.212f, 0.118f, 0.043f } },
            VertexFormat{ glm::vec3{ 0.15f,  1.0f,  0.15f }, glm::vec3{ 0.212f, 0.118f, 0.043f } },
            // crown
            VertexFormat{ glm::vec3{ -0.5f,  0.9f, -0.5f }, glm::vec3{ 0.122f, 0.369f, 0.059f } },
            VertexFormat{ glm::vec3{ -0.5f,  0.9f,  0.5f }, glm::vec3{ 0.122f, 0.369f, 0.059f } },
            VertexFormat{ glm::vec3{ -0.5f,  1.8f, -0.5f }, glm::vec3{ 0.122f, 0.369f, 0.059f } },
            VertexFormat{ glm::vec3{ -0.5f,  1.8f,  0.5f }, glm::vec3{ 0.122f, 0.369f, 0.059f } },
            VertexFormat{ glm::vec3{ 0.5f,  0.9f, -0.5f }, glm::vec3{ 0.122f, 0.369f, 0.059f } },
            VertexFormat{ glm::vec3{ 0.5f,  0.9f,  0.5f }, glm::vec3{ 0.122f, 0.369f, 0.059f } },
            VertexFormat{ glm::vec3{ 0.5f,  1.8f, -0.5f }, glm::vec3{ 0.122f, 0.369f, 0.059f } },
            VertexFormat{ glm::vec3{ 0.5f,  1.8f,  0.5f }, glm::vec3{ 0.122f, 0.369f, 0.059f } },
        };

        vector<unsigned int> indices
        {
            4, 6, 5, 7, 3, 6, 2, 4, 0, 5, 1, 3, 0, 2,
            0xFF,
            12, 14, 13, 15, 11, 14, 10, 12, 8, 13, 9, 11, 8, 10,
        };

        meshes["tree"] = new Mesh("tree");
        meshes["tree"]->InitFromData(vertices, indices);
        meshes["tree"]->SetDrawMode(GL_TRIANGLE_STRIP);
    }

    // Define circle shape
    {
        glm::vec3 center{ 0.0f, 0.0f, 0.0f };
        GLfloat radius = 1.0f;
        glm::vec3 color{ 0.196f, 0.659f, 0.322f };
        glm::vec3 normal{ 0.2f, 0.8f, 0.6f };
        int slices = 32; // minimum 3

        vector<VertexFormat> vertices{ VertexFormat{center, color, normal} };
        vector<unsigned int> indices{ 0 };

        const GLfloat step = static_cast<GLfloat>(2 * M_PI / slices);
        for (int i = 0; i <= slices; ++i)
        {
            const GLfloat angle_rad = i * step;
            vertices.emplace_back(VertexFormat{ {center.x + radius * cosf(angle_rad), 0.0f, center.z + radius * sinf(angle_rad)}, color, normal });
            indices.push_back(i + 1);
        }

        // Create circle
        meshes["circle"] = new Mesh("circle");
        meshes["circle"]->InitFromData(vertices, indices);
        meshes["circle"]->SetDrawMode(GL_TRIANGLE_FAN);
    }

    player_pos = glm::vec3{ -16.5f, 0.0f, 1.0f };
    player_dir = glm::vec3{ -1.0f, 0.0f, 0.0f };
    cpu_pos = player_pos + glm::vec3_forward * 0.5f;
    next_point = 14;
}

void Tema2::FrameStart()
{
    // Clears the color buffer (using the previously set color) and depth buffer
    glClearColor(0.529f, 0.808f, 0.922f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::ivec2 resolution = window->GetResolution();

    // Sets the screen area where to draw
    glViewport(0, 0, resolution.x, resolution.y);
}


void Tema2::Update(float deltaTimeSeconds)
{
    // Player movement
    glm::vec3 camera_pos = player_pos + glm::vec3 { 0.8f, 0.5f, 0.0f };
    glm::quat player_rotation = glm::quatLookAt(player_dir, glm::vec3_up);
    // GetSceneCamera()->SetPositionAndRotation(camera_pos, player_rotation);
    // GetSceneCamera()->Move

    // CPU movement
    glm::vec3 path = cpu_track[next_point] - cpu_pos;
    glm::vec3 cpu_dir = glm::normalize(path);
    cpu_pos += cpu_dir * cpu_speed * deltaTimeSeconds;

    if (glm::length(path) < 0.5f)
        next_point = (next_point - 1) % cpu_track.size();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glLineWidth(2.0f);

    // Draw the circle
    RenderMesh(meshes["circle"], shaders["VertexColor"], glm::vec3(0.0f, -0.01f, 0.0f), glm::vec3(100.0f));

    // Draw the track
    RenderMesh(meshes["track"], shaders["VertexColor"], glm::vec3(0.0f), glm::vec3(1.0f));
    RenderMesh(meshes["start"], shaders["VertexColor"], glm::vec3(0.0f, 0.01f, 0.0f), glm::vec3(1.0f));

    // std::cout << glm::to_string(GetSceneCamera()->m_transform->GetWorldRotation()) << '\n';

    glCullFace(GL_BACK);

    // Draw cars
    RenderMesh(meshes["cpu"], shaders["VertexColor"], cpu_pos, glm::vec3(1.0f));

    {
        glm::mat4 model{ 1.0f };
        model = glm::translate(model, player_pos);
        // model = glm::rotate(model, 3.14f / 2.0f, glm::vec3_up);
        model = glm::translate(model, glm::vec3{ 0.0f, -5.5f, 0.0f });
        model *= glm::toMat4(player_rotation);
        model = glm::translate(model, glm::vec3 { 0.0f, 5.5f, 0.0f });
        RenderMesh(meshes["player"], shaders["VertexColor"], model);
    }

    std::cout << glm::degrees(glm::acosf(glm::dot(player_dir, glm::vec3_left))) << '\n';

    // Draw vegetation
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFF);
    for (const glm::vec3 &pos : trees_pos)
        RenderMesh(meshes["tree"], shaders["VertexColor"], pos, glm::vec3(1.0f));
    glDisable(GL_PRIMITIVE_RESTART);

    glDisable(GL_CULL_FACE);
}


void Tema2::FrameEnd()
{
    // DrawCoordinateSystem();
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Tema2::OnInputUpdate(float deltaTime, int mods)
{
    if (window->KeyHold(GLFW_KEY_W))
        player_pos += player_dir * cpu_speed * deltaTime;
    if (window->KeyHold(GLFW_KEY_S))
        player_pos -= player_dir * cpu_speed * deltaTime;
    if (window->KeyHold(GLFW_KEY_A))
        player_dir -= glm::normalize(glm::cross(player_dir, glm::vec3_up)) * cpu_speed * deltaTime;
    if (window->KeyHold(GLFW_KEY_D))
        player_dir += glm::normalize(glm::cross(player_dir, glm::vec3_up)) * cpu_speed * deltaTime;
    player_dir = glm::normalize(player_dir);
}


void Tema2::OnKeyPress(int key, int mods)
{
    // Add key press event
}


void Tema2::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Tema2::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Tema2::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Tema2::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Tema2::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
    // Add mouse scroll event
}


void Tema2::OnWindowResize(int width, int height)
{
    // Add window resize event
}
