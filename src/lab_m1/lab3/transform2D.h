#pragma once

#include "utils/glm_utils.h"


namespace transform2D
{
    // Translate matrix
    inline glm::mat3 Translate(float translateX, float translateY)
    {
        // TODO(student): Implement the translation matrix
        return glm::transpose(
            glm::mat3{ 1.0f, 0.0f, translateX,
                       0.0f, 1.0f, translateY,
                       0.0f, 0.0f, 1.0f }
        );

    }

    // Scale matrix
    inline glm::mat3 Scale(float scaleX, float scaleY)
    {
        // TODO(student): Implement the scaling matrix
        return glm::transpose(
            glm::mat3{ scaleX, 0.0f, 0.0f,
                       0.0f, scaleY, 0.0f,
                       0.0f, 0.0f, 1.0f }
        );

    }

    // Rotate matrix
    inline glm::mat3 Rotate(float radians)
    {
        // TODO(student): Implement the rotation matrix
        return glm::transpose(
            glm::mat3{ cosf(radians), -sinf(radians), 0.0f,
                       sinf(radians), cosf(radians), 0.0f,
                       0.0f, 0.0f, 1.0f }
        );

    }
}   // namespace transform2D
