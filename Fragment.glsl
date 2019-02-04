#version 440
#//////////////////////////////////////////////////////////


in vec3 frag_color;
in vec2 texture_coord_geo;
in vec3 Normal;
in vec3 fragPos;

out vec4 fragment_color;

uniform sampler2D texture_sampler;

#//////////////////////////////////////////////////////////

vec3 lightpos = vec3(0.0f, 1.0f, 4.0f);
vec3 ambient = vec3(1.0f, 1.0f, 1.0f) * 0.1f;



vec3 normal = normalize(Normal);
vec3 lightDir = normalize(lightpos-fragPos);
float diff = max(dot(normal, lightDir), 0.0);
vec3 diffuse = diff*vec3(1.0f, 1.0f, 1.0f);



vec3 cameraPos = vec3(2, 2, 4);
float materialShininess = 5.0f;

vec3 reflectionVector = reflect(-lightDir,normal);
vec3 surfaceToCamera = normalize(cameraPos - fragPos);
float cosAngle = max(0.0, dot(surfaceToCamera, reflectionVector));
float specularCoefficient = pow(cosAngle, materialShininess);
vec3 specular = specularCoefficient*  vec3(0.5f, 0.5f, 0.5f)  *vec3(1.0f, 1.0f, 1.0f);

vec3 light = ambient*0.5f+diffuse*0.5f+specular*3.0f;
#//////////////////////////////////////////////////////////

void main () 
{
	fragment_color = vec4(frag_color, 1.0f) * vec4(light,1.0);
}