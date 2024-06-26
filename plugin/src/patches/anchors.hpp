#ifndef H_P2MPPATCH_PATCHES_ANCHORS
#define H_P2MPPATCH_PATCHES_ANCHORS

#include <tier0/dbg.h>
#include <tier0/valve_minmax_off.h>

#include <memory>
#include <stdexcept>
#include <sstream>
#include <type_traits>
#include "byteseq.hpp"
#include "patch.hpp"
#include "hook.hpp"

namespace patches::anchors {
    struct SFuncAnchor : public IModuleFact<SAnchor> {
        public:
            SFuncAnchor(const char *name, const char *hex_seq, int off = 0, const char *discrim_seq = nullptr, int discrim_off = 0) : m_Name(name), m_Sequence(hex_seq), m_Offset(off) {
                if(discrim_seq) {
                    m_DiscrimSequence = std::make_unique<CMaskedHexSequence>(discrim_seq);
                    m_DiscrimOff = discrim_off;
                }
            }

            SFuncAnchor(const char *name, const char *hex_seq, const char *begin_hex_seq, int off = 0, const char *discrim_seq = nullptr, int discrim_off = 0) : SFuncAnchor(name, hex_seq, off, discrim_seq, discrim_off) {
                m_FuncBeginSequence = std::make_unique<CMaskedHexSequence>(begin_hex_seq);
                m_HookTrackerFact = std::make_unique<CHookTrackerFact>(*this, *m_FuncBeginSequence);
            }

            virtual const char *fact_name() const override { return m_Name; }

            CHookTracker& hook_tracker(CModule& module) const {
                if(!m_HookTrackerFact) {
                    std::stringstream sstream;
                    sstream << "Function '" << m_Name << "' has not been set up with support for hooking";
                    throw std::runtime_error(sstream.str());
                }

                return m_HookTrackerFact->get(module);
            }

        private:
            const char *m_Name;
            CHexSequence m_Sequence;
            int m_Offset;
            std::unique_ptr<CMaskedHexSequence> m_DiscrimSequence;
            int m_DiscrimOff;

            std::unique_ptr<CMaskedHexSequence> m_FuncBeginSequence;
            std::unique_ptr<CHookTrackerFact> m_HookTrackerFact;

            virtual SAnchor determine_value(CModule& module) override {
                try {
                    SAnchor anchor = module.find_seq_anchor(m_Sequence, m_DiscrimSequence ? &*m_DiscrimSequence : nullptr, m_DiscrimOff) - m_Offset;
                    DevMsg("- &(%s) = %p\n", m_Name, anchor.get_addr());
                    anchor.mark_symbol(m_Name);
                    return anchor;
                } catch(const std::exception& e) {
                    std::stringstream sstream;
                    sstream << "Failed to resolve function '" << m_Name << "': " << e.what();
                    throw std::runtime_error(sstream.str());
                }
            }
    };

    template<typename T> struct SRefInstrAnchor {
        static_assert(std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, int>::value || std::is_same<T, void*>::value, "SRefInstrAnchor only works with primitive types");

        public:
            SRefInstrAnchor(const char *name, IModuleFact<SAnchor>& func, int instr_off, const char *instr_hex_seq, int instr_val_off) : m_Name(name), m_Func(func), m_RefInstrOffset(instr_off), m_RefInstrSequence(instr_hex_seq), m_RefInstrValOff(instr_val_off) {}

            SAnchor ref_instr_anchor(CModule& module) const { return m_Func.get(module) + m_RefInstrOffset; }
            T determine_value(CModule& module) const;

        private:
            const char *m_Name;
            IModuleFact<SAnchor>& m_Func;
            int m_RefInstrOffset;
            CMaskedHexSequence m_RefInstrSequence;
            int m_RefInstrValOff;
    };

    struct SGlobVarAnchor : public IModuleFact<SAnchor>, SRefInstrAnchor<void*> {
        public:
            SGlobVarAnchor(const char *name, IModuleFact<SAnchor>& func, int instr_off, const char *instr_hex_seq, int instr_ptr_off, int var_off = 0) : SRefInstrAnchor(name, func, instr_off, instr_hex_seq, instr_ptr_off), m_Name(name), m_VarOffset(var_off) {}

            virtual const char *fact_name() const override { return m_Name; }

        private:
            const char *m_Name;
            int m_VarOffset;

            virtual SAnchor determine_value(CModule& module) override {
                //Obtain and check the global pointer from the instruction
                uint8_t *glob_ptr = (uint8_t*) SRefInstrAnchor<void*>::determine_value(module) + m_VarOffset;
                if(glob_ptr < (uint8_t*) module.base_addr() || (uint8_t*) module.base_addr() + module.size() <= glob_ptr) {
                    DevMsg("Obtained out-of-bounds global variable '%s' from anchor instruction %s: %p (module '%s': %p - %p)\n", m_Name, ref_instr_anchor(module).debug_str().c_str(), glob_ptr, module.name(), module.base_addr(), (uint8_t*) module.base_addr() + module.size());

                    std::stringstream sstream;
                    sstream << "Global variable anchor instruction " << ref_instr_anchor(module) << " references OOB module address " << glob_ptr;
                    throw std::runtime_error(sstream.str());
                }

                DevMsg("- &(%s) = %p\n", m_Name, glob_ptr);
                return SAnchor(&module, glob_ptr - (uint8_t*) module.base_addr(), m_Name);
            }
    };

