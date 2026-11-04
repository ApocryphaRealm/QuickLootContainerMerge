# QuickLootContainerMerge - changelog

Rule 61: this mod's own history, kept beside the code it describes.

## 1.0.0 - 2026-11-04 - untested

### Added
- QuickLoot IE's window also lists the items of the other containers and dead bodies within a radius of what the crosshair is on, in the same cell, through QuickLoot IE 4.1's public API. Locked containers, summoned actors, merchant chests, QuickLootIE_Exclude and the player's own storage stay out; empty sources add nothing.
- Each merged row is taken from its true source by the calls QuickLoot IE's own take makes, with its red colour, its Steal button and its crime worked out for that source. Take All on something free leaves red rows in place.
- Settings: bEnabled, fRadius (default 500), bLineOfSight (default off); page in the Apocrypha Menu Framework, eleven languages, DevBench tool qcm.control.
