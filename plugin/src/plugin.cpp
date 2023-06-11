#include <vector>
#include <tier0/dbg.h>
#include <tier0/platform.h>
#include <tier1/tier1.h>
#include <tier2/tier2.h>
#include <eiface.h>
#include <tier0/valve_minmax_off.h>

#include "plugin.hpp"
#include "module.hpp"
#include "patch.hpp"

#include "patches/player_count.hpp"
#include "patches/dc_check.hpp"
#include "patches/transitions_fix.hpp"
#include "patches/env_fade.hpp"

void CMPPatchPlugin::update_patches() {
    //Prepare modules
    Msg("Preparing modules...\n");
    m_EngineModule->unprotect();
    m_MatchMakingModule->unprotect();
    m_ServerModule->unprotect();

    //Delete old patches
    if(m_Patches.size() > 0) {
        Msg("Clearing old patches...\n");
        clear_patches();
    }

    //Register patches
    for(const std::unique_ptr<IPatchRegistrar>& reg : m_PatchRegistrars) {
        Msg("Registering patches for registrar '%s'...\n", reg->name());
        reg->register_patches(*this);
    }

    //Apply patches
    Msg("Applying patches...\n");
    for(std::unique_ptr<CPatch>& patch : m_Patches) patch->apply();

    //Invoke post-apply hooks
    Msg("Invoking post-apply hooks...\n");
    for(const std::unique_ptr<IPatchRegistrar>& reg : m_PatchRegistrars) {
        reg->after_patch_status_change(*this, true);
    }

    //Finalize modules
    Msg("Finalizing modules...\n");
    m_EngineModule->reprotect();
    m_MatchMakingModule->reprotect();
    m_ServerModule->reprotect();
}

//Server callbacks
bool CMPPatchPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    ConnectTier1Libraries(&interfaceFactory, 1);
    ConnectTier2Libraries(&interfaceFactory, 1);

    //Get interfaces
    if(!(m_PlayerInfoManager = (IPlayerInfoManager*) gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, NULL))) {
        Warning("Couldn't obtain IPlayerInfoManager [%s] interface!", INTERFACEVERSION_PLAYERINFOMANAGER);
        return false;
    } else DevMsg("IPlayerInfoManager: %p\n", m_PlayerInfoManager);

    try {
        //Prepare modules
        Msg("Creating modules...\n");
        m_EngineModule = new CModule("engine");
        m_MatchMakingModule = new CModule("matchmaking");
        m_ServerModule = new CModule("server");

        //Prearing registrars
        Msg("Preparing registrars...\n");
        m_PatchRegistrars.emplace_back(new patches::CPlayerCountPatch(MAX_PLAYERS));
        m_PatchRegistrars.emplace_back(new patches::CDCCheckPatch());
        m_PatchRegistrars.emplace_back(new patches::CTransitionsFixPatch());
        m_PatchRegistrars.emplace_back(new patches::CEnvFadePatch());

        //Apply patches
        update_patches();
    } catch(const std::exception& e) {
        Warning("Exception while applying patches: %s\n", e.what());
        return false;
    }

    return true;
}

void CMPPatchPlugin::Unload() {
    try {
        //Delete patches
        Msg("Deleting patches...\n");

        if(m_EngineModule) m_EngineModule->unprotect();
        if(m_MatchMakingModule) m_MatchMakingModule->unprotect();
        if(m_ServerModule) m_ServerModule->unprotect();

        clear_patches();

        if(m_EngineModule) m_EngineModule->reprotect();
        if(m_MatchMakingModule) m_MatchMakingModule->reprotect();
        if(m_ServerModule) m_ServerModule->reprotect();

        //Delete registrars
        Msg("Deleting registrars...\n");
        m_PatchRegistrars.clear();

        //Delete modules
        Msg("Deleting modules...\n");
        delete m_EngineModule;
        delete m_MatchMakingModule;
        delete m_ServerModule;

        //Clear the scratchpad in case something still uses it
        m_ScratchPad.clear();

        Msg("Done!\n");
    } catch(const std::exception& e) {
        Warning("Exception while reverting patches: %s\n", e.what());
    }

    DisconnectTier1Libraries();
    DisconnectTier2Libraries();
}

void CMPPatchPlugin::clear_patches() {
    //Revert patches
    for(std::unique_ptr<CPatch>& patch : m_Patches) patch->revert();
    m_Patches.clear();

    //Invoke post-revert hooks
    Msg("Invoking post-revert hooks...\n");
    for(const std::unique_ptr<IPatchRegistrar>& reg : m_PatchRegistrars) {
        reg->after_patch_status_change(*this, false);
    }
}

void CMPPatchPlugin::Pause() {}
void CMPPatchPlugin::UnPause() {}
const char *CMPPatchPlugin::GetPluginDescription() { return "Patches Portal 2 multiplayer to work with arbitrary players and maps"; }

void CMPPatchPlugin::LevelInit(char const *pMapName) {}
void CMPPatchPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {}
void CMPPatchPlugin::GameFrame(bool simulating) {}
void CMPPatchPlugin::LevelShutdown(void) {}

void CMPPatchPlugin::ClientActive(edict_t *pEntity) {}
void CMPPatchPlugin::ClientFullyConnect(edict_t *pEntity) {}
void CMPPatchPlugin::ClientDisconnect(edict_t *pEntity) {}
void CMPPatchPlugin::ClientPutInServer(edict_t *pEntity, char const *playername) {}
void CMPPatchPlugin::SetCommandClient(int index) {}
void CMPPatchPlugin::ClientSettingsChanged(edict_t *pEdict) {}
PLUGIN_RESULT CMPPatchPlugin::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) { return PLUGIN_CONTINUE; }
PLUGIN_RESULT CMPPatchPlugin::ClientCommand(edict_t *pEntity, const CCommand &args) { return PLUGIN_CONTINUE; }

PLUGIN_RESULT CMPPatchPlugin::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) { return PLUGIN_CONTINUE; }
void CMPPatchPlugin::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) {}
void CMPPatchPlugin::OnEdictAllocated(edict_t *edict) {}
void CMPPatchPlugin::OnEdictFreed(const edict_t *edict) {}

//Module definition
CMPPatchPlugin g_MPPatchPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CMPPatchPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_MPPatchPlugin);