// QuickLoot Container Merge - own code, GPL-3.0-or-later.
//
// How the merge works, against QuickLoot IE 4.1's public API (include/QuickLootAPI.h, MIT):
//   ModifyInventory  when the window builds its list, the rows of every other allowed source of its kind within the distance around the player go in behind the
//                    rows of what the crosshair is on. Each row is a new InventoryEntryData, which QuickLoot IE then owns.
//   ModifyItemData   a row's red colour is worked out for ITS source, not for the container the window is showing.
//   ModifyButtonBar  the same for the Take/Steal button while such a row is selected.
//   TakingItem       QuickLoot IE would take the row from the container on the crosshair; for a merged row the event is cancelled
//                    and the row is taken from its true source by the same engine calls QuickLoot IE uses, with the same crime.
// Nothing is hooked and nothing is moved until the player takes a row.
#include "PCH.h"

#include "Merge.h"

#include "QuickLootAPI.h"
#include "Settings.h"
#include "utils/Logger.h"

#include <atomic>
#include <chrono>
#include <format>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace merge
{
	namespace
	{
		using namespace QuickLoot::API;

		struct Row
		{
			RE::ObjectRefHandle source;
			bool stealing = false;
		};

		struct SourceInfo
		{
			RE::FormID id = 0;
			std::string name;
			int rows = 0;
			int redRows = 0;
		};

		struct LeftOut
		{
			RE::FormID id = 0;
			std::string name;
			const char* why = "";
		};

		std::atomic<ApiState> g_api{ ApiState::kNotChecked };
		std::atomic<QuickLootAction> g_action{ QuickLootAction::kNone };  // the action QuickLoot IE is about to run (it runs in the same call)

		std::mutex g_lock;                                        // the two below: DevBench reads them from its listener thread
		std::unordered_map<RE::InventoryEntryData*, Row> g_rows;  // merged rows of the list now showing; keys are only compared
		struct
		{
			RE::FormID focus = 0;
			std::string focusName;
			std::vector<SourceInfo> sources;
			int refused = 0;
			std::vector<LeftOut> leftOut;
			double scanMs = 0.0;
			std::uint64_t scans = 0;
			std::uint64_t taken = 0;
			std::uint64_t leftRed = 0;
			std::uint64_t sourceGone = 0;
			int stealRowsDropped = 0;  // the last list: rows left out because taking them would be stealing (bIncludeStealing=0)
		} g_state;

		std::unordered_set<RE::FormID> g_merchantChests;
		RE::BGSKeyword* g_excludeKeyword = nullptr;

		bool IsPlayerAllowedToTake(RE::PlayerCharacter* a_player, RE::TESForm* a_owner, int a_value)
		{
			using func_t = decltype(&IsPlayerAllowedToTake);
			REL::Relocation<func_t> func{ RELOCATION_ID(39584, 40670) };
			return func(a_player, a_owner, a_value);
		}

		RE::ExtraDataList* ExtraListForRemoval(RE::InventoryEntryData* a_entry, int a_count, bool a_isViewingContainer)
		{
			using func_t = decltype(&ExtraListForRemoval);
			REL::Relocation<func_t> func{ RELOCATION_ID(50948, 51825) };
			return func(a_entry, a_count, a_isViewingContainer);
		}

		void RefreshEnchantedWeapons(RE::Actor* a_actor, RE::InventoryChanges* a_changes)
		{
			using func_t = decltype(&RefreshEnchantedWeapons);
			REL::Relocation<func_t> func{ RELOCATION_ID(50946, 51823) };
			func(a_actor, a_changes);
		}

		// What QuickLoot IE lists: the same test as its LootMenu::CanDisplay, so a merged source shows what it would show alone.
		bool CanDisplay(RE::TESBoundObject& a_object)
		{
			if (!a_object.GetPlayable()) { return false; }
			const char* name = a_object.GetName();
			if (!name || name[0] == '\0') { return false; }
			switch (a_object.GetFormType())
			{
			case RE::FormType::Scroll:
			case RE::FormType::Armor:
			case RE::FormType::Book:
			case RE::FormType::Ingredient:
			case RE::FormType::Misc:
			case RE::FormType::Weapon:
			case RE::FormType::Ammo:
			case RE::FormType::KeyMaster:
			case RE::FormType::AlchemyItem:
			case RE::FormType::Note:
			case RE::FormType::SoulGem:
				return true;
			case RE::FormType::Light:
				{
					const auto* light = a_object.As<RE::TESObjectLIGH>();
					return light && light->CanBeCarried();
				}
			default:
				return false;
			}
		}

		// Is taking this entry from this source a crime? The game's rule, as QuickLoot IE reads it per item: a dead actor never,
		// an unowned container never, otherwise the owner against what the player may take.
		bool RowIsStealing(RE::TESObjectREFR& a_source, RE::InventoryEntryData& a_entry)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) { return false; }
			if (const auto* actor = a_source.As<RE::Actor>(); actor && actor->IsDead(false)) { return false; }
			auto* owner = a_source.GetOwner();
			if (!owner) { return false; }  // no owner and no living actor: free; a living actor is not a source
			auto* itemOwner = a_entry.GetOwner();
			if (!itemOwner) { itemOwner = owner; }
			return !IsPlayerAllowedToTake(player, itemOwner, a_entry.GetValue());
		}

		// Does the container on the crosshair belong to someone else? Take All then already means stealing.
		bool FocusIsSomeoneElses(RE::TESObjectREFR* a_focus)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!a_focus || !player) { return false; }
			if (const auto* actor = a_focus->As<RE::Actor>(); actor && actor->IsDead(false)) { return false; }
			return a_focus->GetOwner() && player->WouldBeStealing(a_focus);
		}

		// Why a reference found in range is not a source; nullptr when it is one. a_viaAshPile: reached through an ash pile, whose
		// actor is disabled and has no 3D by design.
		const char* WhyRefused(RE::TESObjectREFR& a_ref, bool a_viaAshPile)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) { return "no player"; }
			if (a_ref.IsPlayerRef()) { return "the player"; }
			if (a_ref.IsDeleted()) { return "deleted"; }
			if (!a_viaAshPile && a_ref.IsDisabled()) { return "disabled"; }
			if (!a_viaAshPile && !a_ref.Is3DLoaded()) { return "no 3D"; }
			if (!a_ref.HasContainer()) { return "no container"; }
			if (a_ref.IsLocked()) { return "locked"; }
			if (a_ref.IsActivationBlocked()) { return "activation blocked"; }
			if (g_excludeKeyword && a_ref.HasKeyword(g_excludeKeyword)) { return "QuickLootIE_Exclude"; }
			if (g_merchantChests.contains(a_ref.GetFormID())) { return "merchant chest"; }  // often under the shop's floor
			const char* name = a_ref.GetDisplayFullName();
			if (!name || name[0] == '\0') { return "no name"; }

			if (auto* actor = a_ref.As<RE::Actor>())
			{
				if (!actor->IsDead()) { return "alive"; }
				if (actor->IsSummoned()) { return "summoned"; }
			}
			else if (a_ref.GetOwner() && !player->WouldBeStealing(&a_ref))
			{
				return "the player's own";  // a storage room would otherwise pour into one window, and Take All would empty it
			}
			else if (!settings::includeStealing.load() && a_ref.GetOwner())
			{
				return "would be stolen";  // someone else's container: every row of it would be a theft (bIncludeStealing=0)
			}

			if (settings::lineOfSight.load())
			{
				bool unused = false;
				if (!player->HasLineOfSight(&a_ref, unused)) { return "out of sight"; }
			}
			return nullptr;
		}

		// The allowed sources in range: the interior, or every loaded exterior cell, so a body over a cell border is found. The cells
		// are walked under their locks, so only handles are gathered inside.
		std::vector<RE::TESObjectREFRPtr> FindSources(RE::TESObjectREFR& a_focus, int& a_refused, std::vector<LeftOut>& a_leftOut)
		{
			std::vector<RE::TESObjectREFRPtr> sources;
			auto* tes = RE::TES::GetSingleton();
			if (!tes)
			{
				logger::debug("merge: no TES singleton; nothing merged");
				return sources;
			}

			std::vector<RE::ObjectRefHandle> inRange;
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player)
			{
				logger::debug("merge: no player; nothing merged");
				return sources;
			}
			const auto* cell = player->GetParentCell();
			const float feet = cell && cell->IsInteriorCell() ? settings::interiorFeet.load() : settings::exteriorFeet.load();
			tes->ForEachReferenceInRange(player, feet * settings::kUnitsPerFoot, [&inRange](RE::TESObjectREFR* a_ref) {
				if (a_ref && (a_ref->HasContainer() || a_ref->extraList.HasType(RE::ExtraDataType::kAshPileRef))) { inRange.push_back(a_ref->GetHandle()); }
				return RE::BSContainer::ForEachResult::kContinue;
			});

			// Bodies merge only with bodies, containers only with containers, so a corpse's list never fills up with the barrels around
			// it - unless bMixBodiesAndContainers is on, when both kinds share the list. Every other exclusion applies either way.
			const bool focusIsBody = a_focus.As<RE::Actor>() || a_focus.extraList.HasType(RE::ExtraDataType::kAshPileRef);
			const bool mix = settings::mixKinds.load();

			std::unordered_set<RE::FormID> seen{ a_focus.GetFormID() };
			for (const auto& handle : inRange)
			{
				auto ref = handle.get();
				if (!ref) { continue; }
				bool viaAshPile = false;
				if (!ref->HasContainer())
				{
					ref = ref->extraList.GetAshPileRef().get();
					viaAshPile = true;
					if (!ref) { continue; }
				}
				if (!seen.insert(ref->GetFormID()).second) { continue; }
				const bool isBody = viaAshPile || ref->As<RE::Actor>();
				const char* name = ref->GetDisplayFullName();
				if (const char* why = !mix && isBody != focusIsBody ? "the other kind (mixing is off)" : WhyRefused(*ref, viaAshPile))
				{
					logger::debug("merge: {:08X} \"{}\" left out: {}", ref->GetFormID(), name ? name : "", why);
					++a_refused;
					a_leftOut.push_back({ ref->GetFormID(), name ? name : "", why });
					continue;
				}
				logger::debug("merge: {:08X} \"{}\" merged: {}{}", ref->GetFormID(), name ? name : "", isBody ? "body" : "container",
					isBody != focusIsBody ? " (the other kind, mixing is on)" : "");
				sources.push_back(std::move(ref));
			}
			return sources;
		}

		// One source's rows, built as the game's container menu counts them: changed entries, plus the base container's counts unless
		// the entry is levelled, and every dropped item as a row of its own.
		void AddRows(RE::TESObjectREFR& a_source, RE::BSTArray<ItemStack>& a_inventory, SourceInfo& a_info, int& a_stealDropped)
		{
			const bool includeStealing = settings::includeStealing.load();
			auto* changes = a_source.GetInventoryChanges();
			if (auto* actor = a_source.As<RE::Actor>(); actor && changes) { RefreshEnchantedWeapons(actor, changes); }

			std::vector<RE::InventoryEntryData*> entries;
			std::unordered_map<RE::TESBoundObject*, RE::InventoryEntryData*> byObject;

			if (changes && changes->entryList)
			{
				for (auto* entry : *changes->entryList)
				{
					if (!entry || !entry->object || !CanDisplay(*entry->object)) { continue; }
					auto* copy = new RE::InventoryEntryData(*entry);
					byObject.emplace(entry->object, copy);
					entries.push_back(copy);
				}
			}

			if (auto* container = a_source.GetContainer())
			{
				container->ForEachContainerObject([&](RE::ContainerObject& a_object) {
					auto* object = a_object.obj;
					if (!object || object->Is(RE::FormType::LeveledItem) || !CanDisplay(*object)) { return RE::BSContainer::ForEachResult::kContinue; }
					if (const auto it = byObject.find(object); it != byObject.end())
					{
						if (!it->second->IsLeveled()) { it->second->countDelta += a_object.count; }
					}
					else
					{
						auto* fresh = new RE::InventoryEntryData(object, a_object.count);
						byObject.emplace(object, fresh);
						entries.push_back(fresh);
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}

			const auto sourceHandle = a_source.GetHandle();
			for (auto* entry : entries)
			{
				if (entry->countDelta <= 0)
				{
					delete entry;
					continue;
				}
				const bool stealing = RowIsStealing(a_source, *entry);
				if (stealing && !includeStealing)
				{
					// bIncludeStealing=0: a row that would be a theft (an item owned apart from its container) never reaches the list.
					logger::debug("merge: {:08X}: \"{}\" left out: would be stolen", a_source.GetFormID(), entry->GetDisplayName());
					++a_stealDropped;
					delete entry;
					continue;
				}
				a_inventory.push_back(ItemStack{ entry, {} });
				g_rows.emplace(entry, Row{ sourceHandle, stealing });
				++a_info.rows;
				if (stealing) { ++a_info.redRows; }
			}

			// A dropped item's row carries its own reference, and QuickLoot IE's own take picks it up wherever it lies.
			if (auto* dropped = a_source.extraList.GetByType<RE::ExtraDroppedItemList>())
			{
				for (const auto& handle : dropped->droppedItemList)
				{
					const auto ref = handle.get();
					if (!ref || ref->IsDeleted() || ref->IsDisabled()) { continue; }
					auto* object = ref->GetObjectReference();
					if (!object || !CanDisplay(*object)) { continue; }
					a_inventory.push_back(ItemStack{ new RE::InventoryEntryData(object, ref->extraList.GetCount()), handle });
					++a_info.rows;
				}
			}
		}

		void OnModifyInventory(ModifyInventoryEvent* a_event)
		{
			std::scoped_lock lock(g_lock);
			g_rows.clear();
			g_state.sources.clear();
			g_state.focus = 0;
			g_state.focusName.clear();
			g_state.refused = 0;
			g_state.leftOut.clear();
			g_state.stealRowsDropped = 0;

			if (!a_event || !settings::enabled.load()) { return; }
			const auto focus = a_event->container.get();
			if (!focus) { return; }

			const auto start = std::chrono::steady_clock::now();
			const char* focusName = focus->GetDisplayFullName();
			g_state.focus = focus->GetFormID();
			g_state.focusName = focusName ? focusName : "";

			const auto sources = FindSources(*focus, g_state.refused, g_state.leftOut);
			for (const auto& source : sources)
			{
				const char* name = source->GetDisplayFullName();
				SourceInfo info{ source->GetFormID(), name ? name : "" };
				AddRows(*source, a_event->inventory, info, g_state.stealRowsDropped);
				logger::debug("merge: {:08X} \"{}\": {} row(s), {} red", info.id, info.name, info.rows, info.redRows);
				if (info.rows > 0) { g_state.sources.push_back(std::move(info)); }  // an empty source adds nothing and is not counted
			}

			g_state.scanMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			++g_state.scans;
			logger::debug("merge: {:08X} \"{}\": {} source(s) with items, {} left out, {} would-be-stolen row(s) dropped, list now {} rows, {:.2f} ms (mix {}, include stealing {})",
				g_state.focus, g_state.focusName, g_state.sources.size(), g_state.refused, g_state.stealRowsDropped, a_event->inventory.size(), g_state.scanMs,
				settings::mixKinds.load(), settings::includeStealing.load());
		}

		// The row's copy of QuickLoot IE's ItemStack::Take, against the row's true source.
		void TakeFromSource(RE::TESObjectREFR& a_source, RE::InventoryEntryData& a_entry, int a_count, bool a_stealing)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* object = a_entry.object;
			if (!player || !object) { return; }
			auto* owner = a_source.GetOwner();
			auto* sourceActor = a_source.As<RE::Actor>();

			auto type = RE::AQUIRE_TYPE::kContainer;
			if (a_stealing) { type = RE::AQUIRE_TYPE::kSteal; }
			else if (sourceActor && sourceActor->IsDead(false)) { type = RE::AQUIRE_TYPE::kDeadBody; }

			auto* extraList = ExtraListForRemoval(&a_entry, a_count, true);
			const auto value = a_entry.GetValue();
			player->AddPlayerAddItemEvent(object, owner, &a_source, type);
			player->PlayPickUpSound(object, true, false);
			a_source.RemoveItem(object, a_count, a_stealing ? RE::ITEM_REMOVE_REASON::kSteal : RE::ITEM_REMOVE_REASON::kStoreInContainer, extraList, player);
			object->HandleRemoveItemFromContainer(&a_source);
			if (sourceActor && object->IsAmmo()) { sourceActor->ClearExtraArrows(); }
			if (a_stealing) { player->StealAlarm(&a_source, object, a_count, value, owner, true); }
		}

		void OnTakingItem(TakingItemEvent* a_event)
		{
			if (!a_event || !a_event->stack || !a_event->stack->entry) { return; }
			auto* entry = a_event->stack->entry;

			Row row;
			{
				std::scoped_lock lock(g_lock);
				const auto it = g_rows.find(entry);
				if (it == g_rows.end()) { return; }  // the crosshair's own row: QuickLoot IE takes it as usual
				row = it->second;
			}
			a_event->result = HandleResult::kStop;

			const auto action = g_action.load();
			// Take All on something free never commits a crime the player did not choose: red rows stay until they are taken one by
			// one. On a container that is already someone else's, Take All already means stealing, and takes them too.
			if (action == QuickLootAction::kTakeAll && row.stealing && !FocusIsSomeoneElses(a_event->container.get().get()))
			{
				std::scoped_lock lock(g_lock);
				++g_state.leftRed;
				logger::debug("merge: Take All left a red row in place: {}", entry->GetDisplayName());
				return;
			}

			auto source = row.source.get();
			if (!source || source->IsDeleted())
			{
				std::scoped_lock lock(g_lock);
				++g_state.sourceGone;
				logger::warn("merge: the source of \"{}\" is gone; nothing taken", entry->GetDisplayName());
				return;
			}

			const bool use = action == QuickLootAction::kUse;
			const auto* object = entry->object;
			const bool wholeStack = !use || (object && (object->Is(RE::FormType::Book) || object->IsAmmo()));
			const int count = wholeStack ? entry->countDelta : 1;
			if (count <= 0) { return; }

			logger::debug("merge: taking {} x \"{}\" from {:08X}{}", count, entry->GetDisplayName(), source->GetFormID(), row.stealing ? " (stealing)" : "");
			TakeFromSource(*source, *entry, count, row.stealing);

			// Use after the take, as QuickLoot IE does; a book is left in the player's bag rather than opened.
			if (use && object && !object->Is(RE::FormType::Book))
			{
				auto* player = RE::PlayerCharacter::GetSingleton();
				auto* equip = RE::ActorEquipManager::GetSingleton();
				if (player && equip)
				{
					auto* extraList = entry->extraLists && !entry->extraLists->empty() ? entry->extraLists->front() : nullptr;
					equip->EquipObject(player, entry->object, extraList, count, nullptr, false, false, true, false);
				}
			}

			std::scoped_lock lock(g_lock);
			++g_state.taken;
		}

		void OnInputAction(InputActionEvent* a_event)
		{
			if (a_event) { g_action = a_event->action; }
		}

		bool RowFor(const ItemStack* a_stack, Row& a_out)
		{
			if (!a_stack || !a_stack->entry) { return false; }
			std::scoped_lock lock(g_lock);
			const auto it = g_rows.find(a_stack->entry);
			if (it == g_rows.end()) { return false; }
			a_out = it->second;
			return true;
		}

		void OnModifyItemData(ModifyItemDataEvent* a_event)
		{
			Row row;
			if (!a_event || !RowFor(a_event->stack, row)) { return; }
			// "stolen" is what the shipped menu and its reskins colour by; "isStealing" is the newer name for the same thing.
			a_event->data.SetMember("stolen", row.stealing);
			a_event->data.SetMember("isStealing", row.stealing);
		}

		void OnModifyButtonBar(ModifyButtonBarEvent* a_event)
		{
			Row row;
			if (!a_event || !RowFor(a_event->stack, row)) { return; }
			auto* gameSettings = RE::GameSettingCollection::GetSingleton();
			for (auto& button : a_event->buttons)
			{
				if (button.action == QuickLootAction::kTake)
				{
					auto* setting = gameSettings ? gameSettings->GetSetting(row.stealing ? "sSteal" : "sTake") : nullptr;
					const char* label = setting ? setting->GetString() : nullptr;
					button.label = label ? label : (row.stealing ? "Steal" : "Take");
					button.stealing = row.stealing;
				}
				else if (button.action == QuickLootAction::kUse)
				{
					button.stealing = row.stealing;
				}
			}
		}

		void OnCloseLootMenu(CloseLootMenuEvent*)
		{
			std::scoped_lock lock(g_lock);
			g_rows.clear();
		}

		std::string JsonText(std::string_view a_text)
		{
			std::string out;
			out.reserve(a_text.size());
			for (const char c : a_text)
			{
				if (c == '"' || c == '\\') { out += '\\'; }
				if (static_cast<unsigned char>(c) >= 0x20) { out += c; }
			}
			return out;
		}

		const char* ApiName(ApiState a_state)
		{
			switch (a_state)
			{
			case ApiState::kNotInstalled: return "not installed";
			case ApiState::kTooOld: return "too old (needs 4.1 or newer)";
			case ApiState::kConnected: return "connected";
			default: return "not checked";
			}
		}
	}

	void Connect(bool a_lastAttempt)
	{
		if (g_api.load() == ApiState::kConnected) { return; }

		// Ask the object (rule 30): the DLL, then the exports, each answered and logged.
		const HMODULE module = GetModuleHandleA(QuickLootAPI::SERVER_PLUGIN_NAME);
		const bool v20 = module && GetProcAddress(module, "GetQuickLootInterfaceV20");
		const bool v21 = module && GetProcAddress(module, "GetQuickLootInterfaceV21");
		if (!v21 || !QuickLootAPI::Init("QuickLootContainerMerge", ApiVersion::kV21))
		{
			g_api = module ? ApiState::kTooOld : ApiState::kNotInstalled;
			if (a_lastAttempt)
			{
				logger::warn("QuickLoot IE: dll {}, interface 2.0 {}, interface 2.1 {}. This mod needs QuickLoot IE 4.1 or newer and does nothing without it.",
					module ? "loaded" : "not loaded", v20, v21);
			}
			return;
		}

		QuickLootAPI::RegisterModifyInventoryHandler(OnModifyInventory);
		QuickLootAPI::RegisterModifyItemDataHandler(OnModifyItemData);
		QuickLootAPI::RegisterModifyButtonBarHandler(OnModifyButtonBar);
		QuickLootAPI::RegisterTakingItemHandler(OnTakingItem);
		QuickLootAPI::RegisterInputActionHandler(OnInputAction);
		QuickLootAPI::RegisterCloseLootMenuHandler(OnCloseLootMenu);
		g_api = ApiState::kConnected;
		logger::info("QuickLoot IE connected (interface 2.1): six handlers registered");
	}

	void OnDataLoaded()
	{
		g_merchantChests.clear();
		if (auto* data = RE::TESDataHandler::GetSingleton())
		{
			for (const auto* faction : data->GetFormArray<RE::TESFaction>())
			{
				if (faction && faction->vendorData.merchantContainer) { g_merchantChests.insert(faction->vendorData.merchantContainer->GetFormID()); }
			}
		}
		g_excludeKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("QuickLootIE_Exclude");
		logger::info("{} merchant chest(s) are never merged; keyword QuickLootIE_Exclude {}", g_merchantChests.size(), g_excludeKeyword ? "found" : "not found");
	}

	ApiState Api() { return g_api.load(); }

	std::string StateJson()
	{
		std::scoped_lock lock(g_lock);
		std::string leftOut;
		for (const auto& l : g_state.leftOut)
		{
			if (!leftOut.empty()) { leftOut += ','; }
			leftOut += std::format(R"({{"id":"{:08X}","name":"{}","why":"{}"}})", l.id, JsonText(l.name), l.why);
		}
		std::string sources;
		int rows = 0;
		for (const auto& s : g_state.sources)
		{
			if (!sources.empty()) { sources += ','; }
			sources += std::format(R"({{"id":"{:08X}","name":"{}","rows":{},"red":{}}})", s.id, JsonText(s.name), s.rows, s.redRows);
			rows += s.rows;
		}
		return std::format(
			R"("quickLoot":"{}","enabled":{},"interiorFeet":{},"exteriorFeet":{},"lineOfSight":{},"mixBodiesAndContainers":{},"includeStealing":{},"focus":"{:08X}","focusName":"{}","mergedRows":{},"sources":[{}],"leftOut":{},"leftOutList":[{}],"stealRowsDropped":{},"scanMs":{:.2f},"scans":{},"taken":{},"leftRed":{},"sourceGone":{},"merchantChests":{})",
			ApiName(g_api.load()), settings::enabled.load(), settings::interiorFeet.load(), settings::exteriorFeet.load(), settings::lineOfSight.load(), settings::mixKinds.load(), settings::includeStealing.load(), g_state.focus, JsonText(g_state.focusName), rows, sources,
			g_state.refused, leftOut, g_state.stealRowsDropped, g_state.scanMs, g_state.scans, g_state.taken, g_state.leftRed, g_state.sourceGone, g_merchantChests.size());
	}

	bool Action(std::string_view a_name)
	{
		QuickLootAction action = QuickLootAction::kNone;
		if (a_name == "take") { action = QuickLootAction::kTake; }
		else if (a_name == "takeall") { action = QuickLootAction::kTakeAll; }
		else if (a_name == "use") { action = QuickLootAction::kUse; }
		else if (a_name == "up") { action = QuickLootAction::kScrollUp; }
		else if (a_name == "down") { action = QuickLootAction::kScrollDown; }
		else if (a_name == "transfer") { action = QuickLootAction::kTransfer; }
		auto* tasks = SKSE::GetTaskInterface();
		if (action == QuickLootAction::kNone || !tasks || g_api.load() != ApiState::kConnected) { return false; }
		tasks->AddTask([action] { QuickLootAPI::PerformInputAction(action); });
		return true;
	}

	bool ForceWindow(std::uint32_t a_formID)
	{
		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks || g_api.load() != ApiState::kConnected) { return false; }
		auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_formID);
		if (!ref) { return false; }
		tasks->AddTask([handle = ref->GetHandle()] { QuickLootAPI::ForceCurrentContainer(handle); });
		return true;
	}

	void ClearForcedWindow()
	{
		auto* tasks = SKSE::GetTaskInterface();
		if (tasks && g_api.load() == ApiState::kConnected) { tasks->AddTask([] { QuickLootAPI::ClearForcedContainer(); }); }
	}
}
