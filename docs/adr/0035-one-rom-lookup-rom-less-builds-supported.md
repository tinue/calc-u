# One ROM lookup; a build without ROMs is supported

Status: accepted (2026-11-09)

A core never contains ROM data. The app session loads every ROM through
one lookup in the library: first the ROMs bundled with the product, then a
ROM folder the user chooses. A product built without ROMs starts, says
which ROMs are missing and where they go, and boots once they are there;
the same lookup lets users use their own dumps. This is the design that
makes the ROMs removable (Briefing §8); a ROM-less build is documented,
not shipped now.
