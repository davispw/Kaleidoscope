/* Kaleidoscope-MacroSupport -- Macro keys for Kaleidoscope
 * Copyright 2022-2025 Keyboard.io, inc.
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

#include "kaleidoscope/plugin/MacroSupport.h"

#include <Arduino.h>                   // for F, __FlashStringHelper
#include <Kaleidoscope-FocusSerial.h>  // for Focus, FocusSerial
#include <stdint.h>                    // for uint8_t

#include "kaleidoscope/KeyAddr.h"               // for KeyAddr
#include "kaleidoscope/KeyEvent.h"              // for KeyEvent
#include "kaleidoscope/Runtime.h"               // for Runtime, Runtime_
#include "kaleidoscope/event_handler_result.h"  // for EventHandlerResult, EventHandlerResult::OK
#include "kaleidoscope/key_defs.h"              // for Key, Key_NoKey
#include "kaleidoscope/keyswitch_state.h"       // for INJECTED, IS_PRESSED, WAS_PRESSED

#include "kaleidoscope/device/virtual/Logging.h"  // for log_info, logging

// =============================================================================
// `Macros` plugin code
namespace kaleidoscope {
namespace plugin {

constexpr uint8_t press_state   = IS_PRESSED | INJECTED;
constexpr uint8_t release_state = WAS_PRESSED | INJECTED;

// -----------------------------------------------------------------------------
// Public helper functions

void MacroSupport::press(Key key) {
  Runtime.handleKeyEvent(KeyEvent{KeyAddr::none(), press_state, key});
#ifdef KALEIDOSCOPE_VIRTUAL_BUILD
  ::kaleidoscope::logging::log_error("MacroSupport.press(%d)\n", key.getRaw());
#endif
  // This key may remain active for several subsequent events, so we need to
  // store it in the active macro keys array.
  for (Key &macro_key : active_macro_keys_) {
    if (macro_key == Key_NoKey) {
      macro_key = key;
      break;
    }
#ifdef KALEIDOSCOPE_VIRTUAL_BUILD
    ::kaleidoscope::logging::log_error("> MacroSupport.press(): holding %d\n", macro_key.getRaw());
#endif
  }
}

void MacroSupport::release(Key key) {
#ifdef KALEIDOSCOPE_VIRTUAL_BUILD
  ::kaleidoscope::logging::log_error("MacroSupport.release(%d)\n", key.getRaw());
#endif
  // Before sending the release event, we need to remove the key from the active
  // macro keys array, or it will get inserted into the report anyway.
  for (Key &macro_key : active_macro_keys_) {
    if (macro_key == key) {
      macro_key = Key_NoKey;
    }
#ifdef KALEIDOSCOPE_VIRTUAL_BUILD
    else if (macro_key != Key_NoKey)
      ::kaleidoscope::logging::log_error("> MacroSupport.release(): still holding %d\n", macro_key.getRaw());
#endif
  }
  Runtime.handleKeyEvent(KeyEvent{KeyAddr::none(), release_state, key});
}

void MacroSupport::clear() {
#ifdef KALEIDOSCOPE_VIRTUAL_BUILD
  ::kaleidoscope::logging::log_error("MacroSupport.clear()\n");
#endif
  // Clear the active macro keys array.
  for (Key &macro_key : active_macro_keys_) {
    if (macro_key == Key_NoKey)
      continue;
    Runtime.handleKeyEvent(KeyEvent{KeyAddr::none(), release_state, macro_key});
    macro_key = Key_NoKey;
  }
}

void MacroSupport::tap(Key key) const {
  // No need to call `press()` & `release()`, because we're immediately
  // releasing the key after pressing it. It is possible for some other plugin
  // to insert an event in between, but very unlikely.
#ifdef KALEIDOSCOPE_VIRTUAL_BUILD
  ::kaleidoscope::logging::log_error("MacroSupport.tap(%d): delay=%d\n", key.getRaw(), MACRO_TAP_DELAY);
#endif
  Runtime.handleKeyEvent(KeyEvent{KeyAddr::none(), press_state, key});
  delay(MACRO_TAP_DELAY);
  Runtime.handleKeyEvent(KeyEvent{KeyAddr::none(), release_state, key});
}

// -----------------------------------------------------------------------------
// Event handlers

EventHandlerResult MacroSupport::beforeReportingState(const KeyEvent &event) {
  // Do this in beforeReportingState(), instead of `onAddToReport()` because
  // `live_keys` won't get updated until after the macro sequence is played from
  // the keypress. This could be changed by either updating `live_keys` manually
  // ahead of time, or by executing the macro sequence on key release instead of
  // key press. This is probably the simplest solution.
  ::kaleidoscope::logging::log_error("# MacroSupport.beforeReportingState():\n");
  for (Key key : active_macro_keys_) {
#ifdef KALEIDOSCOPE_VIRTUAL_BUILD
    if (key != Key_NoKey)
      ::kaleidoscope::logging::log_error("> MacroSupport.beforeReportingState(): holding %d\n", key.getRaw());
#endif
    if (key != Key_NoKey)
      Runtime.addToReport(key);
  }
  return EventHandlerResult::OK;
}

EventHandlerResult MacroSupport::onNameQuery() {
  return ::Focus.sendName(F("MacroSupport"));
}

}  // namespace plugin
}  // namespace kaleidoscope

kaleidoscope::plugin::MacroSupport MacroSupport;
