#include "lab_m1/Tema2/Tema2.h"

#include <algorithm> // sort
#include <iomanip>   // std::setw & std::setfill
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>

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

#define CPU_CAR(index) "cpu" + std::to_string(index)
#define next_float() (float) (rand()) / (float) RAND_MAX
#define SECTOR1_START_SEGMENT 0
#define SECTOR1_END_SEGMENT 14
#define SECTOR2_START_SEGMENT 15
#define SECTOR2_END_SEGMENT 34
#define SECTOR3_START_SEGMENT 35
#define SECTOR3_END_SEGMENT 57
#define PLAYER_NUMBER 31

static vector<glm::vec3> trees_pos{};
static vector<glm::vec3> track_pois{};
static vector<std::pair<glm::vec3, glm::vec3>> track_segments{};
static float camera_offset_xz{ 1.0f };
static float camera_offset_y{ 0.5f };
static glm::vec3 player_pos{};
static glm::vec3 player_dir = glm::vec3_left;
static float player_speed{ 4.9f };
static float player_turn_speed{ 90.0f };
static float initial_player_rotation{ -90.0f };
static float frontwheel_angle{ 0.0f };
static bool piua{ true };
static bool wireframe{ false };
static float track_width{ 2.0f };
static float half_track_width = track_width * 0.5f;
static unsigned int player_curr_segment = SECTOR3_END_SEGMENT;
static float car_width = 0.20f;
static float car_length = 0.50f;
static gfxc::Camera* main_camera = nullptr;
static gfxc::Camera* minimap = nullptr;


struct seconds_t {
    float value;
};

// ostream operator for your type:
std::ostream& operator<<(std::ostream& os, const seconds_t& v) {
    // convert to milliseconds
    int ms = static_cast<int>(v.value * 1000.0f);

    int h = ms / (1000 * 60 * 60);
    ms -= h * (1000 * 60 * 60);

    int m = ms / (1000 * 60);
    ms -= m * (1000 * 60);

    int s = ms / 1000;
    ms -= s * 1000;

    return os << std::setfill('0') << std::setw(2) << h << ':' << std::setw(2) << m
        << ':' << std::setw(2) << s << '.' << std::setw(3) << ms << std::setfill(' ');
}


float ProjectPointLine_RH(glm::vec3& out_projection, const glm::vec3& l1, const glm::vec3& l2, const glm::vec3& p);
void NotifyNewLap(unsigned int car_number);

namespace m1
{
    enum class Sector : unsigned short
    {
        SECTOR_1 = 0,
        SECTOR_2,
        SECTOR_3,
        OUT_LAP
    };

    struct Telemetrics
    {
        // Number the car is racing with.
        unsigned int car_number;

        // Absolute segment position where car is located, irrespective of whether it is going forwards or backwards.
        unsigned int curr_segment{ SECTOR3_END_SEGMENT };

        // Last segment position in a lap where the car advanced, so only counting forward movement valid for a lap.
        unsigned int last_segment{ SECTOR3_END_SEGMENT };

        // Sector in which the car currently is.
        Sector sector{ Sector::OUT_LAP };

        Telemetrics(unsigned int car_number) : car_number(car_number) {}

        static Sector LocateInSector(unsigned int segment)
        {
            Sector s = Sector::SECTOR_1;

            if (segment >= SECTOR2_START_SEGMENT && segment <= SECTOR2_END_SEGMENT)
                s = Sector::SECTOR_2;
            else if (segment >= SECTOR3_START_SEGMENT && segment <= SECTOR3_END_SEGMENT)
                s = Sector::SECTOR_3;

            return s;
        }

