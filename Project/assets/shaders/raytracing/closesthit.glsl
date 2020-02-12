#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec2 TexCoord;
};

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

layout(location = 0) rayPayloadInNV RayPayload rayPayload;

hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 2, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
} cam;
layout(binding = 3, set = 0) buffer Vertices { Vertex v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;

void main()
{
	ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

	Vertex v0 = vertices[gl_InstanceCustomIndexNV][gl_PrimitiveID];
	Vertex v1 = vertices[gl_InstanceCustomIndexNV][gl_PrimitiveID + 1];
	Vertex v2 = vertices[gl_InstanceCustomIndexNV][gl_PrimitiveID + 2];

	// Interpolate normal
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 normal = normalize(v0.Normal * barycentricCoords.x + v1.Normal * barycentricCoords.y + v2.Normal * barycentricCoords.z);

	// Basic lighting
	vec3 lightVector = normalize(cam.lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.6);
	rayPayload.color = vec3(1.0f, 0.0f, 0.0f) * vec3(dot_product);
	rayPayload.distance = gl_RayTmaxNV;
	rayPayload.normal = normal;

	// Objects with full white vertex color are treated as reflectors
	//rayPayload.reflector = ((v0.color.r == 1.0f) && (v0.color.g == 1.0f) && (v0.color.b == 1.0f)) ? 1.0f : 0.0f; 
	rayPayload.reflector = 1.0f;
}
