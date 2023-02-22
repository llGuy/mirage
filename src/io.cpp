#include "io.hpp"
#include "imgui.h"
#include "log.hpp"
#include "debug_overlay.hpp"
#include "render_context.hpp"

const char *button_names_ [] = {
    "keyboard_a", "keyboard_b", "keyboard_c", "keyboard_d", "keyboard_e", "keyboard_f", "keyboard_g", "keyboard_h", "keyboard_i", "keyboard_j",
    "keyboard_k", "keyboard_l", "keyboard_m", "keyboard_n", "keyboard_o", "keyboard_p", "keyboard_q", "keyboard_r", "keyboard_s", "keyboard_t",
    "keyboard_u", "keyboard_v", "keyboard_w", "keyboard_x", "keyboard_y", "keyboard_z",

    "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", "keyboard_4", "keyboard_5", "keyboard_6",
    "keyboard_7", "keyboard_8", "keyboard_9", "keyboard_up", "keyboard_left", "keyboard_down", "keyboard_right",
    "keyboard_space", "keyboard_left_shift", "keyboard_left_control", "keyboard_enter", "keyboard_backspace",
    "keyboard_escape", "keyboard_f1", "keyboard_f2", "keyboard_f3", "keyboard_f4", "keyboard_f5", "keyboard_f9", "keyboard_f11",

    "mouse_left", "mouse_right", "mouse_middle",

    "button_none"
};

static button_state buttons_[button_type::button_none];

static uint32_t pressed_key_count_;
static uint32_t pressed_keys_[button_type::button_none];

static uint32_t released_key_count_;
static uint32_t released_keys_[button_type::button_none];

static cursor cursor_;

static void key_callback_(GLFWwindow *win, int key, int sc, int action, int mods) {
    button_type button;

    switch(key) {
    case GLFW_KEY_A: { button = button_type::keyboard_a; } break;
    case GLFW_KEY_B: { button = button_type::keyboard_b; } break;
    case GLFW_KEY_C: { button = button_type::keyboard_c; } break;
    case GLFW_KEY_D: { button = button_type::keyboard_d; } break;
    case GLFW_KEY_E: { button = button_type::keyboard_e; } break;
    case GLFW_KEY_F: { button = button_type::keyboard_f; } break;
    case GLFW_KEY_G: { button = button_type::keyboard_g; } break;
    case GLFW_KEY_H: { button = button_type::keyboard_h; } break;
    case GLFW_KEY_I: { button = button_type::keyboard_i; } break;
    case GLFW_KEY_J: { button = button_type::keyboard_j; } break;
    case GLFW_KEY_K: { button = button_type::keyboard_k; } break;
    case GLFW_KEY_L: { button = button_type::keyboard_l; } break;
    case GLFW_KEY_M: { button = button_type::keyboard_m; } break;
    case GLFW_KEY_N: { button = button_type::keyboard_n; } break;
    case GLFW_KEY_O: { button = button_type::keyboard_o; } break;
    case GLFW_KEY_P: { button = button_type::keyboard_p; } break;
    case GLFW_KEY_Q: { button = button_type::keyboard_q; } break;
    case GLFW_KEY_R: { button = button_type::keyboard_r; } break;
    case GLFW_KEY_S: { button = button_type::keyboard_s; } break;
    case GLFW_KEY_T: { button = button_type::keyboard_t; } break;
    case GLFW_KEY_U: { button = button_type::keyboard_u; } break;
    case GLFW_KEY_V: { button = button_type::keyboard_v; } break;
    case GLFW_KEY_W: { button = button_type::keyboard_w; } break;
    case GLFW_KEY_X: { button = button_type::keyboard_x; } break;
    case GLFW_KEY_Y: { button = button_type::keyboard_y; } break;
    case GLFW_KEY_Z: { button = button_type::keyboard_z; } break;
    case GLFW_KEY_0: { button = button_type::keyboard_0; } break;
    case GLFW_KEY_1: { button = button_type::keyboard_1; } break;
    case GLFW_KEY_2: { button = button_type::keyboard_2; } break;
    case GLFW_KEY_3: { button = button_type::keyboard_3; } break;
    case GLFW_KEY_4: { button = button_type::keyboard_4; } break;
    case GLFW_KEY_5: { button = button_type::keyboard_5; } break;
    case GLFW_KEY_6: { button = button_type::keyboard_6; } break;
    case GLFW_KEY_7: { button = button_type::keyboard_7; } break;
    case GLFW_KEY_8: { button = button_type::keyboard_8; } break;
    case GLFW_KEY_9: { button = button_type::keyboard_9; } break;
    case GLFW_KEY_UP: { button = button_type::keyboard_up; } break;
    case GLFW_KEY_LEFT: { button = button_type::keyboard_left; } break;
    case GLFW_KEY_DOWN: { button = button_type::keyboard_right; } break;
    case GLFW_KEY_RIGHT: { button = button_type::keyboard_right; } break;
    case GLFW_KEY_SPACE: { button = button_type::keyboard_space; } break;
    case GLFW_KEY_LEFT_SHIFT: { button = button_type::keyboard_left_shift; } break;
    case GLFW_KEY_LEFT_CONTROL: { button = button_type::keyboard_left_control; } break;
    case GLFW_KEY_ENTER: { button = button_type::keyboard_enter; } break;
    case GLFW_KEY_BACKSPACE: { button = button_type::keyboard_backspace; } break;
    case GLFW_KEY_ESCAPE: { button = button_type::keyboard_escape; } break;
    default: {} return;
    }

    switch(action) {
    case GLFW_PRESS: case GLFW_REPEAT: {
        buttons_[button].is_down = true;
        buttons_[button].did_instant = true;

        pressed_keys_[pressed_key_count_++] = button;
    } break;

    case GLFW_RELEASE: {
        buttons_[button].is_down = false;
        buttons_[button].did_release = true;

        released_keys_[released_key_count_++] = button;
    } break;
    }
}

