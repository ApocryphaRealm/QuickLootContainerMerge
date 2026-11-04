// QuickLoot Container Merge - own code, GPL-3.0-or-later. QuickLoot IE's window also lists the items of the other containers and
// corpses around what the crosshair is on, each taken from its true source. See include/Merge.h and the head of source/Merge.cpp.
#include "PCH.h"

#include "DevBenchTool.h"
#include "Merge.h"
#include "Page.h"
#include "Settings.h"
#include "Strings.h"

#include "utils/AddressLibraryGuard.h"
#include "utils/Logger.h"

namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) { return; }
		switch (a_msg->type)
		{
		case SKSE::MessagingInterface::kPostLoad:
			DevBenchTool::Init(false);
			merge::Connect(false);
			break;
		case SKSE::MessagingInterface::kPostPostLoad:
			page::Register();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			qcm::Strings::Load();
			merge::Connect(true);
			merge::OnDataLoaded();
			DevBenchTool::Init(true);
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::log::init("QuickLootContainerMerge");
	// Address Library pre-check, BEFORE SKSE::Init, which opens the Address Library itself: a missing file gets a message
	// naming it and the plugin loads inert instead of CommonLibSSE-NG's bare failure line.
	if (!AddressLibraryGuard::Guard("QuickLoot Container Merge")) { return true; }
	SKSE::Init(a_skse);

	settings::Load();
	settings::ApplyLogLevel();
	SKSE::log::describe_level("QuickLootContainerMerge.ini");
	logger::info("QuickLoot Container Merge {} loading", SKSE::PluginDeclaration::GetSingleton()->GetVersion().string("."));

	auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler))
	{
		logger::error("could not register the SKSE message listener; nothing will merge");
		return true;
	}
	return true;
}
