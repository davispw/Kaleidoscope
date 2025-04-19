/* Kaleidoscope-Macros -- Macro keys for Kaleidoscope
 * Copyright 2017-2025 Keyboard.io, inc.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * Additional Permissions:
 * As an additional permission under Section 7 of the GNU General Public
 * License Version 3, you may link this software against a Vendor-provided
 * Hardware Specific Software Module under the terms of the MCU Vendor
 * Firmware Library Additional Permission Version 1.0.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "kaleidoscope/plugin/Macros.h"

#include <Arduino.h>                   // for pgm_read_byte, delay, F, PROGMEM, __F...
#include <Kaleidoscope-FocusSerial.h>  // for Focus, FocusSerial
#include <Kaleidoscope-Ranges.h>       // for MACRO_FIRST
#include <stdint.h>                    // for uint8_t

#include "kaleidoscope/KeyEvent.h"              // for KeyEvent
#include "kaleidoscope/event_handler_result.h"  // for EventHandlerResult, EventHandlerResul...
#include "kaleidoscope/key_defs.h"              // for Key, LSHIFT, Key_NoKey, Key_0, Key_1
#include "kaleidoscope/keyswitch_state.h"       // for keyToggledOff
#include "kaleidoscope/plugin/MacroSteps.h"     // for macro_t, MACRO_NONE, MACRO_ACTION_END

// =============================================================================
// Default `macroAction()` function definitions
__attribute__((weak))
const macro_t *
macroAction(uint8_t macro_id, KeyEvent &event) {
  return MACRO_NONE;
}

// =============================================================================
// `Macros` plugin code
namespace kaleidoscope {
namespace plugin {

// Accessor for MacroSupport::play to read Program Memory.
struct PgmAccessor {
  const macro_t *current;

  // There is no boundary check for program macros.
  // The main loop checks for MACRO_STEP_END.
  // The sequence loop checks for Key_NoKey.
  static constexpr bool has_boundary = false;

  explicit PgmAccessor(const macro_t *macro_ptr)
    : current(macro_ptr) {}

  // Read the next byte from PROGMEM and advance.
  inline uint8_t readByte() {
    // No internal boundary check here -- relies on loop structure.
    return pgm_read_byte(current++);
  }
};

// -----------------------------------------------------------------------------
// Public helper functions

void Macros::play(const macro_t *macro_ptr) {
  if (macro_ptr == MACRO_NONE)
    return;

  PgmAccessor accessor(macro_ptr);
  ::MacroSupport.play(accessor);
}

const macro_t *Macros::type(const char *string) const {
  while (true) {
    uint8_t ascii_code = pgm_read_byte(string++);
    if (ascii_code == 0)
      break;

    Key key = lookupAsciiCode(ascii_code);

    if (key == Key_NoKey)
      continue;

    tap(key);
  }

  return MACRO_NONE;
}

// -----------------------------------------------------------------------------
// Translation from ASCII to keycodes

static const Key ascii_to_key_map[] PROGMEM = {
  // 0x21 - 0x30
  LSHIFT(Key_1),
  LSHIFT(Key_Quote),
  LSHIFT(Key_3),
  LSHIFT(Key_4),
  LSHIFT(Key_5),
  LSHIFT(Key_7),
  Key_Quote,
  LSHIFT(Key_9),
  LSHIFT(Key_0),
  LSHIFT(Key_8),
  LSHIFT(Key_Equals),
  Key_Comma,
  Key_Minus,
  Key_Period,
  Key_Slash,
  Key_0,

  // 0x3a ... 0x40
  LSHIFT(Key_Semicolon),
  Key_Semicolon,
  LSHIFT(Key_Comma),
  Key_Equals,
  LSHIFT(Key_Period),
  LSHIFT(Key_Slash),
  LSHIFT(Key_2),

  // 0x5b ... 0x60
  Key_LeftBracket,
  Key_Backslash,
  Key_RightBracket,
  LSHIFT(Key_6),
  LSHIFT(Key_Minus),
  Key_Backtick,

  // 0x7b ... 0x7e
  LSHIFT(Key_LeftBracket),
  LSHIFT(Key_Backslash),
  LSHIFT(Key_RightBracket),
  LSHIFT(Key_Backtick),
};

Key Macros::lookupAsciiCode(uint8_t ascii_code) const {
  Key key = Key_NoKey;

  switch (ascii_code) {
  case 0x08 ... 0x09:
    key.setKeyCode(Key_Backspace.getKeyCode() + ascii_code - 0x08);
    break;
  case 0x0A:
    key.setKeyCode(Key_Enter.getKeyCode());
    break;
  case 0x1B:
    key.setKeyCode(Key_Escape.getKeyCode());
    break;
  case 0x20:
    key.setKeyCode(Key_Spacebar.getKeyCode());
    break;
  case 0x21 ... 0x30:
    key = ascii_to_key_map[ascii_code - 0x21].readFromProgmem();
    break;
  case 0x31 ... 0x39:
    key.setKeyCode(Key_1.getKeyCode() + ascii_code - 0x31);
    break;
  case 0x3A ... 0x40:
    key = ascii_to_key_map[ascii_code - 0x3A + 16].readFromProgmem();
    break;
  case 0x41 ... 0x5A:
    key.setFlags(SHIFT_HELD);
    key.setKeyCode(Key_A.getKeyCode() + ascii_code - 0x41);
    break;
  case 0x5B ... 0x60:
    key = ascii_to_key_map[ascii_code - 0x5B + 23].readFromProgmem();
    break;
  case 0x61 ... 0x7A:
    key.setKeyCode(Key_A.getKeyCode() + ascii_code - 0x61);
    break;
  case 0x7B ... 0x7E:
    key = ascii_to_key_map[ascii_code - 0x7B + 29].readFromProgmem();
    break;
  }
  return key;
}

// -----------------------------------------------------------------------------
// Event handlers

EventHandlerResult Macros::onKeyEvent(KeyEvent &event) {
  // Ignore everything except Macros keys
  if (!isMacrosKey(event.key))
    return EventHandlerResult::OK;

  // Decode the macro ID from the Macros `Key` value.
  uint8_t macro_id = event.key.getRaw() - ranges::MACRO_FIRST;

  // Call the new `macroAction(event)` function.
  const macro_t *macro_ptr = macroAction(macro_id, event);

  // Play back the macro pointed to by `macroAction()`
  play(macro_ptr);

  if (keyToggledOff(event.state) || !isMacrosKey(event.key)) {
    // If a Macros key toggled off or if the value of `event.key` has been
    // changed by the user-defined `macroAction()` function, we clear the array
    // of active macro keys so that they won't get "stuck on".  There won't be a
    // subsequent event that Macros will recognize as actionable, so we need to
    // do it here.
    clear();
  }

  // Return `OK` to let Kaleidoscope finish processing this event as normal.
  // This is so that, if the user-defined `macroAction(id, &event)` function
  // changes the value of `event.key`, it will take effect properly.  Note that
  // we're counting on other plugins to not subsequently change the value of
  // `event.key` if a Macros key has toggled on, because that would leave any
  // keys in the supplemental array "stuck on".  We could return
  // `EVENT_CONSUMED` if `event.key` is still a Macros key, but that would lead
  // to other undesirable plugin interactions (e.g. OneShot keys wouldn't be
  // triggered to turn off when a Macros key toggles on, assuming that Macros
  // comes first in `KALEIDOSCOPE_INIT_PLUGINS()`).
  return EventHandlerResult::OK;
}

EventHandlerResult Macros::onNameQuery() {
  return ::Focus.sendName(F("Macros"));
}

}  // namespace plugin
}  // namespace kaleidoscope

kaleidoscope::plugin::Macros Macros;
