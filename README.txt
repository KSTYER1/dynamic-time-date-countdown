Dynamic Texts — OBS Filter Plugin v1.3.0
==========================================

Bündel-Plugin mit zwei Filtern für portable OBS-Installationen (Windows x64).
Enthält:

  * Filter "Dynamic Time/Date"   — Uhrzeit, Datum, Wochentag, ISO-KW
  * Filter "Dynamic Countdown"   — Countdown zu/seit einem Datum

Das Plugin "Nächster Stream" (Dock) ist NICHT mehr enthalten — das ist
in einem separaten Paket "dynamic-next-stream" zu finden.


Inhalt
------

  dynamic-texts-1.3.0\
    INSTALL.bat                     <- Doppelklick = automatisches Kopieren
    README.txt                      <- diese Datei
    obs-plugins\
      64bit\
        dynamic-texts.dll           <- Plugin-Binary
        dynamic-texts.pdb           <- Debug-Symbole, optional
    data\
      obs-plugins\
        dynamic-texts\
          locale\
            en-US.ini
            de-DE.ini


Installation
------------

Empfohlen: Doppelklick auf INSTALL.bat, OBS-Pfad eingeben, fertig.

Manuell: BEIDE Top-Level-Ordner (obs-plugins\, data\) in den OBS-Hauptordner
kopieren — ohne den data\-Teil zeigt OBS rohe Schluessel statt Beschriftungen.


Filter-Verwendung
-----------------

Bei einer Text(GDI+)- oder Text(FreeType 2)-Quelle:
Rechtsklick → Filter → "+" → "Dynamic Time/Date" oder "Dynamic Countdown"


Eigenschaften
-------------

Dynamic Time/Date:
  - Konfigurierbares Update-Intervall (1-3600 s)
  - Komponenten ein-/ausschaltbar: Kalenderwoche (ISO 8601), Wochentag (DE),
    Datum (TT.MM.JJJJ), Uhrzeit (HH:MM), Sekunden, "Uhr"-Suffix
  - Wochentage hardcoded deutsch (locale-unabhaengig)
  - Diff-Check vor obs_source_update (kein Update-Spam)

Dynamic Countdown:
  - Zieldatum: TT-MM-JJJJ [HH:MM] (Trenner: '-' oder '.')
  - Kalenderkorrekte Jahres-/Monatsberechnung (kein 365/30-Tage-Drift)
  - Eigene Praefix/Suffix-Felder fuer "vor" und "nach" dem Zieldatum
  - Optional: Sekunden (klemmt Intervall auf 1 s)
  - Optional: TXT-Datei-Export mit Diff-Check


Versionshistorie
----------------

1.3.0 (2026-04-26)
  * Plugin gesplittet: Dock "Naechster Stream" jetzt in eigenem Paket
    "dynamic-next-stream". Dieses Paket enthaelt nur noch Filter.
  * Vorteil: keine Qt-Abhaengigkeit, DLL nur ~26 KB.

1.2.0 (intern, kombiniert)
  * Dock "Naechster Stream" hinzugefuegt (mittlerweile separat)

1.1.0
  * Filter "Dynamic Countdown" hinzugefuegt
  * Plugin-Name von dynamic-time-date-filter zu dynamic-texts geaendert

1.0.0
  * Filter "Dynamic Time/Date" mit ISO 8601 KW, deutschen Wochentagen


Lizenz: GPLv2 oder neuer.
