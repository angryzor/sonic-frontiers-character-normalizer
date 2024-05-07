#include "Bootstrap.h"

HOOK(void*, __fastcall, GetPlayerParameter, 0x1408B4060, app::player::GOCPlayerParameter* self, const hh::fnd::RflClass* rflClass)
{
	void* orig = originalGetPlayerParameter(self, rflClass);

	if (self->characterId != app::player::CharacterId::SONIC && orig == nullptr) {
		app::player::CharacterId cid = self->characterId;
		hh::fnd::Reference<hh::fnd::ResReflection<app::rfl::SonicParameters>> params = &self->characterParameters->sonic;
		hh::fnd::Reference<hh::fnd::ResReflection<app::rfl::SonicParameters>> sonicParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflection<app::rfl::SonicParameters>>("player_common");
		app::rfl::ModePackage* modePackages[4];

		self->characterId = app::player::CharacterId::SONIC;
		*reinterpret_cast<hh::fnd::Reference<hh::fnd::ResReflection<app::rfl::SonicParameters>>*>(&self->characterParameters) = sonicParams;
		for (size_t i = 0; i < 4; i++)
			modePackages[i] = self->modePackages[i];

		self->modePackages[0] = &sonicParams->reflectionData->forwardView;
		self->modePackages[1] = &sonicParams->reflectionData->forwardView;
		self->modePackages[2] = &sonicParams->reflectionData->cyberspace;
		self->modePackages[3] = &sonicParams->reflectionData->cyberspaceSV;

		orig = originalGetPlayerParameter(self, rflClass);

		self->characterId = cid;
		*reinterpret_cast<hh::fnd::Reference<hh::fnd::ResReflection<app::rfl::SonicParameters>>*>(&self->characterParameters) = params;

		for (size_t i = 0; i < 4; i++)
			self->modePackages[i] = modePackages[i];
	}

	return orig;
}

HOOK(void, __fastcall, LoadPlayerParams, 0x1408B3790, app::player::GOCPlayerParameter* self)
{
	originalLoadPlayerParams(self);

	if (self->characterId != app::player::CharacterId::SONIC) {
		auto cyberMode = static_cast<size_t>(app::player::GOCPlayerParameter::Mode::CYBERSPACE_FORWARD_VIEW);
		hh::fnd::ResReflection<app::rfl::ModePackage>* spoofedModePackage{};

		switch (self->characterId) {
		case app::player::CharacterId::AMY:
			spoofedModePackage = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflection<app::rfl::ModePackage>>("amy_modepackage_cyber");
			break;
		case app::player::CharacterId::TAILS:
			spoofedModePackage = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflection<app::rfl::ModePackage>>("tails_modepackage_cyber");
			break;
		case app::player::CharacterId::KNUCKLES:
			spoofedModePackage = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflection<app::rfl::ModePackage>>("knuckles_modepackage_cyber");
			break;
		}

		if (spoofedModePackage && spoofedModePackage->reflectionData) {
			self->modePackages[cyberMode] = spoofedModePackage->reflectionData;

			self->commonParameters[cyberMode] = self->commonParameters[cyberMode] ? self->commonParameters[cyberMode] : &self->modePackages[cyberMode]->common;
			self->speedParameters[cyberMode] = self->speedParameters[cyberMode] ? self->speedParameters[cyberMode] : &self->modePackages[cyberMode]->speed;
			self->jumpParameters[cyberMode] = self->jumpParameters[cyberMode] ? self->jumpParameters[cyberMode] : &self->modePackages[cyberMode]->jump;
			self->jumpSpeedParameters[cyberMode] = self->jumpSpeedParameters[cyberMode] ? self->jumpSpeedParameters[cyberMode] : &self->modePackages[cyberMode]->jumpSpeed;
			self->doubleJumpParameters[cyberMode] = self->doubleJumpParameters[cyberMode] ? self->doubleJumpParameters[cyberMode] : &self->modePackages[cyberMode]->doubleJump;
			self->boostParameters[cyberMode] = self->boostParameters[cyberMode] ? self->boostParameters[cyberMode] : &self->modePackages[cyberMode]->boost;
			self->airBoostParameters[cyberMode] = self->airBoostParameters[cyberMode] ? self->airBoostParameters[cyberMode] : &self->modePackages[cyberMode]->airboost;
		}
	}
}

