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

#pragma once

#include "kaleidoscope/KeyEvent.h"              // for KeyEvent
#include "kaleidoscope/event_handler_result.h"  // for EventHandlerResult
#include "kaleidoscope/key_defs.h"              // for Key
#include "kaleidoscope/plugin.h"                // for Plugin

// This is a special exception to the rule of only including a plugin's
// top-level header file, because MacroSupport doesn't depend on the Macros
// plugin itself; it's just using the same macro step definitions.
#include "kaleidoscope/plugin/Macros/MacroSteps.h"  // for MACRO_ACTION_END, MACRO_ACTION_STEP_...

// =============================================================================
// The number of simultaneously-active `Key` values that a macro can have
// running during a call to `Macros.play()`. I don't know if it's actually
// possible to override this by defining it in a sketch before including
// "Kaleidoscope-Macros.h", but probably not.
#if !defined(MAX_CONCURRENT_MACRO_KEYS)
#define MAX_CONCURRENT_MACRO_KEYS 8
#endif

namespace kaleidoscope {
namespace plugin {

class MacroSupport : public Plugin {
 public:
  /// Send a key press event from a Macro
  ///
  /// Generates a new `KeyEvent` and calls `Runtime.handleKeyEvent()` with the
  /// specified `key`, then stores that `key` in an array of active macro key
  /// values. This allows the macro to press one key and keep it active when a
  /// subsequent key event is sent as part of the same macro sequence.
  void press(Key key);

  /// Send a key release event from a Macro
  ///
  /// Generates a new `KeyEvent` and calls `Runtime.handleKeyEvent()` with the
  /// specified `key`, then removes that key from the array of active macro
  /// keys (see `Macros.press()`).
  void release(Key key);

  /// Clear all virtual keys held by Macros
  ///
  /// This function clears the active macro keys array, sending a release event
  /// for each key stored there.
  void clear();

  /// Send a key "tap event" from a Macro
  ///
  /// Generates two new `KeyEvent` objects, one each to press and release the
  /// specified `key`, passing both in sequence to `Runtime.handleKeyEvent()`.
  void tap(Key key) const;

  /// Play a macro.
  ///
  /// This function reads a sequence of macro steps from memory and
  /// presses/releases/taps keys as instructed.
  ///
  /// This is intended to be called from other plugins.
  /// Macros, DynamicMacros, and EphemeralMacros are core plugins that
  /// use this method and implement the `Accessor` template type.
  ///
  /// The Accessor must provide the following fields and methods:
  ///
  /// * `uint8_t readByte()`. This method reads one byte and advances.
  ///   The first returned value should be a `MacroActionStepType`.
  ///   Depending on the step type, this is followed by zero or more bytes
  ///   containing key flag(s), key code(s), or a delay time (in
  ///   milliseconds).
  ///
  /// * `static constexpr bool had_boundary`. If true, then it must also
  ///   provide an `bool isEnd()` method. Otherwise the end of the macro
  ///   is indicated by a MACRO_STEP_END value (for the main loop), or a
  ///   Key_NoKey value (for a MACRO_ACTION_STEP_TAP_SEQUENCE loop).
  template<typename Accessor>
  void play(Accessor &accessor, uint16_t default_interval = 0)
  // Template function body must be defined in header:
  {
    macro_t macro = MACRO_ACTION_END;
    Key key;
    uint16_t interval = default_interval;

    auto setKeyAndAction = [&key, &macro, &accessor, this]() {
      uint8_t flags   = (macro == MACRO_ACTION_STEP_KEYCODEDOWN ||
                       macro == MACRO_ACTION_STEP_KEYCODEUP ||
                       macro == MACRO_ACTION_STEP_TAPCODE)
                          ? 0
                          : accessor.readByte();
      uint8_t keycode = accessor.readByte();

      key.setFlags(flags);
      key.setKeyCode(keycode);

      switch (macro) {
      case MACRO_ACTION_STEP_KEYCODEDOWN:
      case MACRO_ACTION_STEP_KEYDOWN:
        press(key);
        break;
      case MACRO_ACTION_STEP_KEYCODEUP:
      case MACRO_ACTION_STEP_KEYUP:
        release(key);
        break;
      case MACRO_ACTION_STEP_TAP:
      case MACRO_ACTION_STEP_TAPCODE:
        tap(key);
        break;
      default:
        break;
      }
    };

    // Main playback loop - always loops until explicit break/return
    while (true) {
      // Boundary check *before* reading,
      // only for accessors that need it (SRAM/EEPROM).
      if constexpr (Accessor::has_boundary)
        if (accessor.isEnd())
          break;  // Reached end of buffer/storage

      macro = accessor.readByte();

      // Check for explicit end marker *after* reading (primary check for PGM)
      if (macro == MACRO_ACTION_END) {
        break;  // Normal termination
      }

      switch (macro) {
      case MACRO_ACTION_STEP_EXPLICIT_REPORT:
      case MACRO_ACTION_STEP_IMPLICIT_REPORT:
      case MACRO_ACTION_STEP_SEND_REPORT:
        break;

      case MACRO_ACTION_STEP_INTERVAL:
        interval = accessor.readByte();
        break;

      case MACRO_ACTION_STEP_WAIT: {
        uint8_t wait = accessor.readByte();
        // NOTE: If reading 'wait' consumes the last byte for a boundary-checking
        // accessor, the loop will terminate correctly on the next iteration's check.
        delay(wait);
        break;
      }

      case MACRO_ACTION_STEP_KEYDOWN:
      case MACRO_ACTION_STEP_KEYUP:
      case MACRO_ACTION_STEP_TAP:
      case MACRO_ACTION_STEP_KEYCODEDOWN:
      case MACRO_ACTION_STEP_KEYCODEUP:
      case MACRO_ACTION_STEP_TAPCODE:
        setKeyAndAction();
        // NOTE: Similar to WAIT, if reading key data consumes the last byte,
        // the loop terminates on the next iteration's boundary check (if applicable).
        break;

      case MACRO_ACTION_STEP_TAP_SEQUENCE:
      case MACRO_ACTION_STEP_TAP_CODE_SEQUENCE: {
        bool isKeycodeSequence = macro == MACRO_ACTION_STEP_TAP_CODE_SEQUENCE;
        while (true) {
          if constexpr (Accessor::has_boundary)
            if (accessor.isEnd())
              break;  // Reached end of buffer/storage during sequence

          uint8_t flags   = isKeycodeSequence ? 0 : accessor.readByte();
          uint8_t keycode = accessor.readByte();

          key.setFlags(flags);
          key.setKeyCode(keycode);

          // Check for end-of-sequence marker
          if (key == Key_NoKey) break;

          tap(key);
          delay(interval);
        }
      }

      MACRO_ACTION_END:
      default:  // Unknown action treated like end
        return;
      }

      delay(interval);
    }
  }

  // ---------------------------------------------------------------------------
  // Event handlers
  EventHandlerResult onNameQuery();
  EventHandlerResult beforeReportingState(const KeyEvent &event);

 private:
  // An array of key values that are active while a macro sequence is playing
  Key active_macro_keys_[MAX_CONCURRENT_MACRO_KEYS];
};

}  // namespace plugin
}  // namespace kaleidoscope

extern kaleidoscope::plugin::MacroSupport MacroSupport;
