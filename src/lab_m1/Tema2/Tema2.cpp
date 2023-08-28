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
static vector<glm::vec3> track_pois{};
static vector<std::pair<glm::vec3, glm::vec3>> track_segments{};
static float camera_offset_xz{};
static float camera_offset_y{};
static glm::vec3 player_pos{};
static glm::vec3 player_dir{};
static float player_speed{};
static float player_turn_speed{};
static float initial_player_rotation{};
static float frontwheel_angle{};
static bool piua{};
static bool wireframe{};
static float track_width{};
static unsigned int start_line_idx{};

#define CPU_CAR(index) "cpu" + std::to_string(index)

namespace m1
{
    struct Cpu {
        glm::vec3 pos{};
        glm::vec3 dir{};
        unsigned int next_point{};
        float speed{};

        Cpu(const glm::vec3& pos, int next_point, float speed)
            : pos(pos), next_point(next_point), speed(speed)
        {
            dir = glm::vec3_left;
        }

        void Update(float dt)
        {
            glm::vec3 path = track_pois[next_point] - pos;
            dir = glm::normalize(path);
            pos += dir * speed * dt;

            if (glm::length(path) < 0.5f)
            {
                if (next_point == 0)
                    next_point = track_pois.size() - 1;
                else
                    next_point -= 1;
            }
        }
    };
}

static vector<Cpu> cpu_cars{};


void Tema2::AddCpu(const glm::vec3& pos, int next_point, float speed, const glm::vec3& color)
{
    int index = cpu_cars.size();

    vector<VertexFormat> vertices
    {
        VertexFormat{ glm::vec3{ -0.25f,  0.02f, -0.10f }, glm::vec3{ 1.0f } },
        VertexFormat{ glm::vec3{ -0.25f,  0.02f,  0.10f }, glm::vec3{ 1.0f } },
        VertexFormat{ glm::vec3{ -0.25f,  0.20f, -0.10f }, glm::vec3{ 0.0f } },
        VertexFormat{ glm::vec3{ -0.25f,  0.20f,  0.10f }, glm::vec3{ 0.0f } },
        VertexFormat{ glm::vec3{ 0.25f,  0.02f, -0.10f }, color },
        VertexFormat{ glm::vec3{ 0.25f,  0.02f,  0.10f }, color },
        VertexFormat{ glm::vec3{ 0.25f,  0.20f, -0.10f }, color },
        VertexFormat{ glm::vec3{ 0.25f,  0.20f,  0.10f }, color },
    };

    vector<unsigned int> indices
    {
        4, 6, 5, 7, 3, 6, 2, 4, 0, 5, 1, 3, 0, 2,
    };

    meshes[CPU_CAR(index)] = new Mesh(CPU_CAR(index));
    meshes[CPU_CAR(index)]->InitFromData(vertices, indices);
    meshes[CPU_CAR(index)]->SetDrawMode(GL_TRIANGLE_STRIP);

    cpu_cars.emplace_back(Cpu{ pos, next_point, speed });

    std::cout << "Added " CPU_CAR(index) << '\n';
}


Tema2::Tema2()
{
}


Tema2::~Tema2()
{
}


