#include "Bootstrap.h"

inline heur::rfl::ModePackage* GetSonicCyberModePackage() {
	return &hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<heur::rfl::SonicParameters>>("player_common")->GetData()->cyberspace;
}

inline heur::rfl::ModePackage* GetCyberModePackageFromResource(const char* resourceName) {
	auto* rfl = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<heur::rfl::ModePackage>>(resourceName);
	return rfl == nullptr ? nullptr : rfl->GetData();
}

inline heur::rfl::ModePackage* GetCyberModePackage(app::player::CharacterId characterId) {
	switch (characterId) {
	case app::player::CharacterId::SONIC: return GetSonicCyberModePackage();
	case app::player::CharacterId::AMY: return GetCyberModePackageFromResource("amy_modepackage_cyber");
	case app::player::CharacterId::TAILS: return GetCyberModePackageFromResource("tails_modepackage_cyber");
	case app::player::CharacterId::KNUCKLES: return GetCyberModePackageFromResource("knuckles_modepackage_cyber");
	default: return GetSonicCyberModePackage();
	}
}

template<typename T>
inline void LoadParameters(app::player::GOCPlayerParameter* playerParam, app::player::CharacterId characterId, const char* resourceName) {
	auto* parameters = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<T>>(resourceName);

	playerParam->characterId = characterId;
	*reinterpret_cast<hh::fnd::Reference<hh::fnd::ResReflectionT<T>>*>(&playerParam->characterParameters) = parameters;

	playerParam->modePackages[0] = &parameters->GetData()->forwardView;
	playerParam->modePackages[1] = &parameters->GetData()->forwardView;
	playerParam->modePackages[2] = GetCyberModePackage(characterId);
	playerParam->modePackages[3] = &parameters->GetData()->cyberspaceSV;
}

inline void LoadCharacterParameters(app::player::GOCPlayerParameter* playerParam, app::player::CharacterId charId) {
	switch (charId) {
	case app::player::CharacterId::SONIC: LoadParameters<heur::rfl::SonicParameters>(playerParam, charId, "player_common"); break;
	case app::player::CharacterId::AMY: LoadParameters<heur::rfl::AmyParameters>(playerParam, charId, "amy_common"); break;
	case app::player::CharacterId::KNUCKLES: LoadParameters<heur::rfl::KnucklesParameters>(playerParam, charId, "knuckles_common"); break;
	case app::player::CharacterId::TAILS: LoadParameters<heur::rfl::TailsParameters>(playerParam, charId, "tails_common"); break;
	default: LoadParameters<heur::rfl::SonicParameters>(playerParam, charId, "player_common"); break;
	}
}

HOOK(void*, __fastcall, GetPlayerParameter, 0x1408B4060, app::player::GOCPlayerParameter* self, const hh::fnd::RflClass* rflClass)
{
	void* result = originalGetPlayerParameter(self, rflClass);

	if (result == nullptr) {
		app::player::CharacterId originalCharacterId = self->characterId;

		for (uint32_t iCid = 0; iCid < 4 && result == nullptr; iCid++) {
			app::player::CharacterId cid{ iCid };

			if (cid == originalCharacterId)
				continue;

			LoadCharacterParameters(self, cid);

			result = originalGetPlayerParameter(self, rflClass);
		}

		// Technically unsafe because the resource could deallocate and then the GOCPlayerParameter::xxxParameters pointers would be wrong,
		// but I don't think this would happen in practice as the playercommon packfile doesn't really unload.
		LoadCharacterParameters(self, originalCharacterId);
	}

	return result;
}

HOOK(void, __fastcall, LoadPlayerParams, 0x1408B3790, app::player::GOCPlayerParameter* self)
{
	originalLoadPlayerParams(self);

	if (self->characterId != app::player::CharacterId::SONIC) {
		auto cyberMode = static_cast<size_t>(app::player::GOCPlayerParameter::Mode::CYBERSPACE_FORWARD_VIEW);
		heur::rfl::ModePackage* spoofedModePackage = GetCyberModePackage(self->characterId);

		if (spoofedModePackage) {
			self->modePackages[cyberMode] = spoofedModePackage;

			self->commonParameters[cyberMode] = &spoofedModePackage->common;
			self->speedParameters[cyberMode] = &spoofedModePackage->speed;
			self->jumpParameters[cyberMode] = &spoofedModePackage->jump;
			self->jumpSpeedParameters[cyberMode] = &spoofedModePackage->jumpSpeed;
			self->doubleJumpParameters[cyberMode] = &spoofedModePackage->doubleJump;
			self->boostParameters[cyberMode] = &spoofedModePackage->boost;
			self->airBoostParameters[cyberMode] = &spoofedModePackage->airboost;
		}
	}
}

