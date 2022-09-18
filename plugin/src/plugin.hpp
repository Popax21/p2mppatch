#ifndef H_P2MPPATCH_PLUGIN
#define H_P2MPPATCH_PLUGIN

#include <vector>
#include <memory>
#include <engine/iserverplugin.h>
#include "module.hpp"
#include "patch.hpp"

class CMPPatchPlugin : public IServerPluginCallbacks {
    public:
        void update_patches();

        template<typename T, typename... Args> void register_patch(Args... args) {
            m_Patches.push_back(std::unique_ptr<T>(new T(args...)));
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
        CModule *m_EngineModule, *m_MatchMakingModule, *m_ServerModule;
        std::vector<std::unique_ptr<CPatch>> m_Patches;
};

#endif