# QuickLootContainerMerge - changelog

Rule 61: this mod's own history, kept beside the code it describes.

## 1.1.0 - 2026-11-06 - untested

### Added
- Setting bIncludeStealing (default on, as 1.0.0): off, a nearby container that belongs to someone else adds nothing, and any other merged row that would be stealing is dropped, so nothing in the merged part of the list is red. What the crosshair is on stays QuickLoot IE's own list. On the page as 'Include items you'd have to steal'.
- Setting bMixBodiesAndContainers (default off, as 1.0.0): on, a body also lists the containers nearby and a container the bodies, in one list. Locked containers, merchant chests, the player's own storage, summoned creatures and everything else 1.0.0 left out still stay out. On the page as 'Mix bodies and containers'.

### Changed
- Debug logging: every reference found in range is logged at debug as merged (body or container, and whether it is the other kind) or left out with its reason (the other kind while mixing is off, would be stolen, locked, merchant chest...), each would-be-stolen row dropped is logged, and the list summary names both settings. DevBench qcm.control reports mixBodiesAndContainers, includeStealing and stealRowsDropped, and op=set takes both new keys. The two labels and tooltips are in all eleven languages.

## 1.0.0 - 2026-11-04 - working

### Added
- QuickLoot IE's window also lists the items of others of its kind within a radius of what the crosshair is on, across the loaded cells: a body lists the other dead bodies, a container the other containers, and the two never mix.
- Each merged row is taken from its true source by the calls QuickLoot IE's own take makes, with its red colour, its Steal button and its crime worked out for that source. Take All on something free leaves red rows in place.
- Settings: bEnabled, fInteriorFeet (default 10) and fExteriorFeet (default 35) around the player, bLineOfSight (default off); page in the Apocrypha Menu Framework, eleven languages, DevBench tool qcm.control.
