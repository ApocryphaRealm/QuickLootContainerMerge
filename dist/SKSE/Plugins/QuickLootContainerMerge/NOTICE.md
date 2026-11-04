# QuickLoot Container Merge - copyright and licence

Copyright (C) 2026 ApocryphaRealm

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program (`LICENSE`). If not, see
<https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later

## Why GPL-3.0-or-later

The Skyrim 1.7.x build statically links CommonLibSSE-NG 7.2.0 (https://github.com/alandtse/CommonLibSSE-NG, commit
7a60f4de794095d7b0f8928d1b930a52e9a7da83), GPL-3.0-or-later WITH its Modding Exception and GPL-3.0 Linking Exception,
whose README requires a linking plugin to be GPL-3.0-or-later or GPL-compatible. The whole work, both build lines and
the source, is GPL-3.0-or-later.

Components under other licences, with their notices: `THIRD_PARTY_NOTICES.md`.

## QuickLoot IE

QuickLoot IE (MissCorruption and AtomCrafty, Nexus 120075, https://github.com/MissCorruption/QuickLootIE) is a separate, required
mod and ships nothing here. This mod uses its public API header, `include/QuickLootAPI.h` from tag 4.1.3 (commit
43baf0cc4d44fdcb89482d10050106d38f6595ae), which that project keeps under MIT and which is vendored unmodified. `source/Merge.cpp`
follows the shape of its GPL-3.0-or-later `ItemStack::Take`, `ItemStack::IsStealing`, `Inventory::LoadContainerInventory` and
`LootMenu::CanDisplay`, so that a merged row behaves as that mod's own row does; both are GPL-3.0-or-later works, and the credit
is given here.

Source code: https://github.com/ApocryphaRealm/QuickLootContainerMerge