    struct SMemberOffAnchor : public IModuleFact<int>, SRefInstrAnchor<int> {
        public:
            SMemberOffAnchor(const char *name, IModuleFact<SAnchor>& func, int instr_off, const char *instr_hex_seq, int instr_ptr_off) : SRefInstrAnchor(name, func, instr_off, instr_hex_seq, instr_ptr_off), m_Name(name) {}
        
            virtual const char *fact_name() const override { return m_Name; }

        private:
            const char *m_Name;

            virtual int determine_value(CModule& module) override {
                int member_off = SRefInstrAnchor<int>::determine_value(module);
                DevMsg("- offsetof(%s) = 0x%x\n", m_Name, member_off);
                return member_off;
            }
    };

    //>>>>> server anchors <<<<<

    namespace server {
        extern SGlobVarAnchor g_pGameRules;
        extern SGlobVarAnchor g_pMatchFramework;

        extern SFuncAnchor UTIL_PlayerByIndex;
        extern SFuncAnchor UTIL_DropToFloor;

        extern SFuncAnchor FUNC_numSlots_adj;

        namespace CServerGameClients {
            extern SFuncAnchor GetPlayerLimits;
        }

        namespace CPortalMPGameRules {
            extern SMemberOffAnchor m_bDataReceived;

            extern SFuncAnchor CPortalMPGameRules;
            extern SFuncAnchor destr_CPortalMPGameRules;

            extern SFuncAnchor ClientCommandKeyValues;
            extern SFuncAnchor ClientDisconnected;

            extern SFuncAnchor SetMapCompleteData;
            extern SFuncAnchor SendAllMapCompleteData;
            extern SFuncAnchor StartPlayerTransitionThinks;
        }

        namespace CGameMovement {
            extern SMemberOffAnchor m_LastStuck;
            extern SFuncAnchor CheckStuck;
        }

        extern SFuncAnchor ServerClassInit_DT_BaseEntity;
        namespace CBaseEntity {
            extern SMemberOffAnchor m_MoveType;
            extern SMemberOffAnchor m_CollisionGroup;
            extern SFuncAnchor SetMoveType;
            extern SFuncAnchor SetCollisionGroup;
            extern SFuncAnchor CollisionRulesChanged;

            extern SFuncAnchor ThinkSet;
            extern SFuncAnchor SetNextThink;

            extern SFuncAnchor GetGroundEntity;
        }

        namespace CCollisionEvent {
            extern SFuncAnchor ShouldCollide;
        }

        extern SGlobVarAnchor g_pLastSpawn;
        namespace CBasePlayer {
            extern SFuncAnchor EntSelectSpawnPoint;
        }

        namespace CPortal_Player {
            extern SFuncAnchor EntSelectSpawnPoint;
            extern SFuncAnchor Spawn;

            extern SFuncAnchor ShouldCollide;

            extern SFuncAnchor ClientCommand;

            extern SFuncAnchor OnFullyConnected;
            extern SFuncAnchor PlayerTransitionCompleteThink;
        }

        namespace CProp_Portal {
            extern SFuncAnchor Spawn; //Only used to find g_pMatchFramework 
            extern SFuncAnchor DispatchPortalPlacementParticles; //Only used to find g_pGameRules
        }

        extern SFuncAnchor DataMapInit_CTriggerMultiple;
        namespace CTriggerMultiple {
            extern SGlobVarAnchor typedescription_m_OnTrigger;

            extern SMemberOffAnchor m_flWait;

            extern SFuncAnchor MultiTouch;
        }

        namespace CTriggerOnce {
            extern SFuncAnchor Spawn;
        }

        namespace CBaseEntityOutput {
            extern SFuncAnchor Restore;

            extern SMemberOffAnchor m_ActionList;
        }

        namespace CPointTeleport {
            extern SFuncAnchor DoTeleport;
        }

        namespace CEnvFade {
            extern SFuncAnchor InputFade;
            extern SFuncAnchor InputReverseFade;
        }
    }

    //>>>>> matchmaking anchors <<<<<

    namespace matchmaking {
        namespace CMatchTitleGameSettingsMgr {
            extern SFuncAnchor InitializeGameSettings;
        }
    }

    //>>>>> engine anchors <<<<<

    namespace engine {
        extern SGlobVarAnchor sv;

        extern SFuncAnchor SV_Frame; //Only used to find the global sv variable

        namespace CGameServer {
            extern SFuncAnchor InitMaxClients;
        }

        //Present in every lib as part of the static tier0 lib, but take it from the engine
        //Don't just use the SDK's tier0, as it might have a different layout
        namespace KeyValues {
            extern SFuncAnchor GetInt;
        }
    }
}

#endif