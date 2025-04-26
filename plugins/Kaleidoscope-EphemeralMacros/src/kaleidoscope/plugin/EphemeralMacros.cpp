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

#include "kaleidoscope/KeyAddr.h"            // for KeyAddr
#include "kaleidoscope/KeyEvent.h"           // for KeyEvent
#include "kaleidoscope/keyswitch_state.h"    // for keyToggledOn
#include "kaleidoscope/plugin/MacroSteps.h"  // for macro_t, MACRO_ACTION_END, MACRO_ACTION_STEP_...

namespace kaleidoscope {
namespace plugin {

// =============================================================================

EphemeralMacros::EphemeralMacros()
  : interval_millis_(MACRO_TAP_DELAY) {}

void EphemeralMacros::initializeBuffer(void *buffer, size_t size) {
  buffer_     = reinterpret_cast<macro_t *>(buffer);
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
  return recordKey(event.key, event.addr, is_key_down);
}

bool EphemeralMacros::recordKey(Key key, KeyAddr addr, bool is_key_down) {
  if (!is_key_down && prev_keydown_addr_ == addr) {
    // Record a tap if a key is up immediately after down.
    // Downstream plugins can mutate the event's key, but the addr is distinct.
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
    prev_keydown_addr_ = KeyAddr::none();
    prev_keydown_key_  = Key_NoKey;
    return true;
  }

  // Flush any previously-buffered keydown event, which is not a tap.
  if (prev_keydown_addr_ != KeyAddr::none()) {
    Key buffered       = prev_keydown_key_;
    prev_keydown_addr_ = KeyAddr::none();
    prev_keydown_key_  = Key_NoKey;
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
    prev_keydown_addr_ = addr;
    prev_keydown_key_  = key;
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
      if (!recordKey(key, KeyAddr::none(), /* is_key_down= */ false)) {
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
        recording_         = true;
        prev_keydown_key_  = Key_NoKey;
        prev_keydown_addr_ = KeyAddr::none();
        pos_               = 0;
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

// Accessor for MacroSupport::play() to read SRAM buffer.
struct SramAccessor {
  const uint8_t *current;
  const uint8_t *end;
  static constexpr bool has_boundary = true;

  SramAccessor(const uint8_t *buf_start, size_t buf_size)
    : current(buf_start), end(buf_start + buf_size) {}

  // Read the next byte and advance
  inline uint8_t readByte() {
    return *current++;
  }

  inline bool isEnd() const {
    return current >= end;
  }
};

void EphemeralMacros::play() {
  SramAccessor accessor(buffer_, max_length_);
  ::MacroSupport.play(accessor, interval_millis_);
}

EventHandlerResult EphemeralMacros::onNameQuery() {
  return ::Focus.sendName(F("EphemeralMacros"));
}

}  // namespace plugin
}  // namespace kaleidoscope

kaleidoscope::plugin::EphemeralMacros EphemeralMacros;