struct SpoofedStateDescArray {
	app::player::GOCPlayerHsm::StateDescRef stateDescs[400]{};
	size_t size{};

	inline void Set(const app::player::GOCPlayerHsm::StateDescRef* originalArray, size_t newSize) {
		size = newSize;
		memcpy(stateDescs, originalArray, newSize * sizeof(app::player::GOCPlayerHsm::StateDescRef));
	}

	inline void Add(const app::player::GOCPlayerHsm::StateDescRef& ref) {
		for (size_t i = 0; i < size; i++)
			if (ref.id == stateDescs[i].id)
				return;

		stateDescs[size++] = ref;
	}
};

static auto* sonicStateDescs = rangerssdk::GetAddress(app::player::Sonic::stateDescs);
static auto* amyStateDescs = rangerssdk::GetAddress(app::player::Amy::stateDescs);
static auto* knucklesStateDescs = rangerssdk::GetAddress(app::player::Knuckles::stateDescs);
static auto* tailsStateDescs = rangerssdk::GetAddress(app::player::Tails::stateDescs);

static SpoofedStateDescArray spoofedSonicStateDescArray;
static SpoofedStateDescArray spoofedAmyStateDescArray;
static SpoofedStateDescArray spoofedKnucklesStateDescArray;
static SpoofedStateDescArray spoofedTailsStateDescArray;

const SpoofedStateDescArray& GetSpoofedArray(const app::player::GOCPlayerHsm::StateDescRef* normalArray) {
	if (normalArray == sonicStateDescs) return spoofedSonicStateDescArray;
	if (normalArray == amyStateDescs) return spoofedAmyStateDescArray;
	if (normalArray == knucklesStateDescs) return spoofedKnucklesStateDescArray;
	if (normalArray == tailsStateDescs) return spoofedTailsStateDescArray;
	return spoofedSonicStateDescArray;
}

HOOK(void, __fastcall, GOCPlayerHsmSetup, 0x14AE48C40, app::player::GOCPlayerHsm* self, app::player::GOCPlayerHsm::SetupInfo& setupInfo)
{
	auto& spoofedArray = GetSpoofedArray(setupInfo.stateDescs);

	setupInfo.stateDescs = spoofedArray.stateDescs;
	setupInfo.stateDescCount = spoofedArray.size;

	originalGOCPlayerHsmSetup(self, setupInfo);
}

class FakePlayer : public app::player::Player {
public:
	void AdditionalPlayerSetup() {
		if (auto* levelInfo = gameManager->GetService<app::level::LevelInfo>()) {
			if (auto* stageData = levelInfo->stageData) {
				if (stageData->attributeFlags.test(app::level::StageData::AttributeFlags::CYBER)) {
					switch (stageData->cyberMode) {
					case app::level::StageData::CyberMode::SPEED_SCALE:
						if (auto* gocPlayerParam = GetComponent<app::player::GOCPlayerParameter>())
							if (auto* cyberMode = gocPlayerParam->GetPlayerParameter<heur::rfl::PlayerParamCyberMode>()) {
								app::game::MsgChangeLayerTimeScale changeLayerTimeScale{ "CyberModeTimeScale", 0xC007FF0, cyberMode->timeScale };
								app::game::MsgChangeGlobalTimeScale changeGlobalTimeScale{ "CyberModeTimeScaleGlobal", cyberMode->timeScale };

								SendMessageToGame(changeLayerTimeScale);
								SendMessageToGame(changeGlobalTimeScale);
							}
						break;

					case app::level::StageData::CyberMode::NITRO:
						if (auto* gocPlayerHsm = GetComponent<app::player::GOCPlayerHsm>())
							if (auto& statePluginMgr = gocPlayerHsm->statePluginManager)
								if (auto* statePluginBoost = statePluginMgr->GetPlugin<app::player::StatePluginBoost>())
									statePluginBoost->SetNitroMode();
						break;

					case app::level::StageData::CyberMode::MAX_SPEED_CHALLENGE:
						if (auto* gocPlayerHsm = GetComponent<app::player::GOCPlayerHsm>())
							if (auto& statePluginMgr = gocPlayerHsm->statePluginManager)
								if (auto* statePluginBoost = statePluginMgr->GetPlugin<app::player::StatePluginBoost>()) {
									statePluginBoost->SetBoostType(2);
									statePluginBoost->SetUnk1(1);
								}
						if (auto* gocPlayerBlackboard = GetComponent<app::player::GOCPlayerBlackboard>())
							if (auto* blackboardStatus = gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardStatus>())
								blackboardStatus->SetWorldFlag(app::player::BlackboardStatus::WorldFlag::MAX_SPEED_CHALLENGE, true);
						break;

					case app::level::StageData::CyberMode::LOW_GRAVITY:
						if (auto* gocPlayerParam = GetComponent<app::player::GOCPlayerParameter>())
							if (auto* cyberMode = gocPlayerParam->GetPlayerParameter<heur::rfl::PlayerParamCyberMode>())
								if (auto* gocPlayerKineParams = GetComponent<app::player::GOCPlayerKinematicParams>()) {
									gocPlayerKineParams->SetGravityScale(cyberMode->lowGravityScale);
								}
						break;
					}

					if (auto* gocPlayerHsm = GetComponent<app::player::GOCPlayerHsm>())
						if (auto& statePluginMgr = gocPlayerHsm->statePluginManager) {
							auto* cyberStart = new (GetAllocator()) app::player::StatePluginCyberStart(GetAllocator());
							cyberStart->context = statePluginMgr->context;
							statePluginMgr->AddPlugin(cyberStart);

							if (setupInfo.unk2 != 1)
								cyberStart->Setup();
						}
				}
			}
		}
	}

