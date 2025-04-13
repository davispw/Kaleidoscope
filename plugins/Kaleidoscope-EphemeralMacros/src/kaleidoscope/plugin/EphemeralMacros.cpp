/* Kaleidoscope-EphemeralMacros -- Record macros on the fly
 * Copyright 2025 Keyboard.io, inc.
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

#include "kaleidoscope/plugin/EphemeralMacros.h"

#include <Kaleidoscope-FocusSerial.h>   // for Focus
#include <Kaleidoscope-MacroSupport.h>  // for MacroSupport
#include <Kaleidoscope-Ranges.h>        // for RECORD_MACRO, PLAY_RECORDED_MACRO
#include <stdint.h>                     // for uint8_t, uint16_t, size_t

#include "kaleidoscope/KeyEvent.h"         // for KeyEvent
#include "kaleidoscope/keyswitch_state.h"  // for keyToggledOn
// This is a special exception to the rule of only including a plugin's
// top-level header file, because DynamicMacros doesn't depend on the Macros
// plugin itself; it's just using the same macro step definitions.
#include "kaleidoscope/plugin/Macros/MacroSteps.h"  // for macro_t, MACRO_ACTION_END, MACRO_ACTION_STEP_...

namespace kaleidoscope {
namespace plugin {

// =============================================================================

void EphemeralMacros::initializeBuffer(void *buffer, size_t size) {
  buffer_     = (macro_t *)buffer;
  max_length_ = size;
  pos_        = 0;
  if (buffer != nullptr) {
    buffer_[0] = MACRO_ACTION_END;
  }
}

bool EphemeralMacros::recordKey(const KeyEvent &event) {
  // Only record valid key down/up events.
  bool is_key_down = keyToggledOn(event.state);
  bool is_key_up   = keyToggledOff(event.state);
  if (!is_key_down && !is_key_up || event.key == Key_NoKey)
    return true;
  return recordKey(event.key, is_key_down);
}

bool EphemeralMacros::recordKey(Key key, bool is_key_down) {
  if (!is_key_down && previous_keydown_ == key) {
    // Record a tap if a key is up immediately after down.
    // This saves a little memory and reduces interval delay for simple taps.
    if (key.getFlags() == 0) {
      if (!saveStep(MACRO_ACTION_STEP_TAPCODE, key.getKeyCode())) {
        return false;
      }
    } else {
      if (!saveStep(MACRO_ACTION_STEP_TAP, key.getFlags(), key.getKeyCode())) {
        return false;
      }
    }
    previous_keydown_ = Key_NoKey;
    return true;
  }

  // Flush any previously-buffered keydown event, which is not a tap.
  if (previous_keydown_ != Key_NoKey) {
    Key buffered      = previous_keydown_;
    previous_keydown_ = Key_NoKey;
    if (buffered.getFlags() == 0) {
      if (!saveStep(MACRO_ACTION_STEP_KEYCODEDOWN, buffered.getKeyCode())) {
        return false;
      }
    } else {
      if (!saveStep(MACRO_ACTION_STEP_KEYDOWN, buffered.getFlags(), buffered.getKeyCode())) {
        return false;
      }
    }
    // Continue to process the current event.
  }

  // Buffer keydown events.
  // They'll be either flushed or converted to a tap on the next event.
  if (is_key_down) {
    previous_keydown_ = key;
    return true;
  }

  // The remaining possibility is a keyup event that isn't a tap.
  // Record it.
  if (key.getFlags() == 0) {
    if (!saveStep(MACRO_ACTION_STEP_KEYCODEUP, key.getKeyCode())) {
      return false;
    }
  } else {
    if (!saveStep(MACRO_ACTION_STEP_KEYUP, key.getFlags(), key.getKeyCode())) {
      return false;
    }
  }
  return true;
}

bool EphemeralMacros::saveStep(const macro_t step, uint8_t arg1) {
  uint8_t args[]{arg1};
  return saveStep(step, args, 1);
}

bool EphemeralMacros::saveStep(const macro_t step, uint8_t arg1, uint8_t arg2) {
  uint8_t args[]{arg1, arg2};
  return saveStep(step, args, 2);
}

bool EphemeralMacros::saveStep(macro_t step, const uint8_t *args, size_t args_size) {
  // Ensure room for: step, args, and trailing MACRO_ACTION_END sentinel.
  if (buffer_ == nullptr)
    return false;
  if (pos_ + (2 * sizeof(macro_t)) + args_size >= max_length_)
    return false;

  buffer_[pos_++] = step;
  if (args != nullptr) {
    memcpy(buffer_ + pos_, args, args_size);
    pos_ += args_size;
  }

  // Always maintain this sentinel.
  // This will be overwritten if another step is recorded,
  // so don't increment pos_.
  buffer_[pos_] = MACRO_ACTION_END;
  return true;
}

void EphemeralMacros::failRecording() {
  // 1. Prevent subsequent steps from being recorded, no matter their length.
  // 2. Erase the macro.
  // This prevents stuck keys on playback of an incomplete macro.
  pos_ = max_length_;
  if (buffer_ != nullptr) {
    buffer_[0] = MACRO_ACTION_END;
  }
}

bool EphemeralMacros::flushLiveKeys() {
  // Record keyUp events for all held keys.
  // This prevents stuck keys at the end of a macro.
  for (Key key : live_keys.all()) {
    if (key != Key_Inactive && key != Key_Masked) {
      if (!recordKey(key, /* is_key_down= */ false)) {
        return false;
      }
    }
  }
  return true;
}

