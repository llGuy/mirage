#pragma once

#include "types.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

struct viewer_desc {
    m4x4 projection;
    m4x4 view;
    m4x4 inverse_projection;
    m4x4 inverse_view;
    m4x4 view_projection;

    alignas(16) v3 wposition;
    alignas(16) v3 wview_dir;
    alignas(16) v3 wup;

    f32 fov;
    f32 aspect_ratio;
    f32 near;
    f32 far;

    void tick(iv2 viewport_res) {
        near = 1.0f;
        far = 100000.0f;
        aspect_ratio = (float)viewport_res.x / (float)viewport_res.y;

        projection = glm::perspective(glm::radians(fov), aspect_ratio, near, far);
        view = glm::lookAt(wposition, wposition + wview_dir, wup);
        inverse_projection = glm::inverse(projection);
        inverse_view = glm::inverse(view);
        view_projection = projection * view;
    }
};

void tick_debug_viewer();
