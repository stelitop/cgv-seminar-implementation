#version 410

#define FACET_EDGE 3.0f

uniform int showFacetEdges;

in vec3 fragPosition;
in vec3 fragColor;
in float edgeType;

layout(location = 0) out vec4 finalColor;

void main()
{
    if (abs(edgeType- FACET_EDGE) < 1e-4 && showFacetEdges == 0) {
        discard;
    } 

    finalColor = vec4(fragColor, 1);
}
