//#include "Image.h"
#include "mesh.h"
#include "texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include <glm/
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()

//#include <framework/>

#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include "camera.h"
#include "origami.h"
#include "glyph_drawer.h"
#include <ShObjIdl_core.h>
#include "settings.h"

class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
        , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
        , m_glyphDrawer(m_settings)
    {
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods);
        });
        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
        });
        m_window.registerWindowResizeCallback([&](const glm::ivec2& size)
            { m_projectionMatrix =
            glm::perspective(glm::radians(80.0f), m_window.getAspectRatio(), 0.1f, 10000.0f); });

        m_camera = Camera(&m_window, glm::vec3(1, 1, 1), glm::normalize(glm::vec3(-1, -1, -1)));

        //m_origami = Origami::load_from_file("origami_examples/simple.fold");
        //m_origami = Origami::loadFromFile("origami_examples/mapfold.fold
        //m_origami = Origami::loadFromFile("origami_examples/huffmanWaterbomb.fold");
        m_origami = Origami::loadFromFile(m_filename);

        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            ShaderBuilder faceBuilder;
            faceBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/face_vert.glsl");
            faceBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/face_frag.glsl");
            m_faceShader = faceBuilder.build();

            ShaderBuilder edgeBuilder;
            edgeBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/edge_vert.glsl");
            edgeBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/edge_frag.glsl");
            m_edgeShader = edgeBuilder.build();

            ShaderBuilder plainBuilder;
            plainBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/face_vert.glsl");
            plainBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/plain_frag.glsl");
            m_plainShader = plainBuilder.build();

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void update()
    {
        int dummyInteger = 0; // Initialized to 0
        while (!m_window.shouldClose()) {
            glViewport(0, 0, m_window.getWindowSize().x, m_window.getWindowSize().y);
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();

            // Use ImGui for easy input/output of ints, floats, strings, etc...
            renderUI();

            ImGuiIO io = ImGui::GetIO();
            if (!io.WantCaptureMouse) {
                m_camera.updateInput();
            }

            if (m_settings.simulate) {
                for (int i = 0; i < m_settings.steps_per_frame; i++) {
                    m_origami.step();
                    m_origami.updateVertexBuffers();
                }
            }

            // Clear the screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);

            const glm::mat4 mvpMatrix = m_projectionMatrix * m_camera.viewMatrix() * m_modelMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

            m_origami.draw(m_faceShader, m_edgeShader, mvpMatrix, m_settings);
            drawGlyphs(mvpMatrix);

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
        m_origami.free();
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {
        //std::cout << "Key pressed: " << key << std::endl;
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods)
    {
        //std::cout << "Key released: " << key << std::endl;
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& cursorPos)
    {
        //std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            auto tmp = m_window.getNormalizedCursorPos();
            auto screenResolution = glm::vec2(m_window.getWindowSize());
            auto pixel = glm::ivec2(tmp * screenResolution);
            glm::vec2 position = (glm::vec2(pixel) + 0.5f) / screenResolution * 2.0f - 1.0f;
            Ray r = m_camera.generateRay(position, m_projectionMatrix);
            if (m_origami.intersectWithRay(r)) {
                m_settings.selectedPointFace = r.face;
                m_settings.selectedPointBarycentricCoords = r.barycentricCoords;
                //std::cout << "Point selected: " << m_settings.selectedPoint.x << " " << m_settings.selectedPoint.y << " " << m_settings.selectedPoint.z << std::endl;
                m_settings.useSelectedPoint = true;
            }
            else {
                m_settings.useSelectedPoint = false;
            }
        }
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods)
    {
        //std::cout << "Released mouse button: " << button << std::endl;
    }

    void renderUI() {
        ImGui::Begin("Settings");
        ImGui::PushItemWidth(150.0f);

        //----------------------------------------------
        if (ImGui::Button("Select File")) {
            // file selection code taken from https://cplusplus.com/forum/windows/169960/
            char filename[MAX_PATH];

            OPENFILENAME ofn;
            ZeroMemory(&filename, sizeof(filename));
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
            ofn.lpstrFilter = "Fold Files\0*.fold\0Any File\0*.*\0";
            ofn.lpstrFile = filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = "Select a file";
            ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameA(&ofn))
            {
                m_filename = std::string(filename);
                m_origami.free();
                m_origami = Origami::loadFromFile(m_filename);
            }
        }
        ImGui::SameLine();
        ImGui::Text(m_origami.name.c_str());
        ImGui::SliderFloat("Fold Percent", &m_origami.target_angle_percent, 0.0f, 1.0f);
        ImGui::Checkbox("Simulate", &m_settings.simulate);
        ImGui::SameLine();
        ImGui::SliderInt("Steps Per Frame", &m_settings.steps_per_frame, 1, 10, "%d");

        if (ImGui::Button("Take Steps")) {
            for (int i = 0; i < m_settings.numberOfStepsToTake; i++) {
                m_origami.step();
            }
            m_origami.updateVertexBuffers();
        }
        ImGui::SameLine();
        ImGui::SliderInt("# Steps", &m_settings.numberOfStepsToTake, 1, 50);
        if (ImGui::Button("Reset Origami")) {
            m_origami.free();
            m_origami = Origami::loadFromFile(m_filename);
        }
        ImGui::SliderFloat("Selected Point Radius", &m_settings.selectedPointRadius, 0.0f, 0.5f);
        ImGui::Checkbox("Show Facet Creases", &m_settings.showFacetEdges);
        ImGui::NewLine();

        //----------------------------------------------

        if (ImGui::CollapsingHeader("Parameters")) {
            if (ImGui::Button("Reset Default Parameters")) {
                m_origami.setDefaultSettings();
            }
            ImGui::Checkbox("Enable Axial Constraints", &m_origami.enable_axial_constraints);
            if (m_origami.enable_axial_constraints) {
                if (ImGui::SliderFloat("Axial Stiffness (EA)", &m_origami.EA, 10.0f, 100.0f)) {
                    m_origami.calculateOptimalTimeStep();
                }
            }
            ImGui::Checkbox("Enable Crease Constraints", &m_origami.enable_crease_constraints);
            if (m_origami.enable_crease_constraints) {
                ImGui::SliderFloat("Fold Stiffness", &m_origami.k_fold, 0.0f, 3.0f);
                ImGui::SliderFloat("Facet Crease Stiffness", &m_origami.k_facet, 0.0f, 3.0f);
            }
            ImGui::Checkbox("Enable Face Constraints", &m_origami.enable_face_constraints);
            if (m_origami.enable_face_constraints) {
                ImGui::SliderFloat("Face Stiffness", &m_origami.k_face, 0.0f, 5.0f);
            }
            ImGui::Checkbox("Enable Damping Force", &m_origami.enable_damping_force);
            if (m_origami.enable_damping_force) {
                ImGui::SliderFloat("Damping Ratio", &m_origami.damping_ratio, 0.0f, 0.5f);
            }
            ImGui::NewLine();
        }

        //-------------------------------------------
        if (ImGui::CollapsingHeader("Visualisation Options")) {
            ImGui::Combo("Render Mode", &m_settings.renderMode, "Plain\0Velocity Amplitude\0Force Amplitude");

            if (m_settings.renderMode == RENDERMODE_FORCE || m_settings.renderMode == RENDERMODE_VELOCITY) {
                ImGui::SliderFloat("Magnitude Cutoff", &m_settings.magnitudeCutoff, 0.01f, 1.0f);
            }
            
            ImGui::NewLine();
        }
        //-------------------------------------------
        if (ImGui::CollapsingHeader("Glyph Options")) {
            ImGui::Checkbox("Normalize Glyphs", &m_settings.showGlyphNormalized);
            ImGui::SameLine();
            if (m_settings.showGlyphNormalized) {
                ImGui::SliderFloat("Glyph Length", &m_settings.glyphLength, 0.0f, 2.0f);
            }
            else {
                ImGui::SliderFloat("Glyph Scale", &m_settings.glyphScale, 0.01f, 10.0f);
            }
            ImGui::SliderInt("Face Splitting", &m_settings.faceSplitting, 1, 4);
            ImGui::Checkbox("Velocity Glyphs", &m_settings.showGlyphVelocity);
            ImGui::SameLine();
            ImGui::ColorEdit3("Color##Velocity Glyph Color", &m_settings.glyphVelocityColor[0]);
            ImGui::Checkbox("Total Force Glyphs", &m_settings.showGlyphTotalForce);
            ImGui::SameLine();
            ImGui::ColorEdit3("Color##Total Force Glyph Color", &m_settings.glyphTotalForceColor[0]);
            ImGui::Checkbox("Axial Constraint Glyphs", &m_settings.showGlyphAxialConstraints);
            ImGui::SameLine();
            ImGui::ColorEdit3("Color##Axial Constraint Glyph Color", &m_settings.glyphAxialConstraintColor[0]);
            ImGui::Checkbox("Crease Constraint Glyphs", &m_settings.showGlyphCreaseConstraints);
            ImGui::SameLine();
            ImGui::ColorEdit3("Color##Crease Constraint Glyph Color", &m_settings.glyphCreaseConstraintsColor[0]);
            ImGui::Checkbox("Face Constraint Glyphs", &m_settings.showGlyphFaceConstraints);
            ImGui::SameLine();
            ImGui::ColorEdit3("Color##Face Constraint Glyph Color", &m_settings.glyphFaceConstraintsColor[0]);
            ImGui::Checkbox("Damping Force Glyphs", &m_settings.showGlyphDampingForce);
            ImGui::SameLine();
            ImGui::ColorEdit3("Color##Damping Force Glyph Color", &m_settings.glyphDampingForceColor[0]);
            ImGui::NewLine();
        }

        ImGui::End();
    }

    void drawGlyphs(glm::mat4 mvpMatrix) {

        if (m_settings.showGlyphTotalForce) {
            if (m_settings.showGlyphNormalized) {
                m_glyphDrawer.loadGlyphsSetLength(m_origami, m_origami.getVertices(), m_origami.getTotalForce(), m_settings.glyphLength);
            }
            else {
                m_glyphDrawer.loadGlyphs(m_origami, m_origami.getVertices(), m_origami.getTotalForce(), m_settings.glyphScale);
            }
            m_glyphDrawer.draw(m_plainShader, mvpMatrix, m_settings.glyphTotalForceColor);
        }

        if (m_settings.showGlyphVelocity) {
            if (m_settings.showGlyphNormalized) {
                m_glyphDrawer.loadGlyphsSetLength(m_origami, m_origami.getVertices(), m_origami.getVelocities(), m_settings.glyphLength);
            }
            else {
                m_glyphDrawer.loadGlyphs(m_origami, m_origami.getVertices(), m_origami.getVelocities(), m_settings.glyphScale);
            }
            m_glyphDrawer.draw(m_plainShader, mvpMatrix, m_settings.glyphVelocityColor);
        }

        if (m_settings.showGlyphAxialConstraints) {
            if (m_settings.showGlyphNormalized) {
                m_glyphDrawer.loadGlyphsSetLength(m_origami, m_origami.getVertices(), m_origami.axialConstraints(), m_settings.glyphLength);
            }
            else {
                m_glyphDrawer.loadGlyphs(m_origami, m_origami.getVertices(), m_origami.axialConstraints(), m_settings.glyphScale);
            }
            m_glyphDrawer.draw(m_plainShader, mvpMatrix, m_settings.glyphAxialConstraintColor);
        }

        if (m_settings.showGlyphCreaseConstraints) {
            if (m_settings.showGlyphNormalized) {
                m_glyphDrawer.loadGlyphsSetLength(m_origami, m_origami.getVertices(), m_origami.creaseConstraints(), m_settings.glyphLength);
            }
            else {
                m_glyphDrawer.loadGlyphs(m_origami, m_origami.getVertices(), m_origami.creaseConstraints(), m_settings.glyphScale);
            }
            m_glyphDrawer.draw(m_plainShader, mvpMatrix, m_settings.glyphCreaseConstraintsColor);
        }

        if (m_settings.showGlyphFaceConstraints) {
            if (m_settings.showGlyphNormalized) {
                m_glyphDrawer.loadGlyphsSetLength(m_origami, m_origami.getVertices(), m_origami.faceConstraints(), m_settings.glyphLength);
            }
            else {
                m_glyphDrawer.loadGlyphs(m_origami, m_origami.getVertices(), m_origami.faceConstraints(), m_settings.glyphScale);
            }
            m_glyphDrawer.draw(m_plainShader, mvpMatrix, m_settings.glyphFaceConstraintsColor);
        }

        if (m_settings.showGlyphFaceConstraints) {
            if (m_settings.showGlyphNormalized) {
                m_glyphDrawer.loadGlyphsSetLength(m_origami, m_origami.getVertices(), m_origami.faceConstraints(), m_settings.glyphLength);
            }
            else {
                m_glyphDrawer.loadGlyphs(m_origami, m_origami.getVertices(), m_origami.faceConstraints(), m_settings.glyphScale);
            }
            m_glyphDrawer.draw(m_plainShader, mvpMatrix, m_settings.glyphFaceConstraintsColor);
        }

        if (m_settings.showGlyphDampingForce) {
            if (m_settings.showGlyphNormalized) {
                m_glyphDrawer.loadGlyphsSetLength(m_origami, m_origami.getVertices(), m_origami.dampingForce(), m_settings.glyphLength);
            }
            else {
                m_glyphDrawer.loadGlyphs(m_origami, m_origami.getVertices(), m_origami.dampingForce(), m_settings.glyphScale);
            }
            m_glyphDrawer.draw(m_plainShader, mvpMatrix, m_settings.glyphDampingForceColor);
        }
    }

private: 
    Window m_window;

    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_faceShader;
    Shader m_edgeShader;
    Shader m_plainShader;

    std::vector<GPUMesh> m_meshes;
    Texture m_texture;
    bool m_useMaterial { true };

    Camera m_camera = 0;

    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix { 1.0f };

    std::string m_filename = "origami_examples/mapfold.fold";
    Origami m_origami;
    GlyphDrawer m_glyphDrawer;

    Settings m_settings;
};

int main()
{
    Application app;
    app.update();

    return 0;
}
