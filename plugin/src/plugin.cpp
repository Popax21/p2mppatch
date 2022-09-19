#include <vector>
#include <tier0/dbg.h>
#include <tier0/platform.h>
#include <tier1/tier1.h>
#include <tier2/tier2.h>
#include <engine/iserverplugin.h>
#include <tier0/valve_minmax_off.h>
#include "plugin.hpp"
#include "module.hpp"
#include "patch.hpp"

#include "patches/player_count.h"

struct tm *Plat_localtime(const time_t *timep, struct tm *result) {
    return localtime_r(timep, result);
}

void CMPPatchPlugin::update_patches() {
    //Prepare modules
    Msg("Preparing modules...\n");
    m_EngineModule->unprotect();
    m_MatchMakingModule->unprotect();
    m_ServerModule->unprotect();

    //Delete old patches
    if(m_Patches.size() > 0) {
        Msg("Clearing old patches...\n");
        m_Patches.clear();
    }

    //Register patches
    for(const std::unique_ptr<IPatchRegistrar>& reg : m_PatchRegistrars) {
        Msg("Registering patches for registrar '%s'...\n", reg->name());
        reg->register_patches(*this);
    }

    //Apply patches
    Msg("Applying patches...\n");
    for(std::unique_ptr<CPatch>& patch : m_Patches) patch->apply();

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

    try {
        //Prepare modules
        Msg("Creating modules...\n");
        m_EngineModule = new CModule("engine");
        m_MatchMakingModule = new CModule("matchmaking");
        m_ServerModule = new CModule("server");

        //Prearing registrars
        Msg("Preparing registrars...\n");
        m_PatchRegistrars.emplace_back(new patches::CPlayerCountPatch(MAX_PLAYERS));

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

        m_EngineModule->unprotect();
        m_MatchMakingModule->unprotect();
        m_ServerModule->unprotect();

        for(std::unique_ptr<CPatch>& patch : m_Patches) patch->revert();
        m_Patches.clear();

        m_EngineModule->reprotect();
        m_MatchMakingModule->reprotect();
        m_ServerModule->reprotect();

        //Delete registrars
        Msg("Deleting registrars...\n");
        m_PatchRegistrars.clear();

        //Delete modules
        Msg("Deleting modules...\n");
        delete m_EngineModule;
        delete m_MatchMakingModule;
        delete m_ServerModule;

        Msg("Done!\n");
    } catch(const std::exception& e) {
        Warning("Exception while reverting patches: %s\n", e.what());
    }

    DisconnectTier1Libraries();
    DisconnectTier2Libraries();
}

void CMPPatchPlugin::Pause() {}
void CMPPatchPlugin::UnPause() {}
const char *CMPPatchPlugin::GetPluginDescription() { return "Patches Portal 2 multiplayer to work with arbitrary players and maps"; }

void CMPPatchPlugin::LevelInit(char const *pMapName) {}
void CMPPatchPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {}
void CMPPatchPlugin::GameFrame(bool simulating) {}
void CMPPatchPlugin::LevelShutdown(void) {}

void CMPPatchPlugin::ClientActive(edict_t *pEntity) {}
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