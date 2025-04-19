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

#pragma once

#include <Kaleidoscope-Ranges.h>  // for RECORD_MACRO
#include <stdint.h>               // for uint16_t, uint8_t, size_t

#include "kaleidoscope/KeyAddrBitfield.h"       // for KeyAddrBitfield
#include "kaleidoscope/KeyEvent.h"              // for KeyEvent
#include "kaleidoscope/event_handler_result.h"  // for EventHandlerResult
#include "kaleidoscope/key_defs.h"              // for Key
#include "kaleidoscope/plugin.h"                // for Plugin
#include "kaleidoscope/plugin/MacroSteps.h"     // for macro_t

constexpr Key Key_RecordMacro       = Key(kaleidoscope::ranges::RECORD_MACRO);
constexpr Key Key_PlayRecordedMacro = Key(kaleidoscope::ranges::PLAY_RECORDED_MACRO);

namespace kaleidoscope {
namespace plugin {

class EphemeralMacros : public kaleidoscope::Plugin {
 public:
  void initializeBuffer(void *buffer, size_t size);
  void play();

  void setStandardInterval(uint16_t interval_millis) {
    interval_millis_ = interval_millis;
  }

  EventHandlerResult onNameQuery();
  EventHandlerResult onKeyEvent(KeyEvent &event);

 private:
  bool recordKey(const KeyEvent &event);
  bool recordKey(Key key, bool is_key_down);
  bool saveStep(macro_t step, uint8_t arg1);
  bool saveStep(macro_t step, uint8_t arg1, uint8_t arg2);
  bool saveStep(macro_t step, const uint8_t *args, size_t args_size);
  void failRecording();
  bool flushLiveKeys();

  macro_t *buffer_          = nullptr;
  size_t max_length_        = 0;
  bool recording_           = false;
  size_t pos_               = 0;
  Key previous_keydown_     = Key_NoKey;
  uint16_t interval_millis_ = 25;
};

}  // namespace plugin
}  // namespace kaleidoscope

extern kaleidoscope::plugin::EphemeralMacros EphemeralMacros;
