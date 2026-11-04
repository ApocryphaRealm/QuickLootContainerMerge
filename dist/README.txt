QuickLoot Container Merge
Version 1.0.0

Look at a chest or a body, and QuickLoot IE's window also lists what the others of its kind around it hold: a body the other bodies, a chest the other containers. One window for
the whole pile of bandits.

WHAT GOES IN THE LIST
On a body, the other dead bodies near you; on a container, the other containers near you. The two never mix. Near means 10 feet indoors and 35 feet outdoors by default. Empty ones add nothing.
Locked containers, summoned creatures, merchant chests, anything marked QuickLootIE_Exclude and your own storage stay out.

TAKING
Every row comes from the container or body it belongs to, through the same calls QuickLoot IE uses. A row that would be stealing
is red for its own owner, the button says Steal, and taking it is a crime as usual: the item is marked stolen and
a witness reports it. Take All on something free takes the free rows and leaves the red ones for you to take one by one. Take All
on something already owned by someone else takes everything, red rows too. Closing the window or looking away works as
QuickLoot IE does.

SETTINGS
QuickLoot Container Merge in the Apocrypha Menu Framework: on or off, the indoor and outdoor distances, and
whether only what you can see is listed. All of it is also in SKSE\Plugins\QuickLootContainerMerge.ini, so the mod works the same
without a menu framework.

REQUIREMENTS
SKSE, Address Library, and QuickLoot IE 4.1 or newer (Nexus 120075). QuickLoot IE 3.x has no way for another mod to add rows, so
with it this mod does nothing and says so on its page and in its log. Skyrim 1.5.97, 1.6 or 1.7: the installer picks the build.

KNOWN LIMITS
The same item from two sources is two rows. The window's title names what the crosshair is on. A body just across an outdoor cell
border is not listed until you look at something on its side. Search opens only what the crosshair is on.

SOURCE
GPL-3.0-or-later. https://github.com/ApocryphaRealm/QuickLootContainerMerge
