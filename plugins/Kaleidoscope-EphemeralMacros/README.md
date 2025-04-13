# EphemeralMacros

This plugin offers keys to record an ephemeral macro on the fly and play it
back. Like [Macros][plugin:macros] and [DynamicMacros][plugin:dynamicmacros],
this lets you record and play back a sequence of keystrokes. Unlike those
plugins, EphemeralMacros does so without any updates to firmware or
configuration.

 [plugin:macros]: Kaleidoscope-Macros.md
 [plugin:dynamicmacros]: Kaleidoscope-DynamicMacros.md

The recording isn't saved to the keyboard's storage and resets when power
is lost. While not completely secure, this lets you record and erase macros
whose contents you might not want to keep stored in non-volatile memory.

Like DynamicMacros, a recorded macro comes with certain limitations: recorded
macros do not support running custom code, and they can only play back a
sequence of key events. Additionally, recorded macros do not support different
delays between keys. You can configure a standard interval that will add a delay
between all keys during playback.

You can define a single ephemeral macro. There is no limit on the size except
the amount of SRAM you allocate on the keyboard. Be careful to leave enough RAM
for regular keyboard functions.

The number of keystrokes that can be recorded for a given allocated size varies.
Each key requires a minimum of 2 bytes for simple taps, or up to 6 bytes
for chords, modifiers, and special keys with separate key down and key up
events.

Recording can fail if there isn't enough memory. If this happens, the recording
is cleared.

## Using the plugin

To use the plugin, we need to include the header, initialize the plugin with
`KALEIDOSCOPE_INIT_PLUGINS()`, and reserve storage space for the macros. This
is best illustrated with an example:

```c++
#include <Kaleidoscope.h>
#include <Kaleidoscope-EphemeralMacros.h>

KALEIDOSCOPE_INIT_PLUGINS(
  EphemeralMacros
);

// Allocate RAM to store Ephemeral Macro recordings.
constexpr size_t ephemeralMacroSize = 128;
uint8_t ephemeralMacroBuffer[ephemeralMacroSize];

void setup() {
  Kaleidoscope.setup();

  // For Ephemeral Macros, we need to tell the plugin to use the allocated RAM.
  EphemeralMacros.initializeBuffer(ephemeralMacroBuffer, ephemeralMacroSize);
}
```

## Keymap markup

### `Key_RecordMacro`

> Places the record macro key on the keymap.
>
> Pressing the key will begin recording the macro.
> Any previously recorded macro is cleared.
>
> Pressing the key again will end recording the macro.
>
> While recording, type normally.
> 
> If the recording overflows the available memory, recording will stop
> and the recording will be cleared. There is no indication of failure
> except that when you play back the macro, nothing will happen.

### `Key_PlayRecordedMacro`

> Places a key on the keymap that will immediately play the recorded macro.

## Plugin methods

The plugin provides a `EphemeralMacros` object, with the following methods
available:

### `.initializeBuffer(buffer, size)`

> Tells the plugin to use the given buffer to hold recordings.
> `buffer` should point to an array of bytes that you declare as a global
> variable (in C++, the type is `uint8_t[]`).
>
> This must be called from the `setup()` method of your sketch,
> otherwise recording macros will not function.
>
> Be careful that the `size` argument is correct and matches your
> allocated array. Be careful not to declare the buffer inside the
> `setup()` method. Otherwise, the recording could corrupt the
> keyboard's memory.

### `.play()`

> Plays back a previously recorded macro. You can call this from your own
> code in lieu of adding the `Key_PlayRecordedMacro` key to your keymap.

### `.setStandardInterval(interval_millis)`

> Adds a delay (in milliseconds) between each key event during macro playback.
>
> The default standard interval is 25 milliseconds. This helps with some
> devices that can't handle very short intervals.
>
> Simple key taps count as one key event. When you hold multiple keys while
> recording (including modifiers like shift), each key down and key up event
> will get its own delay.

## Dependencies

* [Kaleidoscope-MacroSupport](Kaleidoscope-MacroSupport.md)
