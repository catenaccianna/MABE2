/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2021-2022.
 *
 *  @file  VirtualCPU_Inst_Flow.hpp
 *  @brief Provides flow control instructions to a population of VirtualCPUOrgs.
 * 
 */

#ifndef MABE_VIRTUAL_CPU_INST_FLOW_H
#define MABE_VIRTUAL_CPU_INST_FLOW_H

#include "../../core/MABE.hpp"
#include "../../core/Module.hpp"
#include "../VirtualCPUOrg.hpp"

namespace mabe {

  /// A collection of flow control instructions to be used by VirtualCPUOrgs 
  class VirtualCPU_Inst_Flow : public Module {
  public:
    using org_t = VirtualCPUOrg;
    using inst_func_t = org_t::inst_func_t;
    using this_t = VirtualCPU_Inst_Flow;
  private:
    int pop_id = 0; ///< ID of the population which will receive these instructions

  public:
    VirtualCPU_Inst_Flow(mabe::MABE & control,
                    const std::string & name="VirtualCPU_Inst_Flow",
                    const std::string & desc="Flow control instructions for VirtualCPUOrg population")
      : Module(control, name, desc) {;}
    ~VirtualCPU_Inst_Flow() { }

    
    void Inst_IfNotEqual(org_t& hw, const org_t::inst_t& inst){
      if(hw.expanded_nop_args){
        size_t idx_op_1 = inst.nop_vec.size() < 1 ? 1 : inst.nop_vec[0];
        size_t idx_op_2 = 
            inst.nop_vec.size() < 2 ? hw.GetComplementNop(idx_op_1) : inst.nop_vec[1];
        if(hw.regs[idx_op_1] == hw.regs[idx_op_2])
          hw.AdvanceIP(1);
        hw.AdvanceIP(inst.nop_vec.size() <= 2 ? inst.nop_vec.size() : 2); 
       }
      else{
        size_t idx_1 = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
        size_t idx_2 = hw.GetComplementNop(idx_1);
        if(hw.regs[idx_1] == hw.regs[idx_2])
          hw.AdvanceIP(1);
        if(inst.nop_vec.size()) hw.AdvanceIP(1); 
      }
    }
    void Inst_IfLess(org_t& hw, const org_t::inst_t& inst){
      if(hw.expanded_nop_args){
        size_t idx_op_1 = inst.nop_vec.size() < 1 ? 1 : inst.nop_vec[0];
        size_t idx_op_2 = 
            inst.nop_vec.size() < 2 ? hw.GetComplementNop(idx_op_1) : inst.nop_vec[1];
        if(hw.regs[idx_op_1] >= hw.regs[idx_op_2])
          hw.AdvanceIP(1);
        hw.AdvanceIP(inst.nop_vec.size() <= 2 ? inst.nop_vec.size() : 2); 
      }
      else{
        size_t idx_1 = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
        size_t idx_2 = hw.GetComplementNop(idx_1);
        if(hw.regs[idx_1] >= hw.regs[idx_2])
          hw.AdvanceIP(1);
        if(inst.nop_vec.size()) hw.AdvanceIP(1); 
      }
    }
    void Inst_IfLabel(org_t& hw, const org_t::inst_t& inst){
      hw.AdvanceIP(inst.nop_vec.size());
      if(!hw.CheckIfLastCopied(hw.GetComplementNopSequence(inst.nop_vec))) hw.AdvanceIP();
    }
    void Inst_MoveHeadIfNotEqual(org_t& hw, const org_t::inst_t& inst){
      if(hw.expanded_nop_args){
        size_t idx_op_1 = inst.nop_vec.size() < 1 ? 1 : inst.nop_vec[0];
        size_t idx_op_2 = 
            inst.nop_vec.size() < 2 ? hw.GetComplementNop(idx_op_1) : inst.nop_vec[1];
        size_t idx_mov_head = inst.nop_vec.size() < 3 ? 0 : inst.nop_vec[2];
        size_t idx_target_head = inst.nop_vec.size() < 4 ? 3 : inst.nop_vec[2];
        hw.AdvanceIP(inst.nop_vec.size() <= 4 ? inst.nop_vec.size() : 4); 
        if(hw.regs[idx_op_1] != hw.regs[idx_op_2]){
          size_t target_head_val = hw.inst_ptr;
          const size_t target_mod = idx_target_head % 4;
          if(     target_mod == 1) target_head_val = hw.read_head; 
          else if(target_mod == 2) target_head_val = hw.write_head; 
          else if(target_mod == 3) target_head_val = hw.flow_head; 
          const size_t mov_mod = idx_mov_head % 4;
          if(     mov_mod == 0) hw.SetIP(target_head_val);
          else if(mov_mod == 1) hw.SetRH(target_head_val);
          else if(mov_mod == 2) hw.SetWH(target_head_val);
          else if(mov_mod == 3) hw.SetFH(target_head_val);
          
        }
      }
      else{
        size_t idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
        size_t idx_2 = hw.GetComplementNop(idx);
        if(hw.regs[idx] != hw.regs[idx_2]) hw.inst_ptr = hw.flow_head; 
      }
    }
    void Inst_MoveHeadIfLess(org_t& hw, const org_t::inst_t& inst){
      if(hw.expanded_nop_args){
        size_t idx_op_1 = inst.nop_vec.size() < 1 ? 1 : inst.nop_vec[0];
        size_t idx_op_2 = 
            inst.nop_vec.size() < 2 ? hw.GetComplementNop(idx_op_1) : inst.nop_vec[1];
        size_t idx_mov_head = inst.nop_vec.size() < 3 ? 0 : inst.nop_vec[2];
        size_t idx_target_head = inst.nop_vec.size() < 4 ? 3 : inst.nop_vec[2];
        if(static_cast<int>(hw.regs[idx_op_1]) < static_cast<int>(hw.regs[idx_op_2])){
          size_t target_head_val = hw.inst_ptr;
          const size_t target_mod = idx_target_head % 4;
          if(     target_mod == 1) target_head_val = hw.read_head; 
          else if(target_mod == 2) target_head_val = hw.write_head; 
          else if(target_mod == 3) target_head_val = hw.flow_head; 
          const size_t mov_mod = idx_mov_head % 4;
          if(     mov_mod == 0) hw.SetIP(target_head_val - 1);
          else if(mov_mod == 1) hw.SetRH(target_head_val);
          else if(mov_mod == 2) hw.SetWH(target_head_val);
          else if(mov_mod == 3) hw.SetFH(target_head_val);
          
        }
      }
      else{
        size_t idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
        size_t idx_2 = hw.GetComplementNop(idx);
        if(hw.regs[idx] < hw.regs[idx_2]) hw.inst_ptr = hw.flow_head; 
      }
    }
    void Inst_IfNotZero(org_t& hw, const org_t::inst_t& inst){
      size_t idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
      if(hw.regs[idx] == 0)
        hw.AdvanceIP(1);
      if(inst.nop_vec.size()) hw.AdvanceIP(1); 
    }
    void Inst_IfEqualZero(org_t& hw, const org_t::inst_t& inst){
      size_t idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
      if(hw.regs[idx] != 0)
        hw.AdvanceIP(1);
      if(inst.nop_vec.size()) hw.AdvanceIP(1); 
    }
    void Inst_IfGreaterThanZero(org_t& hw, const org_t::inst_t& inst){
      size_t idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
      if(hw.regs[idx] <= 0)
        hw.AdvanceIP(1);
      if(inst.nop_vec.size()) hw.AdvanceIP(1); 
    }
    void Inst_IfLessThanZero(org_t& hw, const org_t::inst_t& inst){
      size_t idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
      if(hw.regs[idx] >= 0)
        hw.AdvanceIP(1);
      if(inst.nop_vec.size()) hw.AdvanceIP(1); 
    }
    void Inst_IfEqualX(org_t& hw, const org_t::inst_t& inst){
      size_t value_to_compare = 1;
      for(size_t nop_idx = 0; nop_idx < inst.nop_vec.size(); ++nop_idx){
        const size_t nop_val = inst.nop_vec[nop_idx];
        if(nop_val == 0) value_to_compare *= -1;
        else value_to_compare <<= nop_val;
      }
      if(hw.regs[1] != value_to_compare) hw.AdvanceIP(1);
      hw.AdvanceIP(inst.nop_vec.size());
    }
    void Inst_IfGreaterThanX(org_t& hw, const org_t::inst_t& inst){
      int value_to_compare = 1;
      for(size_t nop_idx = 0; nop_idx < inst.nop_vec.size(); ++nop_idx){
        const size_t nop_val = inst.nop_vec[nop_idx];
        if(nop_val == 0) value_to_compare *= -1;
        else value_to_compare <<= nop_val;
      }
      if(hw.regs[1] <= value_to_compare) hw.AdvanceIP(1);
      hw.AdvanceIP(inst.nop_vec.size());
    }

