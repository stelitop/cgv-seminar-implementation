#version 410

uniform vec3 kd;

in vec3 fragPosition;
//in vec3 fragNormal;
//in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    //vec3 normal = normalize(fragNormal);
    fragColor = vec4(kd, 1);
}