	void AdditionalPlayerTeardown() {
		if (auto* levelInfo = gameManager->GetService<app::level::LevelInfo>())
			if (auto* stageData = levelInfo->stageData)
				if (stageData->attributeFlags.test(app::level::StageData::AttributeFlags::CYBER))
					if (stageData->cyberMode == app::level::StageData::CyberMode::SPEED_SCALE) {
						app::game::MsgRevertLayerTimeScale revertLayerTimeScale{ "CyberModeTimeScale", 0xC007FF0 };
						app::game::MsgRevertGlobalTimeScale revertGlobalTimeScale{ "CyberModeTimeScaleGlobal" };

						SendMessageToGame(revertLayerTimeScale);
						SendMessageToGame(revertGlobalTimeScale);
					}
	}
};

HOOK(void, __fastcall, Amy_SetupPlayer, 0x140886900, FakePlayer* self) {
	originalAmy_SetupPlayer(self);
	self->AdditionalPlayerSetup();
}

HOOK(void, __fastcall, Others_TeardownPlayer, 0x140886870, FakePlayer* self) {
	originalOthers_TeardownPlayer(self);
	self->AdditionalPlayerTeardown();
}

HOOK(void, __fastcall, Knuckles_SetupPlayer, 0x140888930, FakePlayer* self) {
	originalKnuckles_SetupPlayer(self);
	self->AdditionalPlayerSetup();
}

HOOK(void, __fastcall, Tails_SetupPlayer, 0x14088D0F0, FakePlayer* self) {
	originalTails_SetupPlayer(self);
	self->AdditionalPlayerSetup();
}

void CreateSpoofedStateDescArrays() {
	SpoofedStateDescArray* spoofedArrays[] = { &spoofedSonicStateDescArray, &spoofedAmyStateDescArray, &spoofedKnucklesStateDescArray, &spoofedTailsStateDescArray };
	const app::player::GOCPlayerHsm::StateDescRef* arrays[] = { sonicStateDescs, amyStateDescs, knucklesStateDescs, tailsStateDescs };
	const size_t arraySizes[] = { app::player::Sonic::stateDescCount, app::player::Amy::stateDescCount, app::player::Knuckles::stateDescCount, app::player::Tails::stateDescCount };

	for (size_t targetCharacterId = 0; targetCharacterId < 4; targetCharacterId++) {
		spoofedArrays[targetCharacterId]->Set(arrays[targetCharacterId], arraySizes[targetCharacterId]);

		for (size_t sourceCharacterId = 0; sourceCharacterId < 4; sourceCharacterId++) {
			if (sourceCharacterId == targetCharacterId)
				continue;

			for (size_t i = 0; i < arraySizes[sourceCharacterId]; i++)
				spoofedArrays[targetCharacterId]->Add(arrays[sourceCharacterId][i]);
		}
	}
}

void Bootstrap() {
	auto* allocator = hh::fnd::MemoryRouter::GetModuleAllocator();
	auto* rLoader = new (allocator) hh::fnd::ResourceLoader(allocator);
	rLoader->LoadPackfile("mods/angryzor/character_normalizer/player_common_extra_cyber.pac", 0);

	CreateSpoofedStateDescArrays();

	INSTALL_HOOK(GetPlayerParameter);
	INSTALL_HOOK(LoadPlayerParams);
	INSTALL_HOOK(GOCPlayerHsmSetup);
	INSTALL_HOOK(Amy_SetupPlayer);
	INSTALL_HOOK(Knuckles_SetupPlayer);
	INSTALL_HOOK(Tails_SetupPlayer);
	INSTALL_HOOK(Others_TeardownPlayer);
}