    /// Set up variables for configuration file
    void SetupConfig() override {
      LinkPop(pop_id, "target_pop", "Population(s) to manage.");
    }

    /// When config is loaded, set up functions
    void SetupModule() override {
      SetupFuncs();
    }

    /// Add the instruction specified by the config file
    void SetupFuncs(){
      ActionMap& action_map = control.GetActionMap(pop_id);
      { // If not equal
        const inst_func_t func_if_not_equ = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfNotEqual(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "IfNEqu",func_if_not_equ);
      }
      { // If less 
        const inst_func_t func_if_less = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfLess(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "IfLess", func_if_less);
      }
      { // If not 0 
        const inst_func_t func_if_not_zero = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfNotZero(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "IfNot0", func_if_not_zero);
      }
      { // If equal 0 
        const inst_func_t func_if_equal_zero = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfEqualZero(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "IfEqual0", func_if_equal_zero);
      }
      { // If greater than 0 
        const inst_func_t func_if_greater_than_zero = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfGreaterThanZero(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "IfGreaterThan0", func_if_greater_than_zero);
      }
      { // If less than 0 
        const inst_func_t func_if_less_than_zero = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfLessThanZero(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "IfLessThan0", func_if_less_than_zero);
      }
      { // If equal X 
        const inst_func_t func_if_equal_x = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfEqualX(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "IfEqualX", func_if_equal_x);
      }
      { // If greater than X 
        const inst_func_t func_if_greater_than_x = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfGreaterThanX(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "IfGreaterThanX", func_if_greater_than_x);
      }
      { // If label 
        const inst_func_t func_if_label = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_IfLabel(hw, inst); };
        action_map.AddFunc<void, org_t&,const org_t::inst_t&>(
            "IfLabel",func_if_label);
      }
      { // Move head if not equal
        const inst_func_t func_mov_head_if_not_equ = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_MoveHeadIfNotEqual(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "MoveHeadIfNEqu",func_mov_head_if_not_equ);
      }
      { // Move head if less 
        const inst_func_t func_mov_head_if_less = 
          [this](org_t& hw, const org_t::inst_t& inst){ Inst_MoveHeadIfLess(hw, inst); };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "MoveHeadIfLess",func_mov_head_if_less);
      }
    }

  };

  MABE_REGISTER_MODULE(VirtualCPU_Inst_Flow, "Flow control instructions for VirtualCPUOrg");
}

#endif