        glm::vec3 LocateOnCenterLine(const glm::vec3 &car_pos)
        {
            glm::vec3 point_on_line{};
            float offset_in_line = ProjectPointLine_RH(point_on_line, track_pois[curr_segment], track_pois[(curr_segment + 1) % track_pois.size()], car_pos);

            // Went backwards
            if (offset_in_line < 0.0f)
            {
                unsigned int previous_segment = curr_segment;
                curr_segment = (curr_segment == 0) ? track_pois.size() - 1 : curr_segment - 1;
                ProjectPointLine_RH(point_on_line, track_pois[curr_segment], track_pois[previous_segment], car_pos);
            }
            // Advanced
            else if (offset_in_line > 1.0f)
            {
                last_segment = curr_segment;
                curr_segment = (curr_segment + 1) % track_pois.size();

                if (curr_segment > last_segment || last_segment == SECTOR3_END_SEGMENT)
                {
                    if (last_segment == SECTOR1_END_SEGMENT)
                        sector = Sector::SECTOR_2;
                    else if (last_segment == SECTOR2_END_SEGMENT)
                        sector = Sector::SECTOR_3;
                    else if (last_segment == SECTOR3_END_SEGMENT)
                    {
                        NotifyNewLap(car_number);
                        sector = Sector::SECTOR_1;
                    }
                }

                unsigned int next_segment = (curr_segment + 1) % track_pois.size();
                ProjectPointLine_RH(point_on_line, track_pois[curr_segment], track_pois[next_segment], car_pos);
            }

            return point_on_line;
        }
    };

    struct Timetable
    {
    private:
        static int CompareTimes(const glm::vec3& lhs, const glm::vec3& rhs)
        {
            float s1 = lhs[(unsigned short)Sector::SECTOR_1];
            float s2 = lhs[(unsigned short)Sector::SECTOR_2];
            float s3 = lhs[(unsigned short)Sector::SECTOR_3];
            float lt = s1 + s2 + s3;

            s1 = rhs[(unsigned short)Sector::SECTOR_1];
            s2 = rhs[(unsigned short)Sector::SECTOR_2];
            s3 = rhs[(unsigned short)Sector::SECTOR_3];
            float rt = s1 + s2 + s3;

            if (lt < rt)
                return -1;
            if (lt > rt)
                return 1;
            return 0;
        }

        struct
        {
            bool operator()(const pair<unsigned int, pair<unsigned int, glm::vec3>>& lhs, const pair<unsigned int, pair<unsigned int, glm::vec3>>& rhs) const
            {
                // == -1 means lhs time < rhs time. this should prioritize faster times.
                bool test = CompareTimes(lhs.second.second, rhs.second.second) == -1;
                // std::cout << lhs.first << ' ' << test << ' ' << rhs.first << '\n';
                return test;
            }
        } BestTimesSorter;

    public:
        // Map from car number to list of lap times, stored in array of 3 floats for 3 sectors (using glm::vec3)
        unordered_map<unsigned int, vector<glm::vec3>> sector_times{};
        unordered_map<unsigned int, pair<unsigned int, glm::vec3>> best_times{};

        Timetable(size_t reserved_cars)
        {
            sector_times.reserve(reserved_cars);
        }

        void RegisterCar(unsigned int car_number)
        {
            // Create vector (laps) for car, to be filled by Track()
            sector_times.emplace(car_number, vector<glm::vec3>{});
        }

        void TrackCar(unsigned int car_number, const Telemetrics &tele, float delta)
        {
            if (tele.sector == Sector::OUT_LAP || sector_times.find(car_number) == sector_times.end() || sector_times[car_number].empty())
                return;

            // back() gives us the lap in progress
            sector_times[car_number].back()[(unsigned short)tele.sector] += delta;
        }

        void StartLap(unsigned int car_number)
        {
            if (sector_times.find(car_number) == sector_times.end())
                return;

            if (!sector_times[car_number].empty())
            {
                const glm::vec3& times = sector_times[car_number].back();
                /*float s1 = times[(unsigned short)Sector::SECTOR_1];
                float s2 = times[(unsigned short)Sector::SECTOR_2];
                float s3 = times[(unsigned short)Sector::SECTOR_3];
                float laptime = s1 + s2 + s3;*/

                //const glm::vec3& curr_best = best_times[car_number].second;
                /*s1 = curr_best[(unsigned short)Sector::SECTOR_1];
                s2 = curr_best[(unsigned short)Sector::SECTOR_2];
                s3 = curr_best[(unsigned short)Sector::SECTOR_3];
                float besttime = s1 + s2 + s3;*/

                if (best_times.find(car_number) == best_times.end())
                {
                    // Create pair of (lap where best time measure, time measured)
                    best_times.emplace(car_number, make_pair(sector_times[car_number].size(), times));
                }
                else if (CompareTimes(times, best_times[car_number].second) == -1)
                {
                    best_times[car_number].first = sector_times[car_number].size();
                    best_times[car_number].second = times;
                }

                if (car_number == PLAYER_NUMBER)
                    std::cout << '\n';
            }
            sector_times[car_number].emplace_back(glm::vec3{ 0.0f });
        }

