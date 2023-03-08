#pragma once

#include <stdint.h>
#include <glm/glm.hpp>

enum button_type 
{
  keyboard_a, keyboard_b, keyboard_c, keyboard_d, keyboard_e, keyboard_f, keyboard_g, keyboard_h, keyboard_i, keyboard_j,
  keyboard_k, keyboard_l, keyboard_m, keyboard_n, keyboard_o, keyboard_p, keyboard_q, keyboard_r, keyboard_s, keyboard_t,
  keyboard_u, keyboard_v, keyboard_w, keyboard_x, keyboard_y, keyboard_z,

  keyboard_0, keyboard_1, keyboard_2, keyboard_3, keyboard_4, keyboard_5, keyboard_6,
  keyboard_7, keyboard_8, keyboard_9, keyboard_up, keyboard_left, keyboard_down, keyboard_right,
  keyboard_space, keyboard_left_shift, keyboard_left_control, keyboard_enter, keyboard_backspace,
  keyboard_escape, keyboard_f1, keyboard_f2, keyboard_f3, keyboard_f4, keyboard_f5, keyboard_f9, keyboard_f11,

  mouse_left, mouse_right, mouse_middle,

  button_none
};

struct button_state 
{
  uint8_t is_down : 1;
  uint8_t did_instant : 1;
  uint8_t did_release : 1;
  uint8_t pad : 5;
};

struct cursor 
{
  uint8_t did_cursor_move : 4;
  uint8_t did_scroll : 4;

  glm::ivec2 cursor_pos;
  glm::ivec2 previous_pos;
  glm::vec2 scroll;
};

// Initialize keyboard and mouse callback functions
void init_io_raw();
void tick_io_raw();

button_state get_button(button_type t);
cursor get_cursor();
