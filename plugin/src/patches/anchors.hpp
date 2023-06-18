#ifndef H_P2MPPATCH_PATCHES_ANCHORS
#define H_P2MPPATCH_PATCHES_ANCHORS

#include <tier0/dbg.h>
#include <tier0/valve_minmax_off.h>

#include <type_traits>
#include <stdexcept>
#include <sstream>
#include "byteseq.hpp"
#include "patch.hpp"

namespace patches {
    namespace anchors {
        struct SFuncAnchor : public IModuleFact<SAnchor> {
            public:
                SFuncAnchor(const char *name, const char *hex_seq, int off = 0) : m_Name(name), m_Sequence(hex_seq), m_Offset(off) {}

                const char *fact_name() const { return m_Name; }

            private:
                const char *m_Name;
                CHexSequence m_Sequence;
                int m_Offset;

                virtual SAnchor determine_value(CModule& module) {
                    SAnchor anchor = module.find_seq_anchor(m_Sequence) - m_Offset;
                    DevMsg("- &(%s) = %p\n", m_Name, anchor.get_addr());
                    anchor.mark_symbol(m_Name);
                    return anchor;
                }
        };

        template<typename T> struct SRefInstrAnchor {
            static_assert(std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, int>::value || std::is_same<T, void*>::value, "SRefInstrAnchor can only work on primitive types");

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
                SGlobVarAnchor(const char *name, IModuleFact<SAnchor>& func, int instr_off, const char *instr_hex_seq, int instr_ptr_off) : SRefInstrAnchor(name, func, instr_off, instr_hex_seq, instr_ptr_off), m_Name(name) {}

                const char *fact_name() const { return m_Name; }

            private:
                const char *m_Name;

                virtual SAnchor determine_value(CModule& module) {
                    //Obtain and check the global pointer from the instruction
                    uint8_t *glob_ptr = (uint8_t*) SRefInstrAnchor<void*>::determine_value(module);
                    if(glob_ptr < (uint8_t*) module.base_addr() || (uint8_t*) module.base_addr() + module.size() <= glob_ptr) {
                        std::string debug_str = ref_instr_anchor(module).debug_str();
                        DevMsg("Obtained out-of-bounds global variable '%s' from anchor instruction %s: %p (module '%s': %p - %p)\n", m_Name, debug_str.c_str(), glob_ptr, module.name(), module.base_addr(), (uint8_t*) module.base_addr() + module.size());

                        std::stringstream sstream;
                        sstream << "Global variable anchor instruction " << ref_instr_anchor(module).debug_str() << " references OOB module address " << glob_ptr;
                        throw std::runtime_error(sstream.str());
                    }

                    DevMsg("- &(%s) = %p\n", m_Name, glob_ptr);
                    return SAnchor(&module, glob_ptr - (uint8_t*) module.base_addr(), m_Name);
                }
        };

        struct SMemberOffAnchor : public IModuleFact<int>, SRefInstrAnchor<int> {
            public:
                SMemberOffAnchor(const char *name, IModuleFact<SAnchor>& func, int instr_off, const char *instr_hex_seq, int instr_ptr_off) : SRefInstrAnchor(name, func, instr_off, instr_hex_seq, instr_ptr_off), m_Name(name) {}
            
                const char *fact_name() const { return m_Name; }

            private:
                const char *m_Name;

                virtual int determine_value(CModule& module) {
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

            extern SFuncAnchor FUNC_numSlots_adj;

            namespace CServerGameClients {
                extern SFuncAnchor GetPlayerLimits;
            }

            namespace CPortalMPGameRules {
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

            namespace CBaseEntity {
                extern SFuncAnchor ThinkSet;
                extern SFuncAnchor SetNextThink;
            }

            namespace CPortal_Player {
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
}

#endif