        void PrintHeader()
        {
            std::cout << "Lap      Sector 1        Sector 2        Sector 3        Lap time  \n";
            std::cout << "-------------------------------------------------------------------\n";
            // std::cout << "  1    00:00:00.000    00:00:00.000    00:00:00.000    00:00:00.000\n";
        }

        void PrintCar(unsigned int car_number)
        {
            if (car_number != PLAYER_NUMBER)
                return;

            if (sector_times.find(car_number) == sector_times.end() || sector_times[car_number].empty())
                return;

            const glm::vec3& times = sector_times[car_number].back();
            float s1 = times[(unsigned short)Sector::SECTOR_1];
            float s2 = times[(unsigned short)Sector::SECTOR_2];
            float s3 = times[(unsigned short)Sector::SECTOR_3];

            std::cout
                << std::setw(3) << sector_times[car_number].size()
                << "    "
                << seconds_t{ s1 }
                << "    "
                << seconds_t{ s2 }
                << "    "
                << seconds_t{ s3 }
                << "    "
                << seconds_t{ s1 + s2 + s3 }
            << '\t' << '\r' << std::flush;
            ;
        }

        void PrintFinalResults()
        {
            std::cout << std::flush;
            std::cout << "\n\nRace end - results\n\n";
            std::cout << "Lap      Car      Sector 1        Sector 2        Sector 3        Lap time  \n";
            std::cout << "----------------------------------------------------------------------------\n";
            // std::cout << "  1        1    00:00:00.000    00:00:00.000    00:00:00.000    00:00:00.000\n";

            // Sort results from fastest to slowest
            vector<pair<unsigned int, pair<unsigned int, glm::vec3>>> times(best_times.begin(), best_times.end());
            std::sort(times.begin(), times.end(), BestTimesSorter);

            for (const auto& best : best_times)
            {
                unsigned int car = best.first;
                unsigned int lap = best.second.first;
                const glm::vec3& times = best.second.second;
                float s1 = times[(unsigned short)Sector::SECTOR_1];
                float s2 = times[(unsigned short)Sector::SECTOR_2];
                float s3 = times[(unsigned short)Sector::SECTOR_3];

                std::cout
                    << std::setw(3) << lap
                    << "    "
                    << std::setw(3) << car
                    << "    "
                    << seconds_t{ s1 }
                    << "    "
                    << seconds_t{ s2 }
                    << "    "
                    << seconds_t{ s3 }
                    << "    "
                    << seconds_t{ s1 + s2 + s3 }
                << '\n';
                ;
            }
        }
    };

    struct Cpu {
        glm::vec3 pos{};
        glm::vec3 dir{};
        unsigned int next_point{};
        float speed{};
        glm::vec3 target{};
        Telemetrics tele;

        Cpu(unsigned int car_number, const glm::vec3& pos, int next_point, float speed)
            : tele(car_number), pos(pos), next_point(next_point), speed(speed)
        {
            NextTarget();
        }

        void Update(float dt)
        {
            pos += dir * speed * dt;
            tele.LocateOnCenterLine(pos);

            if (glm::length(target - pos) < 0.5f)
            {
                next_point = (next_point + 1) % track_pois.size();
                NextTarget();
            }
        }

        // Instead of going straight on the middle of the track,
        // allow some randomness in choosing the direction
        void NextTarget()
        {
            // Get random value in [0, 1]
            float random = next_float();

            // The direction of the next track segment -- usually perpendicular to the direction of travel
            glm::vec3 segment_dir = glm::normalize(track_segments[next_point].first - track_segments[next_point].second);

            // Choose to offset either left or right for more variation
            if (random < 0.5f)
                segment_dir *= -1.0f;

            // After using the random for direction choice, extend it to track width length, but restrict to maximum quarter deviation.
            // Now we have random value in [0, track_width / 4]
            random *= track_width * 0.25f;

            // Find next path to follow by going randomly to the left or right from the centered line
            target = track_pois[next_point] + (segment_dir * random);
            dir = glm::normalize(target - pos);
        }
    };
}

