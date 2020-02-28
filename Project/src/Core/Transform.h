#pragma once

#include "glm/glm.hpp"

const glm::vec3 g_DefaultForward(0.0f, 0.0f, 1.0f);
const glm::vec3 g_DefaultUp(0.0f, 1.0f, 0.0f);
const glm::vec3 g_DefaultRight(glm::cross(g_DefaultForward, g_DefaultUp));

float getYaw(const glm::vec3& vec);
float getPitch(const glm::vec3& vec);

void applyYaw(glm::vec3& vec, float yaw);
void applyPitch(glm::vec3& vec, float pitch);


