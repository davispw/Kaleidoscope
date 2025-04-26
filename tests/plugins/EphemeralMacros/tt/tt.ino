/* -*- mode: c++ -*-
 * Copyright (C) 2020  Keyboard.io, Inc.
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
#include <Kaleidoscope-TopsyTurvy.h>

// *INDENT-OFF*
KEYMAPS(
  [0] = KEYMAP_STACKED
  (
      Key_RecordMacro, Key_PlayRecordedMacro, ___, ___, ___, ___, ___,
      Key_X, TOPSY(X), ___, ___, ___, ___, ___,
      Key_LeftShift, ___, ___, ___, ___, ___,
      ___, ___, ___, ___, ___, ___, ___,
      ___, ___, ___, ___,
      ___,

      ___, ___, ___, ___, ___, ___, ___,
      ___, ___, ___, ___, ___, ___, ___,
           ___, ___, ___, ___, ___, ___,
      ___, ___, ___, ___, ___, ___, ___,
      ___, ___, ___, ___,
      ___
   ),
)
// *INDENT-ON*

// Use Redial
KALEIDOSCOPE_INIT_PLUGINS(EphemeralMacros, TopsyTurvy);

uint8_t buffer[32];

void setup() {
  Kaleidoscope.setup();

  EphemeralMacros.initializeBuffer(buffer, 32);
}

void loop() {
  Kaleidoscope.loop();
}