static vector<Cpu> cpu_cars{};
static Timetable timetable{ 4 };
static Telemetrics player_telemetrics{ PLAYER_NUMBER };


void NotifyNewLap(unsigned int car_number)
{
    // std::cout << 2 << '\n';
    timetable.StartLap(car_number);
}


void Tema2::AddCpu(const glm::vec3& pos, int next_point, float speed, const glm::vec3& color)
{
    unsigned int index = (unsigned int) cpu_cars.size();

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

    cpu_cars.emplace_back(Cpu{ index, pos, next_point, speed });
    timetable.RegisterCar(index);

    std::cout << "Added " CPU_CAR(index) << '\n';
}


Tema2::Tema2()
{
}


Tema2::~Tema2()
{
}

float ProjectPointLine_RH(glm::vec3& out_projection, const glm::vec3& l1, const glm::vec3& l2, const glm::vec3& p)
{
    float length = glm::length(l2 - l1);
    float u = (
        (p.x - l1.x) * (l2.x - l1.x)
        + (p.y - l1.y) * (l2.y - l1.y)
        + (p.z - l1.z) * (l2.z - l1.z)
        ) / (length * length);
    out_projection.x = l1.x + u * (l2.x - l1.x);
    out_projection.y = l1.y + u * (l2.y - l1.y);
    out_projection.z = l1.z + u * (l2.z - l1.z);
    return u;
}

bool IsSpheresCollision(const glm::vec3& a_pos, float a_radius, const glm::vec3& b_pos, float b_radius)
{
    return glm::length(b_pos - a_pos) < a_radius + b_radius;
}

bool IsCarsCollision(const glm::vec3& a_pos, const glm::vec3& a_dir, const glm::vec3& b_pos, const glm::vec3& b_dir)
{
    float offset = (car_length - car_width) * 0.5f;
    const glm::vec3 a_start = a_pos - a_dir * offset;
    const glm::vec3 a_end = a_pos + a_dir * offset;

    const glm::vec3 b_start = b_pos - b_dir * offset;
    const glm::vec3 b_end = b_pos + b_dir * offset;

    const glm::vec3 v0 = b_start - a_start;
    const glm::vec3 v1 = b_end - a_start;
    const glm::vec3 v2 = b_start - a_end;
    const glm::vec3 v3 = b_end - a_end;

    const float d0 = glm::dot(v0, v0);
    const float d1 = glm::dot(v1, v1);
    const float d2 = glm::dot(v2, v2);
    const float d3 = glm::dot(v3, v3);

    glm::vec3 best_a{};
    if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1)
        best_a = a_end;
    else
        best_a = a_start;

    glm::vec3 best_b{};
    float u = ProjectPointLine_RH(best_b, best_a, b_start, b_end);

    /*if (u < 0.0f || u > 1.0f)
    {
        std::cout << "CACAA\n";
        return false;
    }*/

    u = ProjectPointLine_RH(best_a, best_b, a_start, a_end);

    /*if (u < 0.0f || u > 1.0f)
    {
        std::cout << "CACAA\n";
        return false;
    }*/

    std::cout << "Player pos: " << glm::to_string(a_pos) << '\n';
    std::cout << "Best A: " << glm::to_string(best_a) << '\n';

    std::cout << "CPU pos: " << glm::to_string(b_pos) << '\n';
    std::cout << "Best B: " << glm::to_string(best_b) << '\n';

    std::cout << "length " << glm::length(best_b - best_a) << " < " << car_width << '\n';

    return glm::length(best_b - best_a) < car_width;
}