static void mouse_callback_(GLFWwindow *win, int mb, int action, int mods) {
    button_type button;

    switch(mb) {
    case GLFW_MOUSE_BUTTON_LEFT: { button = button_type::mouse_left; } break;
    case GLFW_MOUSE_BUTTON_RIGHT: { button = button_type::mouse_right; } break;
    case GLFW_MOUSE_BUTTON_MIDDLE: { button = button_type::mouse_middle; } break;
    default: {} return;
    }

    switch(action) {
    case GLFW_PRESS: case GLFW_REPEAT: {
        buttons_[button].is_down = true;
        buttons_[button].did_instant = true;

        pressed_keys_[pressed_key_count_++] = button;
    } break;

    case GLFW_RELEASE: {
        buttons_[button].is_down = false;
        buttons_[button].did_release = true;

        released_keys_[released_key_count_++] = button;
    } break;
    }
}

static void cursor_pos_callback_(GLFWwindow *win, double x, double y) {
    cursor_.previous_pos = cursor_.cursor_pos;
    cursor_.cursor_pos = glm::ivec2((int)x, (int)y);
    cursor_.did_cursor_move = true;
}

void init_io_raw() {
    glfwSetKeyCallback(gctx->window, key_callback_);
    glfwSetMouseButtonCallback(gctx->window, mouse_callback_);
    glfwSetCursorPosCallback(gctx->window, cursor_pos_callback_);

    register_debug_overlay_client("IO", [] () {
        ImGui::Text("Cursor: %d %d", cursor_.cursor_pos.x, cursor_.cursor_pos.y);
    });

    double x, y;
    glfwGetCursorPos(gctx->window, &x, &y);
    cursor_.cursor_pos = iv2((int)x, (int)y);
}

void tick_io_raw() {
    // Clear
    for (int i = 0; i < pressed_key_count_; ++i) {
        int idx = pressed_keys_[i];
        buttons_[idx].did_instant = false;
    }

    for (int i = 0; i < released_key_count_; ++i) {
        int idx = released_keys_[i];
        buttons_[idx].did_release = false;
    }

    pressed_key_count_ = 0;
    released_key_count_ = 0;

    cursor_.did_cursor_move = false;
    cursor_.did_scroll = false;

    // Get input
    poll_input();
}

button_state get_button(button_type t) {
    return buttons_[t];
}

cursor get_cursor() {
    return cursor_;
}
