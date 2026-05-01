Dynamic Texts - OBS Filter Plugin v1.3.0
========================================

Dynamic Texts is a filter bundle for OBS text sources.

Included filters:

  * Dynamic Time/Date
  * Dynamic Countdown

The `Next Stream` dock is no longer part of this package and is distributed
separately as `dynamic-next-stream`.


Package Contents
----------------

  dynamic-texts-1.3.0\
    INSTALL.bat
    README.txt
    obs-plugins\
      64bit\
        dynamic-texts.dll
        dynamic-texts.pdb
    data\
      obs-plugins\
        dynamic-texts\
          locale\
            en-US.ini
            de-DE.ini


Installation
------------

Recommended: double-click `INSTALL.bat`, choose your OBS folder, and let the
script copy the files for you.

Manual installation: copy BOTH top-level folders, `obs-plugins\` and `data\`,
into your OBS Studio installation directory.


Usage
-----

Add the filters to an OBS `Text (GDI+)` or `Text (FreeType 2)` source:

  Right-click source -> Filters -> "+" -> Dynamic Time/Date
  Right-click source -> Filters -> "+" -> Dynamic Countdown


Highlights
----------

Dynamic Time/Date:
  - configurable update interval
  - optional ISO week, weekday, date, time, seconds, and suffix output

Dynamic Countdown:
  - countdown to or since a target date
  - calendar-correct year and month calculations
  - optional TXT export


License
-------

GPLv2 or later.
