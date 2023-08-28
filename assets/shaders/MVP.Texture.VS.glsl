#version 330

// Input
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;
layout(location = 3) in vec3 v_color;

// Uniform properties
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform vec3 car_pos;
uniform float scale_factor;

// Output
out vec3 frag_normal;
out vec3 frag_color;
out vec2 tex_coord;


void main()
{
    vec3 vpos = v_position;
    float dist_to_car = length(car_pos - vpos);
    vpos.y -= dist_to_car * dist_to_car * scale_factor;
    frag_normal = v_normal;
    frag_color = v_color;
    tex_coord = v_texture_coord;
    gl_Position = Projection * View * Model * vec4(vpos, 1.0);
}
