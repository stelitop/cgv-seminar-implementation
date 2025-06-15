#pragma once

#include <glm/ext/vector_float3.hpp>

class Settings {
public:
    bool simulate = true;
    bool showFacetEdges = false;
    int steps_per_frame = 5;
    int renderMode = 0;
    int numberOfStepsToTake = 3;
    float magnitudeCutoff = 0.3f; // used for visualising force/velocity, max value to clamp to

    bool useSelectedPoint = false;
    unsigned int selectedPointFace = -1;
    float selectedPointRadius = 0.05f;
    glm::vec3 selectedPointBarycentricCoords = glm::vec3(0);

    int faceSplitting = 1;

    // glyphs
    float glyphLength = 0.1f;
    float glyphScale = 4.0f;
    bool showGlyphNormalized = true;
    bool showGlyphVelocity = false;
    glm::vec3 glyphVelocityColor{0.0f, 0.8f, 0.8f};
    bool showGlyphTotalForce = false;
    glm::vec3 glyphTotalForceColor{ 1.0f, 0.0f, 0.0f };
    bool showGlyphAxialConstraints = false;
    glm::vec3 glyphAxialConstraintColor{ 1.0f, 0.4f, 0.0f };
    bool showGlyphCreaseConstraints = false;
    glm::vec3 glyphCreaseConstraintsColor{ 1.0f, 1.0f, 0.0f };
    bool showGlyphFaceConstraints = false;
    glm::vec3 glyphFaceConstraintsColor{ 0.7f, 0.0f, 1.0f };
    bool showGlyphDampingForce = false;
    glm::vec3 glyphDampingForceColor{ 0.0f, 0.0f, 0.8f };
};