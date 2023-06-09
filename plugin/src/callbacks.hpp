#ifndef H_P2MPPATCH_CALLBACKS
#define H_P2MPPATCH_CALLBACKS

#include <edict.h>
#include <tier1/interface.h>
#include <tier1/KeyValues.h>

#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS "ISERVERPLUGINCALLBACKS003"

typedef enum  {
    PLUGIN_CONTINUE,
    PLUGIN_OVERRIDE,
    PLUGIN_STOP,
} PLUGIN_RESULT;

typedef enum {
    eQueryCvarValueStatus_ValueIntact,
    eQueryCvarValueStatus_CvarNotFound,
    eQueryCvarValueStatus_NotACvar,
    eQueryCvarValueStatus_CvarProtected
} EQueryCvarValueStatus;

typedef int QueryCvarCookie_t;
#define InvalidQueryCvarCookie -1

struct edict_t;

class IServerPluginCallbacks {
    public:
        virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory ) = 0;
        virtual void Unload(void) = 0;
        virtual void Pause(void) = 0;
        virtual void UnPause(void) = 0;
        virtual const char *GetPluginDescription(void) = 0;
        virtual void LevelInit(char const *pMapName) = 0;
        virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) = 0;
        virtual void GameFrame(bool simulating) = 0;
        virtual void LevelShutdown(void) = 0;
        virtual void ClientActive(edict_t *pEntity) = 0;
        virtual void ClientFullyConnect(edict_t *pEntity) = 0;
        virtual void ClientDisconnect(edict_t *pEntity) = 0;
        virtual void ClientPutInServer(edict_t *pEntity, char const *playername) = 0;
        virtual void SetCommandClient(int index) = 0;
        virtual void ClientSettingsChanged(edict_t *pEdict) = 0;
        virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) = 0;
        virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args) = 0;
        virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) = 0;
        virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) = 0;
        virtual void OnEdictAllocated(edict_t *edict) = 0;
        virtual void OnEdictFreed(const edict_t *edict ) = 0;	
};

#endif