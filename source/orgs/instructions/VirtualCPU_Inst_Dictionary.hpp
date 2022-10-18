/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2021-2022.
 *
 *  @file  VirtualCPU_Inst_Dictionary.hpp
 *  @brief Provides VirtualCPUOrgs instructions to store/retrieve info from a dictionary 
 * 
 */

#ifndef MABE_VIRTUAL_CPU_INST_DICTIONARY_H
#define MABE_VIRTUAL_CPU_INST_DICTIONARY_H

#include <limits>

#include "../../core/MABE.hpp"
#include "../../core/Module.hpp"
#include "../VirtualCPUOrg.hpp"

namespace mabe {

  /// \brief Provides VirtualCPUOrgs instructions to store/retrieve info from a dictionary 
  class VirtualCPU_Inst_Dictionary : public Module {
  public: 
    using org_t = VirtualCPUOrg;
    using data_t = org_t::data_t;
    using dict_t = emp::unordered_map<data_t, data_t>;
    using inst_func_t = org_t::inst_func_t;
    using this_t = VirtualCPU_Inst_Dictionary;
  private:
    int pop_id = 0; ///< ID of the population which will receive these instructions
    std::string dictionary_trait_name = "dictionary"; ///< Name of the dictionary trait

  public:
    VirtualCPU_Inst_Dictionary(mabe::MABE & control,
                    const std::string & name="VirtualCPU_Inst_Dictionary",
                    const std::string & desc="VirtualCPUOrg instructions to store/retrieve info from a dictionary")
      : Module(control, name, desc){ ; }

    ~VirtualCPU_Inst_Dictionary() {;}

    // Store a value in the organism's dictionary
    void Inst_Dict_Store(org_t& hw, const org_t::inst_t& inst){
        dict_t& dict = hw.GetTrait<dict_t>(dictionary_trait_name);
        size_t key_reg_idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
        size_t val_reg_idx = 
            inst.nop_vec.size() < 2 ? hw.GetComplementNop(key_reg_idx) : inst.nop_vec[1];
        dict[hw.regs[key_reg_idx]] = val_reg_idx;
        hw.AdvanceIP(inst.nop_vec.size() <= 2 ? inst.nop_vec.size() : 2);
    }
    
    // Retrieve a value from the organism's dictionary. Return 0 if key not present.
    void Inst_Dict_Fetch(org_t& hw, const org_t::inst_t& inst){
        dict_t& dict = hw.GetTrait<dict_t>(dictionary_trait_name);
        size_t key_reg_idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
        size_t dest_reg_idx = 
            inst.nop_vec.size() < 2 ? hw.GetComplementNop(key_reg_idx) : inst.nop_vec[1];
        data_t val = 0;
        if(dict.find(hw.regs[key_reg_idx]) != dict.end()) val = dict[key_reg_idx];
        hw.regs[dest_reg_idx] = val;
        hw.AdvanceIP(inst.nop_vec.size() <= 2 ? inst.nop_vec.size() : 2);
    }
    /// Set up variables for configuration file 
    void SetupConfig() override {
       LinkPop(pop_id, "target_pop", "Population(s) to manage.");
       LinkVar(dictionary_trait_name, "dictionary_trait_name", 
           "Name of the trait that stores the organism's dictionary");
    }

    /// Create organism traits and create dictionary instruction 
    void SetupModule() override {
      AddOwnedTrait<dict_t>(dictionary_trait_name, "VirtualCPUOrg's dictionary", {} );
      SetupFuncs();
    }

    /// Define dictionary instructions and make it available to the specified population
    void SetupFuncs(){
      ActionMap& action_map = control.GetActionMap(pop_id);
      const inst_func_t func_dict_store = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_Dict_Store(hw, inst); };
      action_map.AddFunc<void, VirtualCPUOrg&, const VirtualCPUOrg::inst_t&>(
          "dict-store", func_dict_store);
      const inst_func_t func_dict_fetch = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_Dict_Fetch(hw, inst); };
      action_map.AddFunc<void, VirtualCPUOrg&, const VirtualCPUOrg::inst_t&>(
          "dict-fetch", func_dict_fetch);
    }

  };

  MABE_REGISTER_MODULE(VirtualCPU_Inst_Dictionary, "Dictionary instructions for VirtualCPUOrg");
}

#endif
