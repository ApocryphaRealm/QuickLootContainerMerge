#pragma once

#include <cstdint>
#include <string>
#include <string_view>

// The merge itself. QuickLoot IE 4.1+ owns the window; this adds the rows of nearby containers and corpses to its list through
// its public API and takes each of those rows from the reference it came from.
namespace merge
{
	enum class ApiState
	{
		kNotChecked,    // before kPostLoad
		kNotInstalled,  // no QuickLootIE.dll loaded
		kTooOld,        // QuickLootIE.dll without the 4.1 interface (3.x has no way to add rows)
		kConnected
	};

	void Connect(bool a_lastAttempt);  // kPostLoad, then kDataLoaded as the retry (rule 17)
	void OnDataLoaded();               // the merchant chests and QuickLoot IE's exclude keyword, once forms exist
	ApiState Api();

	std::string StateJson();  // the live state for the DevBench tool (rule 31), as JSON members without the outer braces

	// DevBench's hands (rule 64), which go through QuickLoot IE's own API. Safe from any thread: they queue a game task.
	bool Action(std::string_view a_name);  // take, takeall, use, up, down, transfer
	bool ForceWindow(std::uint32_t a_formID);
	void ClearForcedWindow();
}
