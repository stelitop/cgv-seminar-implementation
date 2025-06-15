#include "camera.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
DISABLE_WARNINGS_POP()

Camera::Camera(Window* pWindow)
    : Camera(pWindow, glm::vec3(0), glm::vec3(0, 0, -1))
{
}

Camera::Camera(Window* pWindow, const glm::vec3& pos, const glm::vec3& forward)
    : m_position(pos)
    , m_forward(glm::normalize(forward))
    , m_pWindow(pWindow)
{
}

void Camera::setUserInteraction(bool enabled)
{
    m_userInteraction = enabled;
}

glm::vec3 Camera::cameraPos() const
{
    return m_position;
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

Ray Camera::generateRay(const glm::vec2& pixel, glm::mat4 projection_matrix)
{
    glm::vec4 rayClip = glm::vec4(pixel.x, pixel.y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection_matrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0, 0.0);
    glm::vec4 rayWorld = glm::inverse(viewMatrix()) * rayEye;

    Ray ray;
    ray.origin = m_position;
    ray.direction = glm::normalize(glm::vec3(rayWorld.x, rayWorld.y, rayWorld.z));
    ray.t = std::numeric_limits<float>::max();
    return ray;
}

void Camera::rotateX(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, horAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::rotateY(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, s_yAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::updateInput()
{
    /*std::cout << "Camera:" << std::endl;
    std::cout << m_position.x << " " << m_position.y << " " << m_position.z << std::endl;
    std::cout << m_forward.x << " " << m_forward.y << " " << m_forward.z << std::endl;
    std::cout << m_up.x << " " << m_up.y << " " << m_up.z << std::endl;*/

    constexpr float moveSpeed = 0.03f;
    constexpr float lookSpeed = 0.0015f;

    if (m_userInteraction) {
        glm::vec3 localMoveDelta{ 0 };
        const glm::vec3 right = glm::normalize(glm::cross(m_forward, m_up));
        if (m_pWindow->isKeyPressed(GLFW_KEY_A))
            m_position -= moveSpeed * right;
        if (m_pWindow->isKeyPressed(GLFW_KEY_D))
            m_position += moveSpeed * right;
        if (m_pWindow->isKeyPressed(GLFW_KEY_W))
            m_position += moveSpeed * m_forward;
        if (m_pWindow->isKeyPressed(GLFW_KEY_S))
            m_position -= moveSpeed * m_forward;
        if (m_pWindow->isKeyPressed(GLFW_KEY_SPACE))
            m_position += moveSpeed * m_up;
        if (m_pWindow->isKeyPressed(GLFW_KEY_C))
            m_position -= moveSpeed * m_up;

        const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
        const glm::vec2 delta = lookSpeed * glm::vec2(m_prevCursorPos - cursorPos);
        m_prevCursorPos = cursorPos;

        if (m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            if (delta.x != 0.0f)
                rotateY(delta.x);
            if (delta.y != 0.0f)
                rotateX(delta.y);
        }
    }
    else {
        m_prevCursorPos = m_pWindow->getCursorPos();
    }
}