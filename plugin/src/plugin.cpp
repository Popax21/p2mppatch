#include <tier0/dbg.h>
#include <tier0/platform.h>
#include <tier1/tier1.h>
#include <tier2/tier2.h>
#include <engine/iserverplugin.h>
#include <tier0/valve_minmax_off.h>
#include "module.hpp"

struct tm *Plat_localtime(const time_t *timep, struct tm *result) {
    return localtime_r(timep, result);
}

class CMPPatchPlugin : public IServerPluginCallbacks {
    public:
        //Server callbacks
        virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
            ConnectTier1Libraries(&interfaceFactory, 1);
            ConnectTier2Libraries(&interfaceFactory, 1);

            try {
                CModule engine_mod("engine"), matchmaking_mod("matchmaking"), server_mod("server");

                CSequentialByteSequence seq STACK_SEQS(SEQ_STR("test"), SEQ_HEX("asdf"));
            } catch(const std::exception& e) {
                Warning("Exception while applying patches: %s\n", e.what());
                return false;
            }

            return true;
        }

        virtual void Unload() {
            DisconnectTier1Libraries();
            DisconnectTier2Libraries();
        }

        virtual void Pause() {}
        virtual void UnPause() {}
        virtual const char *GetPluginDescription() { return "Patches Portal 2 multiplayer to work with arbitrary players and maps"; }

        virtual void LevelInit(char const *pMapName) {}
        virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {}
        virtual void GameFrame(bool simulating) {}
        virtual void LevelShutdown(void) {}

        virtual void ClientActive(edict_t *pEntity) {}
        virtual void ClientDisconnect(edict_t *pEntity) {}
        virtual void ClientPutInServer(edict_t *pEntity, char const *playername) {}
        virtual void SetCommandClient(int index) {}
        virtual void ClientSettingsChanged(edict_t *pEdict) {}
        virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) { return PLUGIN_CONTINUE; }
        virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args) { return PLUGIN_CONTINUE; }

        virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) { return PLUGIN_CONTINUE; }
        virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) {}
        virtual void OnEdictAllocated(edict_t *edict) {}
        virtual void OnEdictFreed(const edict_t *edict) {}
};

CMPPatchPlugin g_MPPatchPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CMPPatchPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_MPPatchPlugin);