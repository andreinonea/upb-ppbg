#version 330

// Input
// TODO(student): Get vertex attributes from each location
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_text_coord;
layout(location = 3) in vec3 v_color;

out vec3 f_color;

// Uniform properties
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform float running_time;

// Output
// TODO(student): Output values to fragment shader

void main()
{
    // TODO(student): Send output to fragment shader
    f_color = (v_normal + sin(running_time) + 2) / 4;

    // TODO(student): Compute gl_Position
    gl_Position = Projection * View * Model * vec4(v_position + sin(running_time), 1.0);

}
