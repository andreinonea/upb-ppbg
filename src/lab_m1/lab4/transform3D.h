#pragma once

#include "utils/glm_utils.h"


namespace transform3D
{
    // Translate matrix
    inline glm::mat4 Translate(float translateX, float translateY, float translateZ)
    {
        // TODO(student): Implement the translation matrix
        return glm::transpose(
            glm::mat4{ 1.0f, 0.0f, 0.0f, translateX,
                       0.0f, 1.0f, 0.0f, translateY,
                       0.0f, 0.0f, 1.0f, translateZ,
                       0.0f, 0.0f, 0.0f,       1.0f }
        );
    }

    // Scale matrix
    inline glm::mat4 Scale(float scaleX, float scaleY, float scaleZ)
    {
        // TODO(student): Implement the scaling matrix
        return glm::transpose(
            glm::mat4{ scaleX,   0.0f,   0.0f, 0.0f,
                         0.0f, scaleY,   0.0f, 0.0f,
                         0.0f,   0.0f, scaleZ, 0.0f,
                         0.0f,   0.0f,   0.0f, 1.0f }
        );
    }

    // Rotate matrix relative to the OZ axis
    inline glm::mat4 RotateOZ(float radians)
    {
        // TODO(student): Implement the rotation matrix
        return glm::transpose(
            glm::mat4{ cosf(radians), -sinf(radians), 0.0f, 0.0f,
                       sinf(radians),  cosf(radians), 0.0f, 0.0f,
                                0.0f,           0.0f, 1.0f, 0.0f,
                                0.0f,           0.0f, 0.0f, 1.0f }
        );
    }

    // Rotate matrix relative to the OY axis
    inline glm::mat4 RotateOY(float radians)
    {
        // TODO(student): Implement the rotation matrix
        return glm::transpose(
            glm::mat4{  cosf(radians), 0.0f, sinf(radians), 0.0f,
                                 0.0f, 1.0f,          0.0f, 0.0f,
                       -sinf(radians), 0.0f, cosf(radians), 0.0f,
                                 0.0f, 0.0f,          0.0f, 1.0f }
        );
    }

    // Rotate matrix relative to the OX axis
    inline glm::mat4 RotateOX(float radians)
    {
        // TODO(student): Implement the rotation matrix
        return glm::transpose(
            glm::mat4{ 1.0f,          0.0f,           0.0f, 0.0f,
                       0.0f, cosf(radians), -sinf(radians), 0.0f,
                       0.0f, sinf(radians),  cosf(radians), 0.0f,
                       0.0f,          0.0f,           0.0f, 1.0f }
        );
    }
}   // namespace transform3D
