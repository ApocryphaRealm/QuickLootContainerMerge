QuickLoot Container Merge
Version 1.0.0

Look at a chest or a body, and QuickLoot IE's window also lists what the others of its kind around it hold: a body the other bodies, a chest the other containers. One window for
the whole pile of bandits.

WHAT GOES IN THE LIST
On a body, the other dead bodies near you; on a container, the other containers near you. With Mix bodies and containers switched on,
both kinds near you share the list, whichever you look at. Near means 10 feet indoors and 35 feet outdoors by default. Empty ones add nothing.
Locked containers, summoned creatures, merchant chests, anything marked QuickLootIE_Exclude and your own storage stay out, with
mixing on or off.

TAKING
Every row comes from the container or body it belongs to, through the same calls QuickLoot IE uses. A row that would be stealing
is red for its own owner, the button says Steal, and taking it is a crime as usual: the item is marked stolen and
a witness reports it. Take All on something free takes the free rows and leaves the red ones for you to take one by one. Take All
on something already owned by someone else takes everything, red rows too. Switch off Include items you'd have to steal and the red
rows never appear: nearby containers that belong to someone else add nothing, and any other row that would be stealing is left
out. What the crosshair is on still shows as QuickLoot IE shows it. Closing the window or looking away works as
QuickLoot IE does.

SETTINGS
QuickLoot Container Merge in the Apocrypha Menu Framework: on or off, the indoor and outdoor distances, whether only what you
can see is listed, whether bodies and containers mix (off by default), and whether items you would have to steal are listed (on
by default). All of it is also in SKSE\Plugins\QuickLootContainerMerge.ini, so the mod works the same
without a menu framework.

REQUIREMENTS
SKSE, Address Library, and QuickLoot IE 4.1 or newer (Nexus 120075). QuickLoot IE 3.x has no way for another mod to add rows, so
with it this mod does nothing and says so on its page and in its log. Skyrim 1.5.97, 1.6 or 1.7: the installer picks the build.

KNOWN LIMITS
The same item from two sources is two rows. The window's title names what the crosshair is on. A body just across an outdoor cell
border is not listed until you look at something on its side. Search opens only what the crosshair is on.

CREDITS
QuickLoot IE (MissCorruption and AtomCrafty, Nexus 120075), whose public API this mod is built on. CommonLibSSE NG and its
contributors, and the libraries listed in THIRD_PARTY_NOTICES.md. Inspired by The Witcher 3: Wild Hunt (next-gen / Remastered),
credit for the idea only.

SOURCE
GPL-3.0-or-later. https://github.com/ApocryphaRealm/QuickLootContainerMerge
