#include "Transform.h"

float getYaw(const glm::vec3& vec)
{
    glm::vec3 sameY = glm::normalize(glm::vec3(vec.x, g_DefaultForward.y, vec.z));

	return glm::orientedAngle(g_DefaultForward, sameY, g_DefaultUp);
}

float getPitch(const glm::vec3& vec)
{
    glm::vec3 projectedToYPlane = glm::normalize(glm::vec3(vec.x, g_DefaultForward.y, vec.z));
    float angle = glm::angle(vec, projectedToYPlane);

    return vec.y > 0.0f ? angle : -angle;
}

void applyYaw(glm::vec3& vec, float yaw)
{
    glm::quat rotationQuat = glm::angleAxis(yaw, g_DefaultUp);
    vec = rotationQuat * vec;
}

void applyPitch(glm::vec3& vec, float pitch)
{
    glm::vec3 right = glm::normalize(glm::cross(vec, g_DefaultUp));
    glm::quat rotationQuat = glm::angleAxis(pitch, right);

    vec = rotationQuat * vec;
}
