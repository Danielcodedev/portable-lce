#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Forward declarations - minecraft types
class LevelGenerationOptions;
class LevelRuleset;
class LevelChunk;
class ModelPart;

// Forward declarations - app types (opaque pointers only)
class DLCManager;
class GameRuleManager;

// SKIN_BOX, FEATURE_DATA, MOJANG_DATA are C-style typedef'd structs
// that cannot be forward-declared.  Include the lightweight headers.
#include "minecraft/client/SkinBox.h"
#include "app/common/App_structs.h"

// Enums needed by callers - pulled from app layer.
// These are small POD enums safe to include transitively.
#include "app/common/App_enums.h"
#include "platform/PlatformTypes.h"  // PlayerUID
#include "minecraft/network/packet/DisconnectPacket.h"  // eDisconnectReason
#include "minecraft/client/IMenuService.h"

// eINSTANCEOF lives in java/Class.h which is heavyweight.
// We use int here and cast at init/call sites.
using EntityTypeId = int;

namespace GameServices {
using LevelGenOptsFn = LevelGenerationOptions* (*)();
using GameRuleDefsFn = LevelRuleset* (*)();

void initLevelGen(LevelGenOptsFn levelGenOpts, GameRuleDefsFn gameRuleDefs);
[[nodiscard]] LevelGenerationOptions* getLevelGenerationOptions();
[[nodiscard]] LevelRuleset* getGameRuleDefinitions();
using AddTexFn = void (*)(const std::wstring&, std::uint8_t*, unsigned int);
using RemoveTexFn = void (*)(const std::wstring&);
using GetTexDetailsFn = void (*)(const std::wstring&, std::uint8_t**,
                                 unsigned int*);
using HasTexFn = bool (*)(const std::wstring&);

void initTextureCache(AddTexFn add, RemoveTexFn remove,
                      GetTexDetailsFn getDetails, HasTexFn has);
void addMemoryTextureFile(const std::wstring& name, std::uint8_t* data,
                          unsigned int size);
void removeMemoryTextureFile(const std::wstring& name);
void getMemFileDetails(const std::wstring& name, std::uint8_t** data,
                       unsigned int* size);
[[nodiscard]] bool isFileInMemoryTextures(const std::wstring& name);
using GetSettingsFn = unsigned char (*)(int iPad, int setting);
using GetSettingsNoArgFn = unsigned char (*)(int setting);

void initPlayerSettings(GetSettingsFn getSettings,
                        GetSettingsNoArgFn getSettingsNoArg);
[[nodiscard]] unsigned char getGameSettings(int iPad, int setting);
[[nodiscard]] unsigned char getGameSettings(int setting);
using AppTimeFn = float (*)();

void initAppTime(AppTimeFn fn);
[[nodiscard]] float getAppTime();

// Game state

void initGameState(
    bool (*getGameStarted)(),
    void (*setGameStarted)(bool),
    bool (*getTutorialMode)(),
    void (*setTutorialMode)(bool),
    bool (*isAppPaused)(),
    int (*getLocalPlayerCount)(),
    bool (*autosaveDue)(),
    void (*setAutosaveTimerTime)(),
    int64_t (*secondsToAutosave)(),
    void (*setDisconnectReason)(DisconnectPacket::eDisconnectReason),
    void (*lockSaveNotify)(),
    void (*unlockSaveNotify)(),
    bool (*getResetNether)(),
    bool (*getUseDPadForDebug)(),
    bool (*getWriteSavesToFolderEnabled)(),
    bool (*isLocalMultiplayerAvailable)(),
    bool (*dlcInstallPending)(),
    bool (*dlcInstallProcessCompleted)(),
    bool (*canRecordStatsAndAchievements)(),
    bool (*getTMSGlobalFileListRead)(),
    void (*setRequiredTexturePackID)(std::uint32_t),
    void (*setSpecialTutorialCompletionFlag)(int iPad, int index),
    void (*setBanListCheck)(int iPad, bool),
    bool (*getBanListCheck)(int iPad),
    unsigned int (*getGameNewWorldSize)(),
    unsigned int (*getGameNewWorldSizeUseMoat)(),
    unsigned int (*getGameNewHellScale)()
);

[[nodiscard]] bool getGameStarted();
void setGameStarted(bool val);
[[nodiscard]] bool getTutorialMode();
void setTutorialMode(bool val);
[[nodiscard]] bool isAppPaused();
[[nodiscard]] int getLocalPlayerCount();
[[nodiscard]] bool autosaveDue();
void setAutosaveTimerTime();
[[nodiscard]] int64_t secondsToAutosave();
void setDisconnectReason(DisconnectPacket::eDisconnectReason reason);
void lockSaveNotification();
void unlockSaveNotification();
[[nodiscard]] bool getResetNether();
[[nodiscard]] bool getUseDPadForDebug();
[[nodiscard]] bool getWriteSavesToFolderEnabled();
[[nodiscard]] bool isLocalMultiplayerAvailable();
[[nodiscard]] bool dlcInstallPending();
[[nodiscard]] bool dlcInstallProcessCompleted();
[[nodiscard]] bool canRecordStatsAndAchievements();
[[nodiscard]] bool getTMSGlobalFileListRead();
void setRequiredTexturePackID(std::uint32_t id);
void setSpecialTutorialCompletionFlag(int iPad, int index);
void setBanListCheck(int iPad, bool val);
[[nodiscard]] bool getBanListCheck(int iPad);
[[nodiscard]] unsigned int getGameNewWorldSize();
[[nodiscard]] unsigned int getGameNewWorldSizeUseMoat();
[[nodiscard]] unsigned int getGameNewHellScale();

// UI dispatch

void initUIDispatch(
    void (*setAction)(int iPad, eXuiAction action, void* param),
    void (*setXuiServerAction)(int iPad, eXuiServerAction action, void* param),
    eXuiAction (*getXuiAction)(int iPad),
    eXuiServerAction (*getXuiServerAction)(int iPad),
    void* (*getXuiServerActionParam)(int iPad),
    void (*setGlobalXuiAction)(eXuiAction action),
    void (*handleButtonPresses)(),
    void (*setTMSAction)(int iPad, eTMSAction action)
);

void setAction(int iPad, eXuiAction action, void* param = nullptr);
void setXuiServerAction(int iPad, eXuiServerAction action,
                        void* param = nullptr);
[[nodiscard]] eXuiAction getXuiAction(int iPad);
[[nodiscard]] eXuiServerAction getXuiServerAction(int iPad);
[[nodiscard]] void* getXuiServerActionParam(int iPad);
void setGlobalXuiAction(eXuiAction action);
void handleButtonPresses();
void setTMSAction(int iPad, eTMSAction action);

// Skin / cape / animation

void initSkinCape(
    std::wstring (*getPlayerSkinName)(int iPad),
    std::uint32_t (*getPlayerSkinId)(int iPad),
    std::wstring (*getPlayerCapeName)(int iPad),
    std::uint32_t (*getPlayerCapeId)(int iPad),
    std::uint32_t (*getAdditionalModelPartsForPad)(int iPad),
    void (*setAdditionalSkinBoxes)(std::uint32_t dwSkinID, SKIN_BOX* boxA,
                                   unsigned int boxC),
    std::vector<SKIN_BOX*>* (*getAdditionalSkinBoxes)(std::uint32_t dwSkinID),
    std::vector<ModelPart*>* (*getAdditionalModelParts)(std::uint32_t dwSkinID),
    std::vector<ModelPart*>* (*setAdditionalSkinBoxesFromVec)(
        std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA),
    void (*setAnimOverrideBitmask)(std::uint32_t dwSkinID, unsigned int bitmask),
    unsigned int (*getAnimOverrideBitmask)(std::uint32_t dwSkinID),
    std::uint32_t (*skinIdFromPath)(const std::wstring& skin),
    std::wstring (*skinPathFromId)(std::uint32_t skinId),
    bool (*defaultCapeExists)(),
    bool (*isXuidNotch)(PlayerUID xuid),
    bool (*isXuidDeadmau5)(PlayerUID xuid)
);

[[nodiscard]] std::wstring getPlayerSkinName(int iPad);
[[nodiscard]] std::uint32_t getPlayerSkinId(int iPad);
[[nodiscard]] std::wstring getPlayerCapeName(int iPad);
[[nodiscard]] std::uint32_t getPlayerCapeId(int iPad);
[[nodiscard]] std::uint32_t getAdditionalModelPartsForPad(int iPad);
void setAdditionalSkinBoxes(std::uint32_t dwSkinID, SKIN_BOX* boxA,
                            unsigned int boxC);
[[nodiscard]] std::vector<SKIN_BOX*>* getAdditionalSkinBoxes(
    std::uint32_t dwSkinID);
[[nodiscard]] std::vector<ModelPart*>* getAdditionalModelParts(
    std::uint32_t dwSkinID);
std::vector<ModelPart*>* setAdditionalSkinBoxesFromVec(
    std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA);
void setAnimOverrideBitmask(std::uint32_t dwSkinID, unsigned int bitmask);
[[nodiscard]] unsigned int getAnimOverrideBitmask(std::uint32_t dwSkinID);
[[nodiscard]] std::uint32_t getSkinIdFromPath(const std::wstring& skin);
[[nodiscard]] std::wstring getSkinPathFromId(std::uint32_t skinId);
[[nodiscard]] bool defaultCapeExists();
[[nodiscard]] bool isXuidNotch(PlayerUID xuid);
[[nodiscard]] bool isXuidDeadmau5(PlayerUID xuid);

// Platform features

void initPlatformFeatures(
    void (*fatalLoadError)(),
    void (*setRichPresenceContext)(int iPad, int contextId),
    void (*captureSaveThumbnail)(),
    void (*getSaveThumbnail)(std::uint8_t**, unsigned int*),
    void (*readBannedList)(int iPad, eTMSAction action, bool bCallback),
    void (*updatePlayerInfo)(std::uint8_t networkSmallId,
                             int16_t playerColourIndex,
                             unsigned int playerPrivileges),
    unsigned int (*getPlayerPrivileges)(std::uint8_t networkSmallId),
    void (*setGameSettingsDebugMask)(int iPad, unsigned int uiVal)
);

void fatalLoadError();
void setRichPresenceContext(int iPad, int contextId);
void captureSaveThumbnail();
void getSaveThumbnail(std::uint8_t** data, unsigned int* size);
void readBannedList(int iPad, eTMSAction action = (eTMSAction)0,
                    bool bCallback = false);
void updatePlayerInfo(std::uint8_t networkSmallId, int16_t playerColourIndex,
                      unsigned int playerPrivileges);
[[nodiscard]] unsigned int getPlayerPrivileges(std::uint8_t networkSmallId);
void setGameSettingsDebugMask(int iPad, unsigned int uiVal);

// Schematics / terrain

void initSchematics(
    void (*processSchematics)(LevelChunk* chunk),
    void (*processSchematicsLighting)(LevelChunk* chunk),
    void (*addTerrainFeaturePosition)(_eTerrainFeatureType, int, int),
    bool (*getTerrainFeaturePosition)(_eTerrainFeatureType, int*, int*),
    void (*loadDefaultGameRules)()
);

void processSchematics(LevelChunk* chunk);
void processSchematicsLighting(LevelChunk* chunk);
void addTerrainFeaturePosition(_eTerrainFeatureType type, int x, int z);
[[nodiscard]] bool getTerrainFeaturePosition(_eTerrainFeatureType type,
                                             int* pX, int* pZ);
void loadDefaultGameRules();

// Archive / resources

void initArchive(
    bool (*hasArchiveFile)(const std::wstring&),
    std::vector<std::uint8_t> (*getArchiveFile)(const std::wstring&)
);

[[nodiscard]] bool hasArchiveFile(const std::wstring& filename);
[[nodiscard]] std::vector<std::uint8_t> getArchiveFile(
    const std::wstring& filename);

// Strings / formatting / misc queries

void initStringsAndMisc(
    int (*getHTMLColour)(eMinecraftColour),
    std::wstring (*getEntityName)(EntityTypeId),
    const wchar_t* (*getGameRulesString)(const std::wstring&),
    unsigned int (*createImageTextData)(std::uint8_t*, int64_t, bool,
                                        unsigned int, unsigned int),
    std::wstring (*getFilePath)(std::uint32_t, std::wstring, bool,
                                std::wstring),
    char* (*getUniqueMapName)(),
    void (*setUniqueMapName)(char*),
    unsigned int (*getOpacityTimer)(int iPad),
    void (*setOpacityTimer)(int iPad),
    void (*tickOpacityTimer)(int iPad),
    bool (*isInBannedLevelList)(int iPad, PlayerUID xuid, char* levelName),
    MOJANG_DATA* (*getMojangDataForXuid)(PlayerUID xuid),
    void (*debugPrintf)(const char*)
);

[[nodiscard]] int getHTMLColour(eMinecraftColour colour);
[[nodiscard]] int getHTMLColor(eMinecraftColour colour);
[[nodiscard]] std::wstring getEntityName(EntityTypeId type);
[[nodiscard]] const wchar_t* getGameRulesString(const std::wstring& key);
[[nodiscard]] unsigned int createImageTextData(std::uint8_t* textMetadata,
                                               int64_t seed, bool hasSeed,
                                               unsigned int uiHostOptions,
                                               unsigned int uiTexturePackId);
[[nodiscard]] std::wstring getFilePath(std::uint32_t packId,
                                       std::wstring filename,
                                       bool bAddDataFolder,
                                       std::wstring mountPoint = L"TPACK:");
[[nodiscard]] char* getUniqueMapName();
void setUniqueMapName(char* name);
[[nodiscard]] unsigned int getOpacityTimer(int iPad);
void setOpacityTimer(int iPad);
void tickOpacityTimer(int iPad);
[[nodiscard]] bool isInBannedLevelList(int iPad, PlayerUID xuid,
                                       char* levelName);
[[nodiscard]] MOJANG_DATA* getMojangDataForXuid(PlayerUID xuid);
void debugPrintf(const char* msg);

// Member variable access (opaque pointers)

void initMemberAccess(
    DLCManager* dlcManager,
    GameRuleManager* gameRules,
    std::vector<std::wstring>* skinNames,
    std::vector<FEATURE_DATA*>* terrainFeatures
);

[[nodiscard]] DLCManager& getDLCManager();
[[nodiscard]] GameRuleManager& getGameRules();
[[nodiscard]] std::vector<std::wstring>& getSkinNames();
[[nodiscard]] std::vector<FEATURE_DATA*>& getTerrainFeatures();
void initMenuService(IMenuService* service);
[[nodiscard]] IMenuService& menus();

}  // namespace GameServices