// -----------------------------------------------------------------------------
EventHandlerResult EphemeralMacros::onKeyEvent(KeyEvent &event) {
  // Start and stop recording.
  if (event.key == Key_RecordMacro) {
    event.key = Key_NoKey;  // always consume
    if (keyToggledOn(event.state)) {
      if (!recording_) {
        // Start recording.
        recording_        = true;
        previous_keydown_ = Key_NoKey;
        pos_              = 0;
      } else {
        // End recording.
        recording_ = false;
        if (!flushLiveKeys()) {
          failRecording();
          return EventHandlerResult::ABORT;
        }
      }
    }
    return EventHandlerResult::EVENT_CONSUMED;
  }

  // Playback.
  if (event.key == Key_PlayRecordedMacro) {
    event.key = Key_NoKey;  // always consume
    if (keyToggledOn(event.state)) {
      // Prevent playback while recording.
      if (recording_) {
        return EventHandlerResult::ABORT;
      }
      play();
    }
    return EventHandlerResult::EVENT_CONSUMED;
  }

  // During recording, record steps.
  if (recording_ && !recordKey(event)) {
    failRecording();
    // Lacking a better way to signal an error to the user, let them
    // keep typing (this is also to prevent stuck keys).
    // Keep "recording" so a 2nd tap of Key_RecordMacro will "end"
    // the recording as the user expects.
    return EventHandlerResult::OK;
  }

  return EventHandlerResult::OK;
}

// TODO(davispw): refactor DynamicMacros.play() and Macros.play().
// The only difference is the method of memory access.
// Until then, some there are some MACRO_ACTION_* steps implemented here
// that aren't possible to record.
void EphemeralMacros::play() {

  macro_t macro     = MACRO_ACTION_END;
  uint16_t interval = interval_millis_;
  uint16_t pos;
  Key key;


  // Define a lambda function for common key operations to reduce redundancy
  auto setKeyAndAction = [this, &key, &macro, &pos]() {
  // Keycode variants of actions don't have flags to set, but we want to make sure
  // we're still initializing them properly.

  key.setFlags((macro == MACRO_ACTION_STEP_KEYCODEDOWN || macro == MACRO_ACTION_STEP_KEYCODEUP || macro == MACRO_ACTION_STEP_TAPCODE) ? 0
                                                                                                                                      : buffer_[pos++]);

  key.setKeyCode(buffer_[pos++]);

  switch (macro) {
    case MACRO_ACTION_STEP_KEYCODEDOWN:
    case MACRO_ACTION_STEP_KEYDOWN:
      ::MacroSupport.press(key);
      break;
    case MACRO_ACTION_STEP_KEYCODEUP:
    case MACRO_ACTION_STEP_KEYUP:
      ::MacroSupport.release(key);
      break;
    case MACRO_ACTION_STEP_TAP:
    case MACRO_ACTION_STEP_TAPCODE:
      ::MacroSupport.tap(key);
      break;
    default:
      break;
    }
  };


  pos = 0;

  while (pos < pos_) {
    switch (macro = buffer_[pos++]) {
    case MACRO_ACTION_STEP_EXPLICIT_REPORT:
    case MACRO_ACTION_STEP_IMPLICIT_REPORT:
    case MACRO_ACTION_STEP_SEND_REPORT:
      break;

    case MACRO_ACTION_STEP_INTERVAL:
      interval = buffer_[pos++];
      break;
    case MACRO_ACTION_STEP_WAIT: {
      uint8_t wait = buffer_[pos++];
      delay(wait);
      break;
    }

    case MACRO_ACTION_STEP_KEYDOWN:
    case MACRO_ACTION_STEP_KEYUP:
    case MACRO_ACTION_STEP_TAP:
    case MACRO_ACTION_STEP_KEYCODEUP:
    case MACRO_ACTION_STEP_TAPCODE:
    case MACRO_ACTION_STEP_KEYCODEDOWN:
      setKeyAndAction();
      break;

    case MACRO_ACTION_STEP_TAP_SEQUENCE:
    case MACRO_ACTION_STEP_TAP_CODE_SEQUENCE: {
      bool isKeycodeSequence = macro == MACRO_ACTION_STEP_TAP_CODE_SEQUENCE;
      while (true) {
        key.setFlags(isKeycodeSequence ? 0 : buffer_[pos++]);
        key.setKeyCode(buffer_[pos++]);
        if (key == Key_NoKey || pos >= pos_)
          break;
        ::MacroSupport.tap(key);
        delay(interval);
      }
      break;
    }
    default:
    case MACRO_ACTION_END:
      return;
    }

    delay(interval);
  }
}

EventHandlerResult EphemeralMacros::onNameQuery() {
  return ::Focus.sendName(F("EphemeralMacros"));
}

}  // namespace plugin
}  // namespace kaleidoscope

kaleidoscope::plugin::EphemeralMacros EphemeralMacros;
