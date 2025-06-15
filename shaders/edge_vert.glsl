#version 410

#define BOUNDARY_EDGE 0.0f
#define MOUNTAIN_EDGE 1.0f
#define VALLEY_EDGE 2.0f
#define FACET_EDGE 3.0f

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
//uniform mat3 normalModelMatrix;

layout(location = 0) in vec4 position;

out vec3 fragPosition;
out vec3 fragColor;
out float edgeType;

void main()
{
    gl_Position = mvpMatrix * vec4(position.xyz, 1);
    fragPosition    = (modelMatrix * vec4(position.xyz, 1)).xyz;
    edgeType = position.w;
    if (abs(position.w - BOUNDARY_EDGE) < 1e-4) {
        fragColor = vec3(0, 0, 0);
    } else if (abs(position.w - MOUNTAIN_EDGE) < 1e-4) {
        fragColor = vec3(1, 0, 0);
    } else if (abs(position.w - VALLEY_EDGE) < 1e-4) {
        fragColor = vec3(0, 0, 1);
    } else if (abs(position.w - FACET_EDGE) < 1e-4) {
        fragColor = vec3(1, 1, 0);
    } else {
        fragColor = vec3(0);
    }
}
