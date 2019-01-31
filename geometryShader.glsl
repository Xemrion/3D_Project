#version 440

layout(triangles) in;
layout(triangle_strip, max_vertices=6) out;

#//////////////////////////////////////////////////////////
in vec3 color[];
out vec3 frag_color;

in vec2 texture_coord_vert[];
out vec2 texture_coord_geo;

in float ID[];

out vec3 Normal;
out vec3 fragPos;

in mat4 World[];
in mat4 GroundWorld[];
in mat4 View[];
in mat4 Projection[];

vec3 normal = cross((gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz), (gl_in[0].gl_Position.xyz - gl_in[2].gl_Position.xyz));

#//////////////////////////////////////////////////////////
void main()
{

	  for(int i=0; i<3; i++)
	  {
	  	if(ID[i] == 1.0)
		{
			gl_Position = Projection[i] * View[i] * World[i] * gl_in[i].gl_Position;

			frag_color = color[i];
			fragPos = vec3(World[i] * gl_in[i].gl_Position);
			texture_coord_geo = texture_coord_vert[i];
			Normal = vec3(World[i]*vec4(normal, 1.0f));

			EmitVertex();
		}
		else if(ID[i] == 2.0)
		{
			gl_Position = Projection[i] * View[i] * GroundWorld[i] * gl_in[i].gl_Position;

			frag_color = color[i];
			fragPos = vec3(GroundWorld[i] * gl_in[i].gl_Position);
			texture_coord_geo = texture_coord_vert[i];
			Normal = vec3(GroundWorld[i]*vec4(normal, 1.0f));

			EmitVertex();
		}
	  }
	  EndPrimitive();

	  for(int i=0; i<3; i++)
	  {
	  	if(ID[i] == 1.0)
		{
			gl_Position = Projection[i] * View[i] * GroundWorld[i] * (gl_in[i].gl_Position+vec4(-0.1f, 1.0f, -0.1f, 0.0f));

			frag_color = color[i];
			fragPos = vec3(GroundWorld[i] * (gl_in[i].gl_Position+vec4(0.1f, 1.0f, 0.1f, 0.0f)));
			texture_coord_geo = texture_coord_vert[i];
			Normal = vec3(GroundWorld[i]*vec4(normal, 1.0f));

			EmitVertex();
		}
	  }
	  EndPrimitive();
}  
