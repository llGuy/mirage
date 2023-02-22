#include "io.hpp"
#include "time.hpp"
#include "viewer.hpp"
#include "core_render.hpp"

#include <glm/glm.hpp>

void tick_debug_viewer() {
    ggfx->viewer.fov = glm::radians(60.0f);

    if (get_button(button_type::mouse_left).is_down) {
        v3 up = ggfx->viewer.wup;
        v3 right = glm::normalize(glm::cross(ggfx->viewer.wview_dir, ggfx->viewer.wup));
        v3 forward = glm::normalize(glm::cross(up, right));

        if (get_button(button_type::keyboard_w).is_down)
            ggfx->viewer.wposition += forward * gtime->frame_dt;
        if (get_button(button_type::keyboard_a).is_down)
            ggfx->viewer.wposition += -right * gtime->frame_dt;
        if (get_button(button_type::keyboard_s).is_down)
            ggfx->viewer.wposition += -forward * gtime->frame_dt;
        if (get_button(button_type::keyboard_d).is_down)
            ggfx->viewer.wposition += right * gtime->frame_dt;
        if (get_button(button_type::keyboard_space).is_down)
            ggfx->viewer.wposition += up * gtime->frame_dt;
        if (get_button(button_type::keyboard_left_shift).is_down)
            ggfx->viewer.wposition += -up * gtime->frame_dt;

        if (get_cursor().did_cursor_move) {
            auto cur = get_cursor();

            static constexpr float sensitivity = 15.0f;

            auto delta = cur.cursor_pos - cur.previous_pos;
            auto res = ggfx->viewer.wview_dir;

            float x_angle = glm::radians((float)-delta.x) * sensitivity * gtime->frame_dt;
            float y_angle = glm::radians((float)-delta.y) * sensitivity * gtime->frame_dt;

            res = glm::mat3(glm::rotate(x_angle, up)) * res;
            auto rotate_y = glm::cross(res, up);
            res = glm::mat3(glm::rotate(y_angle, rotate_y)) * res;

            res = glm::normalize(res);

            ggfx->viewer.wview_dir = res;
        }
    }
}
