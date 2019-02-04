#version 440
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_color;
layout(location = 2) in vec2 texture_coord;
layout(location = 3) in float OBJid;



#//////////////////////////////////////////////////////////
out vec3 color;
out vec2 texture_coord_vert;
out float ID;

uniform mat4 world;
uniform mat4 groundWorld;
uniform mat4 view;
uniform mat4 projection;

out mat4 World;
out mat4 GroundWorld;
out mat4 View;
out mat4 Projection;

#//////////////////////////////////////////////////////////
void main() {
	gl_Position = vec4(vertex_position, 1.0);
	color = vertex_color;
	World = world;
	GroundWorld = groundWorld;
	View = view;
	Projection = projection;
	ID = OBJid;
	texture_coord_vert = texture_coord;
}