#ifndef H_P2MPPATCH_PLUGIN
#define H_P2MPPATCH_PLUGIN

#include <vector>
#include <memory>
#include <engine/iserverplugin.h>
#include <tier0/valve_minmax_off.h>
#include "module.hpp"
#include "scratchpad.hpp"
#include "patch.hpp"

class CMPPatchPlugin : public IServerPluginCallbacks {
    public:
        static const uint8_t MAX_PLAYERS = 0x20;

        void update_patches();

        CScratchPad& scratchpad() { return m_ScratchPad; }

        const CModule& engine_module() const { return *m_EngineModule; }
        const CModule& matchmaking_module() const { return *m_MatchMakingModule; }
        const CModule& server_module() const { return *m_ServerModule; }

        template<typename T, typename... Args> void register_patch(Args... args) {
            m_Patches.emplace_back(new T(args...));
        }

        //Server callbacks
        virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
        virtual void Unload();
        virtual void Pause();
        virtual void UnPause();
        virtual const char *GetPluginDescription();

        virtual void LevelInit(char const *pMapName);
        virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
        virtual void GameFrame(bool simulating);
        virtual void LevelShutdown(void);

        virtual void ClientActive(edict_t *pEntity);
        virtual void ClientFullyConnect(edict_t *pEntity);
        virtual void ClientDisconnect(edict_t *pEntity);
        virtual void ClientPutInServer(edict_t *pEntity, char const *playername);
        virtual void SetCommandClient(int index);
        virtual void ClientSettingsChanged(edict_t *pEdict);
        virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
        virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args);

        virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);
        virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue);
        virtual void OnEdictAllocated(edict_t *edict);
        virtual void OnEdictFreed(const edict_t *edict);

    private:
        CScratchPad m_ScratchPad;
        CModule *m_EngineModule, *m_MatchMakingModule, *m_ServerModule;
        std::vector<std::unique_ptr<IPatchRegistrar>> m_PatchRegistrars;
        std::vector<std::unique_ptr<CPatch>> m_Patches;

        void clear_patches();
};

#endif