void Tema2::Init()
{
    camera_offset_xz = 1.0f;
    camera_offset_y = 0.5f;
    player_pos = glm::vec3{ -16.5f, 0.0f, 1.0f };
    player_dir = glm::vec3{ -1.0f, 0.0f, 0.0f };
    player_speed = 2.9f;
    player_turn_speed = 45.0f;
    initial_player_rotation = -90.0f;
    frontwheel_angle = 0.0f;
    piua = true;
    wireframe = false;
    track_width = 1.0f;
    start_line_idx = 14;

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

            glm::vec3 segment = p2 - p1;
            glm::vec3 side_dir = glm::normalize(glm::cross(segment, glm::vec3_up));

            // i = interior, e = exterior
            glm::vec3 i1 = p1 + track_width * side_dir;
            glm::vec3 e1 = p1 - track_width * side_dir;

            glm::vec3 i2 = p2 + track_width * side_dir;
            glm::vec3 e2 = p2 - track_width * side_dir;

            track_pois.push_back(p1);
            track_pois.push_back(p2);

            track_segments.emplace_back(std::make_pair(i1, e1));
            track_segments.emplace_back(std::make_pair(i2, e2));

            trees_pos.push_back(i1 + 1.0f * side_dir);
            trees_pos.push_back(e1 - 1.0f * side_dir);
            trees_pos.push_back(i2 + 1.0f * side_dir);
            trees_pos.push_back(e2 - 1.0f * side_dir);

            vertices.emplace_back(VertexFormat{ i1, glm::vec3 { 0.1f, 0.1f, 0.1f } });
            indices.push_back(i++);
            vertices.emplace_back(VertexFormat{ e1, glm::vec3 { 0.1f, 0.1f, 0.1f } });
            indices.push_back(i++);
            vertices.emplace_back(VertexFormat{ i2, glm::vec3 { 0.1f, 0.1f, 0.1f } });
            indices.push_back(i++);
            vertices.emplace_back(VertexFormat{ e2, glm::vec3 { 0.1f, 0.1f, 0.1f } });
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
        glm::vec3 istart = track_segments[start_line_idx].first;
        glm::vec3 estart = track_segments[start_line_idx].second;
        glm::vec3 inext = track_segments[start_line_idx - 1].first;
        glm::vec3 enext = track_segments[start_line_idx - 1].second;
        glm::vec3 dir = glm::normalize(inext - istart);
        glm::vec3 iend = istart + 0.2f * dir;
        glm::vec3 eend = estart + 0.2f * dir;

        vector<VertexFormat> sf_vertices
        {
            VertexFormat{ iend },
            VertexFormat{ eend },
            VertexFormat{ istart },
            VertexFormat{ estart },
        };
        vector<unsigned int> sf_indices { 0, 1, 2, 3 };

        meshes["start"] = new Mesh("start");
        meshes["start"]->InitFromData(sf_vertices, sf_indices);
        meshes["start"]->SetDrawMode(GL_TRIANGLE_STRIP);
    }

    // Create CPU-controlled car
    AddCpu(player_pos + glm::vec3_forward * 0.5f, 10, 2.9f, glm::vec3{ 0.671f, 0.324f, 0.245f }); // left side from player
    AddCpu(player_pos - glm::vec3_left * 1.0f, 10, 2.6f, glm::vec3{ 0.724f, 0.871f, 0.745f }); // one row behind
    AddCpu(player_pos + glm::vec3_forward * 0.5f - glm::vec3_left * 1.0f, 10, 3.0f, glm::vec3{ 0.824f, 0.871f, 0.145f }); // left side from player, one row behind

    // Create player-controlled car
    {
        vector<VertexFormat> vertices
        {
            VertexFormat{ glm::vec3{ -0.25f,  0.02f, -0.10f }, glm::vec3{ 1.0f } },
            VertexFormat{ glm::vec3{ -0.25f,  0.02f,  0.10f }, glm::vec3{ 1.0f } },
            VertexFormat{ glm::vec3{ -0.25f,  0.20f, -0.10f }, glm::vec3{ 0.0f } },
            VertexFormat{ glm::vec3{ -0.25f,  0.20f,  0.10f }, glm::vec3{ 0.0f } },
            VertexFormat{ glm::vec3{ 0.25f,  0.02f, -0.10f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ 0.25f,  0.02f,  0.10f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ 0.25f,  0.20f, -0.10f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
            VertexFormat{ glm::vec3{ 0.25f,  0.20f,  0.10f }, glm::vec3{ 0.145f, 0.518f, 0.871f } },
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

    // Do one update to center camera correctly
    piua = false;
    OnInputUpdate(0.0f, 0);
    piua = true;
}

void Tema2::FrameStart()
{
    // Clears the color buffer (using the previously set color) and depth buffer
    glClearColor(0.529f, 0.808f, 0.922f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::ivec2 resolution = window->GetResolution();

    // Sets the screen area where to draw
    glViewport(0, 0, resolution.x, resolution.y);

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
}

void Tema2::Update(float deltaTimeSeconds)
{
    // CPU movement
    if (! piua)
        for (Cpu& cpu : cpu_cars)
            cpu.Update(deltaTimeSeconds);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glLineWidth(2.0f);

    // Draw the circle
    RenderMesh(meshes["circle"], shaders["VertexColor"], glm::vec3(0.0f, -0.01f, 0.0f), glm::vec3(100.0f));

    // Draw the track
    RenderMesh(meshes["track"], shaders["VertexColor"], glm::vec3(0.0f), glm::vec3(1.0f));
    RenderMesh(meshes["start"], shaders["VertexColor"], glm::vec3(0.0f, 0.01f, 0.0f), glm::vec3(1.0f));

    glCullFace(GL_BACK);

    // Draw cars
    for (int i = 0; i < cpu_cars.size(); ++i)
    {
        glm::mat4 model{ 1.0f };
        model = glm::translate(model, cpu_cars[i].pos);
        model = glm::rotate(model, glm::sign(cpu_cars[i].dir.z) * glm::acosf(glm::dot(cpu_cars[i].dir, glm::vec3_left)), glm::vec3_up);
        RenderMesh(meshes[CPU_CAR(i)], shaders["VertexColor"], model);
    }

    {
        glm::mat4 model{ 1.0f };
        model = glm::translate(model, player_pos);
        model = glm::rotate(model, RADIANS(frontwheel_angle), glm::vec3_up);
        RenderMesh(meshes["player"], shaders["VertexColor"], model);
    }

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
    if (window->MouseHold(GLFW_MOUSE_BUTTON_2) || piua)
        return;

    if (window->KeyHold(GLFW_KEY_W))
        player_pos += player_dir * player_speed * deltaTime;
    if (window->KeyHold(GLFW_KEY_S))
        player_pos -= player_dir * player_speed * deltaTime;
    if (window->KeyHold(GLFW_KEY_A))
    {
        float left_turn = player_turn_speed * deltaTime;
        frontwheel_angle += left_turn;
        if (frontwheel_angle > 45.0f && false)
            frontwheel_angle = 45.0f;
    }
    if (window->KeyHold(GLFW_KEY_D))
    {
        float right_turn = -player_turn_speed * deltaTime;
        frontwheel_angle += right_turn;
        if (frontwheel_angle < -45.0f && false)
            frontwheel_angle = -45.0f;
    }

    float angle_rad = RADIANS(initial_player_rotation + frontwheel_angle);
    player_dir = glm::vec3{ glm::sinf(angle_rad), 0.0f, glm::cosf(angle_rad) };

    glm::vec3 camera_pos = player_pos + (-player_dir * camera_offset_xz) + (glm::vec3_up * camera_offset_y);
    glm::quat camera_rotation = glm::quatLookAt(player_dir, glm::vec3_up);
    GetSceneCamera()->SetPositionAndRotation(camera_pos, camera_rotation);
}


void Tema2::OnKeyPress(int key, int mods)
{
    // Add key press event
    if (key == GLFW_KEY_P)
        piua = !piua;

    if (key == GLFW_KEY_W && mods & GLFW_MOD_CONTROL)
    {
        wireframe = !wireframe;
    }
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
