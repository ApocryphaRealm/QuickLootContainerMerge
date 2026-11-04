#pragma once

// The settings page in the Apocrypha Menu Framework (SKSE Menu Framework-compatible). Changes apply the next time the loot window
// builds its list and are written to QuickLootContainerMerge.ini. Without a framework the INI is the settings surface.
namespace page
{
	void Register();  // kPostPostLoad
}
