#version 410

#define RENDERMODE_PLAIN 0
#define RENDERMODE_FORCE 1
#define RENDERMODE_VELOCITY 2

uniform int renderMode;
uniform float magnitudeCutoff;

in vec3 fragPosition;
in vec3 fragForce;
in vec3 fragVelocity;

layout(location = 0) out vec4 fragColor;

vec3 vectorToColor(vec3 v, float maxLength) {
    float len = clamp(length(v), 0.0f, maxLength) / maxLength;
    
    // we want to interpolate the colour evenly going through blue, green, yellow, red
    len *= 3.0f;
    if (len <= 1.0f) {
        return mix(vec3(0, 0, 1), vec3(0, 1, 0), len);
    } else if (len <= 2.0f) {
        return mix(vec3(0, 1, 0), vec3(1, 1, 0), len-1);
    } else {
        return mix(vec3(1, 1, 0), vec3(1, 0, 0), len-2);
    }
}

void main()
{
    vec4 plainColor = vec4(0.8, 0.8, 0.8, 1.0);

    if (renderMode == RENDERMODE_PLAIN) {
        fragColor = plainColor;
    } else if (renderMode == RENDERMODE_FORCE) { 
        fragColor = vec4(vectorToColor(fragForce, magnitudeCutoff), 1);
    } else if (renderMode == RENDERMODE_VELOCITY) {
        fragColor = vec4(vectorToColor(fragVelocity, magnitudeCutoff), 1);
    } else {
        fragColor = plainColor;
    } 
}