void Tema2::Init()
{
    srand(time(nullptr));

    // Load and generate track
    {
        std::cout << "Adding track...\n";
        std::ifstream in("sample_track_reversed.txt");

        vector<VertexFormat> vertices{};
        vector<unsigned int> indices{};

        GLfloat x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
        int i = 0;
        while (in >> x1 >> y1 >> x2 >> y2)
        {
            //std::cout << x1 << ' ' << y1 << '\n';
            //std::cout << x2 << ' ' << y2 << '\n';

            glm::vec3 p1 { x1, 0.0f, y1 };
            glm::vec3 p2 { x2, 0.0f, y2 };

            glm::vec3 segment = p2 - p1;
            glm::vec3 side_dir = glm::normalize(glm::cross(segment, glm::vec3_up));

            // i = interior, e = exterior
            glm::vec3 i1 = p1 + half_track_width * side_dir;
            glm::vec3 e1 = p1 - half_track_width * side_dir;

            glm::vec3 i2 = p2 + half_track_width * side_dir;
            glm::vec3 e2 = p2 - half_track_width * side_dir;

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

        /*for (int idx : indices)
            std::cout << idx << ' ';
        std::cout << '\n';*/

        meshes["track"] = new Mesh("track");
        meshes["track"]->InitFromData(vertices, indices);
        meshes["track"]->SetDrawMode(GL_TRIANGLE_STRIP);

        // Create start/finish line somewhere
        const glm::vec3& istart = track_segments[SECTOR1_START_SEGMENT].first;
        const glm::vec3& estart = track_segments[SECTOR1_START_SEGMENT].second;
        const glm::vec3& iprev = track_segments[SECTOR3_END_SEGMENT].first;
        const glm::vec3& dir = glm::normalize(iprev - istart);
        const glm::vec3& iend = istart + 0.2f * dir;
        const glm::vec3& eend = estart + 0.2f * dir;

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

    player_pos = track_pois[SECTOR1_START_SEGMENT] + glm::vec3{ 1.0f, 0.0f, -0.5f };
    timetable.RegisterCar(PLAYER_NUMBER);

    // Create CPU-controlled car
    AddCpu(player_pos + glm::vec3{ -0.25f, 0.0f, 0.8f }, 3, 4.9f, glm::vec3{ 0.671f, 0.324f, 0.245f }); // left side from player
    AddCpu(player_pos - glm::vec3_left * 1.0f, 3, 4.6f, glm::vec3{ 0.724f, 0.871f, 0.745f }); // one row behind
    AddCpu(player_pos + glm::vec3{ 0.75f, 0.0f, 0.8f }, 3, 5.0f, glm::vec3{ 0.824f, 0.871f, 0.145f }); // left side from player, one row behind

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
    std::cout << "Press P to start race!\n";
    std::cout << "Controls:\n";
    std::cout << "\tW\t\taccelerate\n";
    std::cout << "\tS\t\tbreak\n";
    std::cout << "\tA\t\tturn left\n";
    std::cout << "\tS\t\tturn right\n";
    std::cout << "\tSHIFT + WASD\thold while racing to see how Max Verstappen feels\n";
    std::cout << "\tMOUSE2 + WASD\tfree roam camera\n";

    main_camera = GetSceneCamera();
    glm::ivec2 resolution = window->GetResolution();

    minimap = new gfxc::Camera();
    //minimap->SetOrthographic(0.0f, 300.0f, 0.0f, 250.0f, 0.1f, 10.0f);
    //minimap->SetOrthographic(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 50.0f);
    //minimap->m_transform->SetWorldPosition(player_pos);
    //minimap->m_transform->SetWorldRotation(glm::vec3(-0, 0, 0));
    minimap->SetPerspective(60, window->props.aspectRatio, 0.01f, 200);
    minimap->m_transform->SetWorldPosition(player_pos + glm::vec3_up * 5.0f);
    minimap->m_transform->SetWorldRotation(glm::vec3(-90.0f, 90.0f, 0.0f));
    minimap->Update();

    std::cout << '\n';
    std::cout << '\n';

    timetable.PrintHeader();
}

void Tema2::Render()
{
    /*glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);*/

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
    for (const glm::vec3& pos : trees_pos)
        RenderMesh(meshes["tree"], shaders["VertexColor"], pos, glm::vec3(1.0f));
    glDisable(GL_PRIMITIVE_RESTART);

    //glDisable(GL_CULL_FACE);
}

void Tema2::FrameStart()
{
    // Clears the color buffer (using the previously set color) and depth buffer
    glClearColor(0.529f, 0.808f, 0.922f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
}

void Tema2::Update(float deltaTimeSeconds)
{
    // CPU movement
    if (!piua)
        for (int i = 0; i < cpu_cars.size(); ++i)
        {
            timetable.TrackCar(i, cpu_cars[i].tele, deltaTimeSeconds);
            bool can_update = true;
            for (int j = i + 1; j < cpu_cars.size(); ++j)
                if (IsSpheresCollision(cpu_cars[i].pos, car_width, cpu_cars[j].pos, car_width))
                    can_update = false;
            if (can_update)
                cpu_cars[i].Update(deltaTimeSeconds);
        }

    // Sets the screen area where to draw main camera
    glm::ivec2 resolution = window->GetResolution();
    glViewport(0, 0, resolution.x, resolution.y);
    gfxc::Camera* mainc = GetSceneCamera();

    int loc = shaders["VertexColor"]->GetUniformLocation("car_pos");
    glUniform3fv(loc, 1, glm::value_ptr(player_pos));

    loc = shaders["VertexColor"]->GetUniformLocation("scale_factor");
    // glUniform1f(loc, 0.005f);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    Render();
    glDisable(GL_CULL_FACE);

    // DrawCoordinateSystem();

    // Minimap
    glViewport(0, 0, 400, 200);

    SetSceneCamera(minimap);
    GetSceneCamera()->SetPosition(player_pos + glm::vec3_up * 8.0f);
    Render();
    SetSceneCamera(mainc);
}


void Tema2::FrameEnd()
{
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */

//static float thetime = 0.0f;

void Tema2::OnInputUpdate(float deltaTime, int mods)
{
    if (piua)
        return;

    /*assert(main_camera == GetSceneCamera());
    thetime += deltaTime;*/

    // Update timetable
    timetable.TrackCar(PLAYER_NUMBER, player_telemetrics, deltaTime);
    timetable.PrintCar(PLAYER_NUMBER);
    //std::cout << "segment " << player_telemetrics.curr_segment << '\n';
    //std::cout << "sector" << (unsigned short)player_telemetrics.sector<< '\n';

    if (window->MouseHold(GLFW_MOUSE_BUTTON_2))
        return;

    for (const Cpu& cpu : cpu_cars)
    //Cpu& cpu = cpu_cars[1];
        // if (IsCarsCollision(player_pos, player_dir, cpu.pos, cpu.dir))
        if (IsSpheresCollision(player_pos, car_width, cpu.pos, car_width))
        {
            // std::cout << thetime << " -> Player collides with another car\n";
            return;
        }

    float actual_speed = player_speed;
    float actual_turn_speed = player_turn_speed;
    glm::vec3 new_player_pos = player_pos;

    if (mods & GLFW_MOD_SHIFT)
    {
        actual_speed *= 2;
        actual_turn_speed *= 2;
    }

    if (window->KeyHold(GLFW_KEY_W))
        new_player_pos += player_dir * actual_speed * deltaTime;
    if (window->KeyHold(GLFW_KEY_S))
        new_player_pos -= player_dir * actual_speed * deltaTime;
    if (window->KeyHold(GLFW_KEY_A))
    {
        float left_turn = actual_turn_speed * deltaTime;
        frontwheel_angle += left_turn;
        if (frontwheel_angle > 45.0f && false)
            frontwheel_angle = 45.0f;
    }
    if (window->KeyHold(GLFW_KEY_D))
    {
        float right_turn = -actual_turn_speed * deltaTime;
        frontwheel_angle += right_turn;
        if (frontwheel_angle < -45.0f && false)
            frontwheel_angle = -45.0f;
    }

    float angle_rad = RADIANS(initial_player_rotation + frontwheel_angle);
    player_dir = glm::vec3{ glm::sinf(angle_rad), 0.0f, glm::cosf(angle_rad) };

    glm::vec3 camera_pos = new_player_pos + (-player_dir * camera_offset_xz) + (glm::vec3_up * camera_offset_y);
    glm::quat camera_rotation = glm::quatLookAt(player_dir, glm::vec3_up);
    GetSceneCamera()->SetPositionAndRotation(camera_pos, camera_rotation);

    glm::vec3 point_on_line = player_telemetrics.LocateOnCenterLine(player_pos);
    //std::cout << "sector" << (unsigned short)player_telemetrics.sector << '\n';

    const float dist = glm::length(new_player_pos - point_on_line);
    if (dist > half_track_width)
    {
        // std::cout << "Player out of track bounds! Distance = " << dist << '\n';
    }
    else
        player_pos = new_player_pos;
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

    if (key == GLFW_KEY_ESCAPE)
        timetable.PrintFinalResults();
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
