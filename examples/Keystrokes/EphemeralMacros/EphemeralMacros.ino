/* -*- mode: c++ -*-
 * Kaleidoscope-EphemeralMacros Examples
 * Copyright (C) 2021  Keyboard.io, Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Kaleidoscope.h>
#include <Kaleidoscope-EphemeralMacros.h>

// clang-format off
KEYMAPS(
  [0] = KEYMAP_STACKED
  (Key_RecordMacro, ___, ___, ___, ___, ___, Key_PlayRecordedMacro,
   Key_Backtick, Key_Q, Key_W, Key_E, Key_R, Key_T, Key_Tab,
   Key_PageUp,   Key_A, Key_S, Key_D, Key_F, Key_G,
   Key_PageDown, Key_Z, Key_X, Key_C, Key_V, Key_B, Key_Escape,

   Key_LeftControl, Key_Backspace, Key_LeftGui, Key_LeftShift,
   ShiftToLayer(1),

   ___, ___, ___, ___, ___, ___, ___,
   Key_Enter, Key_Y, Key_U, Key_I,     Key_O,      Key_P,         Key_Equals,
              Key_H, Key_J, Key_K,     Key_L,      Key_Semicolon, Key_Quote,
   ___,       Key_N, Key_M, Key_Comma, Key_Period, Key_Slash,     Key_Minus,

   Key_RightShift, Key_LeftAlt, Key_Spacebar, Key_RightControl,
   ShiftToLayer(1)),

  [1] = KEYMAP_STACKED
  (
    ___, ___, ___, ___, ___, ___, ___,
    ___, ___, ___, ___, ___, ___, ___,
    ___, ___, ___, ___, ___, ___,
    ___, ___, ___, ___, ___, ___, ___,

    ___, ___, ___, ___,
    ___,

    ___, ___,         ___,         ___,        ___,                ___, ___,
    ___, ___,         ___,         ___,        ___,                ___, ___,
         Key_UpArrow, Key_DownArrow, Key_LeftArrow, Key_RightArrow,___, ___,
    ___, ___,         ___,         ___,        ___,                ___, ___,

    ___, ___, ___, ___,
    ___),
)
// clang-format on

KALEIDOSCOPE_INIT_PLUGINS(EphemeralMacros);

// Allocate RAM to store Ephemeral Macro recordings.
constexpr size_t ephemeralMacroSize = 128;
uint8_t ephemeralMacroBuffer[ephemeralMacroSize];

void setup() {
  Kaleidoscope.setup();

  // For Ephemeral Macros, we need to tell the plugin to use the allocated RAM.
  EphemeralMacros.initializeBuffer(ephemeralMacroBuffer, ephemeralMacroSize);
}

void loop() {
  Kaleidoscope.loop();
}