struct SpoofedStateDescArray {
	app::player::GOCPlayerHsm::StateDescRef stateDescs[400]{};
	size_t size{};
};

static auto* sonicStateDescs = rangerssdk::GetAddress(app::player::Sonic::stateDescs);
static auto* amyStateDescs = rangerssdk::GetAddress(app::player::Amy::stateDescs);
static auto* knucklesStateDescs = rangerssdk::GetAddress(app::player::Knuckles::stateDescs);
static auto* tailsStateDescs = rangerssdk::GetAddress(app::player::Tails::stateDescs);

static SpoofedStateDescArray spoofedAmyStateDescArray;
static SpoofedStateDescArray spoofedKnucklesStateDescArray;
static SpoofedStateDescArray spoofedTailsStateDescArray;

const SpoofedStateDescArray& GetSpoofedArray(const app::player::GOCPlayerHsm::StateDescRef* normalArray) {
	if (normalArray == amyStateDescs) return spoofedAmyStateDescArray;
	if (normalArray == knucklesStateDescs) return spoofedKnucklesStateDescArray;
	if (normalArray == tailsStateDescs) return spoofedTailsStateDescArray;
	return spoofedAmyStateDescArray;
}

HOOK(void, __fastcall, GOCPlayerHsmSetup, 0x14AED53B0, app::player::GOCPlayerHsm* self, app::player::GOCPlayerHsm::SetupInfo& setupInfo)
{
	if (setupInfo.stateDescs != sonicStateDescs) {
		auto& spoofedArray = GetSpoofedArray(setupInfo.stateDescs);

		setupInfo.stateDescs = spoofedArray.stateDescs;
		setupInfo.stateDescCount = spoofedArray.size;
	}

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
							if (auto* cyberMode = gocPlayerParam->GetPlayerParameter<app::rfl::PlayerParamCyberMode>()) {
								SendMessageToGame(app::game::MsgChangeLayerTimeScale{ "CyberModeTimeScale", 0xC007FF0, cyberMode->timeScale });
								SendMessageToGame(app::game::MsgChangeGlobalTimeScale{ "CyberModeTimeScaleGlobal", cyberMode->timeScale });
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
							if (auto* cyberMode = gocPlayerParam->GetPlayerParameter<app::rfl::PlayerParamCyberMode>())
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
						SendMessageToGame(app::game::MsgRevertLayerTimeScale{ "CyberModeTimeScale", 0xC007FF0 });
						SendMessageToGame(app::game::MsgRevertGlobalTimeScale{ "CyberModeTimeScaleGlobal" });
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
	SpoofedStateDescArray* spoofedArrays[] = { &spoofedAmyStateDescArray, &spoofedKnucklesStateDescArray, &spoofedTailsStateDescArray };
	const app::player::GOCPlayerHsm::StateDescRef* arrays[] = { amyStateDescs, knucklesStateDescs, tailsStateDescs };
	const size_t arraySizes[] = { app::player::Amy::stateDescCount, app::player::Knuckles::stateDescCount, app::player::Tails::stateDescCount };

	for (size_t i = 0; i < 3; i++) {
		spoofedArrays[i]->size = arraySizes[i];

		memcpy(spoofedArrays[i]->stateDescs, arrays[i], spoofedArrays[i]->size * sizeof(app::player::GOCPlayerHsm::StateDescRef));

		for (size_t j = 0; j < app::player::Sonic::stateDescCount; j++) {
			bool found = false;
			for (size_t k = 0; k < arraySizes[i]; k++) {
				if (sonicStateDescs[j].id == spoofedArrays[i]->stateDescs[k].id) {
					found = true;
					break;
				}
			}
			if (!found)
				spoofedArrays[i]->stateDescs[spoofedArrays[i]->size++] = sonicStateDescs[j];
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
