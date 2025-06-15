#version 410


uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
//uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 force;
layout(location = 2) in vec3 velocity;

out vec3 fragPosition;
out vec3 fragForce;
out vec3 fragVelocity;
out vec3 worldPosPosition;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);
    
    fragPosition = (modelMatrix * vec4(position, 1)).xyz;
    fragForce = force;
    fragVelocity = velocity;
    worldPosPosition = position;
}
