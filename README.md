# Dynamic Time/Date/Countdown

Dynamic Time/Date/Countdown is a third-party OBS Studio filter bundle for text
sources.

It includes two filters:

- `Dynamic Time/Date`
- `Dynamic Countdown`

From version `1.3.0` onward, the `Next Stream` dock is no longer part of this
package and is distributed separately as `dynamic-next-stream`.

## Features

### Dynamic Time/Date

- Configurable update interval.
- Optional display of ISO week number.
- Optional weekday, date, time, seconds, and clock suffix output.
- Update diff checks to avoid unnecessary source updates.

### Dynamic Countdown

- Countdown to or since a target date.
- Calendar-correct year and month calculations.
- Custom prefix and suffix for before and after the target date.
- Optional seconds display.
- Optional TXT file export with diff checks.

## Requirements

- OBS Studio 30.x, 31.x, or 32.x
- Windows x64 for the packaged release
- No Qt runtime dependency beyond what OBS already provides

## Installation

### Windows

Download the release archive and extract or copy its contents into your OBS
Studio installation directory.

The final layout currently uses the technical package id `dynamic-texts`:

```text
obs-plugins/64bit/dynamic-texts.dll
data/obs-plugins/dynamic-texts/locale/en-US.ini
data/obs-plugins/dynamic-texts/locale/de-DE.ini
```

The packaged release also includes `INSTALL.bat`, which can copy the plugin
files into a selected OBS directory.

Restart OBS after installation. The filters appear in the filter menu for
`Text (GDI+)` and `Text (FreeType 2)` sources.

## Basic Usage

1. Create or select an OBS text source.
2. Open the source filters.
3. Add either `Dynamic Time/Date` or `Dynamic Countdown`.
4. Configure the output format and timing options.

## Version History

### 1.3.0

- Renamed the public plugin bundle to `Dynamic Time/Date/Countdown`.
- Split the `Next Stream` dock into the separate `dynamic-next-stream` plugin.
- Kept this package focused on text filters only.
- Reduced size and removed the dock-specific Qt dependency path.

### 1.2.0

- Added the original combined `Next Stream` dock before it was split out.

### 1.1.0

- Added `Dynamic Countdown`.
- Renamed the technical package id from `dynamic-time-date-filter` to
  `dynamic-texts`.

### 1.0.0

- Initial release of `Dynamic Time/Date`.

## License

Dynamic Time/Date/Countdown is licensed under GPL-2.0-or-later.

## Disclaimer

Dynamic Time/Date/Countdown is an unofficial third-party plugin and is not
affiliated with or endorsed by the OBS Project.

AI-assisted tools were used during development and release preparation. The
maintainer is responsible for reviewing, testing, and publishing the released
plugin.
