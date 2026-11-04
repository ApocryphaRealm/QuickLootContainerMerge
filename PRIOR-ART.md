# Prior art for QuickLoot Container Merge (searched 2026-11-04, rule 24)

Searched Nexus (skyrimspecialedition, GraphQL `nameStemmed` MATCHES: quickloot, loot merge, area loot, aoe loot, loot nearby,
nearby containers, container merge, loot all nearby, multi loot, mass loot, looting nearby, corpse merge, loot radius, and the
single stems loot, looting, corpse, corpses, container, containers, filtered on merge/nearby/area/radius/combine) and GitHub
(repositories: quickloot, LootMergeSKSE, loot merge skyrim, area loot skyrim, loot-merge; code: QuickLootAPI.h, LootMergeSKSE,
QuickLootIE corpses).

## The one that does nearly this

**Loot Merge SKSE**, version 1, by Elzar125, Nexus 193509 (2026-10-02, 662 downloads). Merges nearby corpses and containers into
one menu, in the game's own container menu and, through its API, in QuickLoot IE 4.x. 1.5.97, 1.6 and 1.7. Source zip in its
Miscellaneous files (file 813488); no licence stated on the page; not downloaded or read for this mod. No GitHub repository found.

Where it differs from what the owner asked for, by its own page:

| Asked | Loot Merge SKSE |
|---|---|
| Stolen or owned items respected: red, crime as normal | "Never merged: owned containers that would count as stealing, including containers in inns" |
| Line of sight optional | No such setting |
| Containers and bodies in one window | Separate by default: "Combine corpses and containers" defaults to off |
| One setting | Five: corpses, containers, combine, radius, max sources |

It needs the same QuickLoot IE 4.x as this mod. So: the owner's "steal it from the pile, in red" is what this mod adds.

## QuickLoot itself

| Mod | Where | Note |
|---|---|---|
| QuickLoot IE 4.1.3 | Nexus 120075; github.com/MissCorruption/QuickLootIE | The base. API `include/QuickLootAPI.h` (MIT at 4.x, the rest GPL-3.0-or-later since 2026-08-11); `ModifyInventoryEvent` adds rows, `TakingItemEvent` lets a plugin take them. No merge of its own |
| QuickLoot IE 3.4.1 | The version ticked in Njordlinger Test | MIT. Its API (v1.0) has events only: no way to add a row |
| QuickLoot EE | Nexus 69980 | The ancestor; no API for this |
| QuickLoot IE example plugin | github.com/AtomCrafty/QuickLootIE-ExamplePlugin, GPL-3.0 | Shows the API; its comment names ModifyInventory plus TakingItem as the way to handle inserted rows |
| Junk It NG, ShowUnlootedSKSE, HiddenLoot, No Loot While Armed, QuickPocket | GitHub | Use the QuickLoot API for other purposes; none merges sources |

Nothing else on Nexus or GitHub merges sources (AoE-loot results on GitHub are World of Warcraft servers; "Crafting Pulls Resources
From Containers", Nexus 135613, is crafting; the Witcher 3 "Loot Merger" is another game).
