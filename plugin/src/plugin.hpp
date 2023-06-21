#ifndef H_P2MPPATCH_PLUGIN
#define H_P2MPPATCH_PLUGIN

#include <engine/iserverplugin.h>
#include <game/server/iplayerinfo.h>
#include <tier0/valve_minmax_off.h>

#include <memory>
#include <vector>
#include "module.hpp"
#include "scratchpad.hpp"
#include "patch.hpp"

class CMPPatchPlugin : public IServerPluginCallbacks {
    public:
        static const uint8_t MAX_PLAYERS = 0x20;

        CGlobalVars *get_globals() const { return m_PlayerInfoManager->GetGlobalVars(); }
        void *engine_trace() const { return m_EngineTrace; }

        //Patches
        void update_patches();

        CScratchPad& scratchpad() { return m_ScratchPad; }

        CModule& engine_module() const { return *m_EngineModule; }
        CModule& matchmaking_module() const { return *m_MatchMakingModule; }
        CModule& server_module() const { return *m_ServerModule; }

        void register_patch(std::unique_ptr<IPatch> patch) {
            patch->on_register(*this);
            m_Patches.push_back(std::move(patch));
        }
        template<typename T, typename... Args> void register_patch(Args&&... args) { register_patch(std::make_unique<T>(args...)); }

        //Server callbacks
        virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) override;
        virtual void Unload() override;
        virtual void Pause() override;
        virtual void UnPause() override;
        virtual const char *GetPluginDescription() override;

        virtual void LevelInit(char const *pMapName) override;
        virtual void LevelShutdown(void) override;
        virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) override;
        virtual void GameFrame(bool simulating) override;

        virtual void ClientActive(edict_t *pEntity) override;
        virtual void ClientDisconnect(edict_t *pEntity) override;
        virtual void ClientPutInServer(edict_t *pEntity, char const *playername) override;
        virtual void SetCommandClient(int index) override;
        virtual void ClientSettingsChanged(edict_t *pEdict) override;
        virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) override;
        virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args) override;

        virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) override;
        virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) override;
        virtual void OnEdictAllocated(edict_t *edict) override;
        virtual void OnEdictFreed(const edict_t *edict) override;

    private:
        int m_LoadCount;
        IPlayerInfoManager *m_PlayerInfoManager;
        void *m_EngineTrace;

        CScratchPad m_ScratchPad;
        CModule *m_EngineModule, *m_MatchMakingModule, *m_ServerModule;
        std::vector<std::unique_ptr<IPatchRegistrar>> m_PatchRegistrars;
        std::vector<std::unique_ptr<IPatch>> m_Patches;

        void clear_patches();
};

extern CMPPatchPlugin g_MPPatchPlugin;

#endif