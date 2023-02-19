/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2021-2022.
 *
 *  @file  SchedulerProbabilistic.h
 *  @brief Rations out updates to organisms based on a specified attribute, using a method akin to roulette selection. 
 **/

#ifndef MABE_SCHEDULER_PROB_H
#define MABE_SCHEDULER_PROB_H

#include "../core/MABE.hpp"
#include "../core/Module.hpp"
#include "emp/datastructs/UnorderedIndexMap.hpp"

namespace mabe {

  /// Rations out updates to organisms based on a specified attribute, using a method akin to roulette selection  
  class SchedulerProbabilistic : public Module {
  private:
    std::string trait = "merit";  ///< Which trait should we select on?
    std::string parent_trait = "";  /**<  If not empty, parent sets its trait with 
                                            this on repro */
    std::string reset_self_trait = "reset_self";  ///< What should we call the trait used to track resetting?
    double avg_updates = 0; ///< How many updates should organisms receive on average?
    int pop_id = 0;     ///< Which population are we selecting from?
    emp::UnorderedIndexMap weight_map; ///< Data structure storing all organism fitnesses
    double base_value = 1; ///< Fitness value that all organisms start with 
    double merit_scale_factor = 1; ///< Fitness = base_value + (merit * this value)
    int death_age = -1; /**< Organisms that execute death_age * genome length instructions die
                                -1 for no death from old age */
    std::string insts_executed_trait = "insts_executed"; /**< Name of the trait storing the 
                                                              number of instructions the 
                                                              organism has executed. **/
    std::string genome_length_trait = "genome_length"; /**< Name of the trait storing the 
                                                              length of the org's genome **/
  
  public:
    SchedulerProbabilistic(mabe::MABE & control,
                     const std::string & name="SchedulerProbabilistic",
                     const std::string & desc="Rations out updates to organisms based on a specified attribute, using a method akin to roulette wheel selection",
                     const std::string & in_trait="merit",
                     size_t in_avg_updates = 30)
      : Module(control, name, desc)
      , trait(in_trait), avg_updates(in_avg_updates)
      , weight_map()
    {
    }
    ~SchedulerProbabilistic() { }

    /// Set up variables for configuration file
    void SetupConfig() override {
      LinkPop(pop_id, "pop", "Which population should we select parents from?");
      LinkVar(avg_updates, "avg_updates", "How many updates should organism receive on average?");
      LinkVar(trait, "trait", "Which trait provides the fitness value to use?");
      LinkVar(parent_trait, "parent_trait", "Does nothing if empty. Otherwise, on"
          " reproduction the parent will reset their trait (as defined above) with this.");
      LinkVar(reset_self_trait, "reset_self_trait", 
          "Name of the trait tracking if an organism should reset itself");
      LinkVar(base_value, "base_value", "What value should the scheduler use for organisms"
          " that have performed no tasks?");
      LinkVar(merit_scale_factor, "merit_scale_factor", "How should the scheduler scale merit?");
      LinkVar(death_age, "death_age", 
          "Organisms die from old age after executing death_age * genome length instructions."
          " -1 for no death from old age");
      LinkVar(insts_executed_trait, "insts_executed_trait", 
          "The number of instructions this organism has executed");
    }

    /// Register traits
    void SetupModule() override {
      AddRequiredTrait<double>(trait); ///< The fitness trait must be set by another module.
      // If user has defined a parent trait, require it!
      if(parent_trait.size() > 0){
        AddRequiredTrait<double>(parent_trait);
      }
      AddOwnedTrait<bool>(reset_self_trait, "Does org need reset?", false); ///< Allow organisms to reset themselves 
      AddRequiredTrait<size_t>(insts_executed_trait); ///< Number of instructions executed
      AddRequiredTrait<size_t>(genome_length_trait); ///< Length of org's genome 
    }
    
    /// Run organisms in a population a certain number of updates or until one reproduces
    double Evaluate(const Collection & orgs, size_t max_updates, bool stop_at_birth = true) {
      mabe::Collection alive_orgs( orgs.GetAlive() );
      for (Organism & org : alive_orgs) {
        const size_t original_pop_size = alive_orgs.GetFirstPop()->GetSize();
        for(size_t update = 0; update < max_updates; ++update){
          org.ProcessStep();
          if(stop_at_birth && alive_orgs.GetFirstPop()->GetSize() > original_pop_size){
            return update; 
          }
        }
      }
      return max_updates;
    }
    
    /// Set up member functions associated with this class.
    static void InitType(emplode::TypeInfo & info) {
      info.AddMemberFunction(
          "SCHEDULE",
          [](SchedulerProbabilistic & mod) {
            return mod.Schedule();
          },
          "Perform one round of scheduling");
      info.AddMemberFunction(
          "EVAL",
          [](SchedulerProbabilistic & mod, Collection list, size_t num_updates, 
                bool stop_at_birth){ 
              return mod.Evaluate(list, num_updates, stop_at_birth); 
          }, "Run orgs in OrgList a certain number of updates or until one reproduces.");
    }

    /// Ration out updates to members of the population
    double Schedule() {
      // Grab the variables we'll use repeatedly 
      emp::Random & random = control.GetRandom();
      Population & pop = control.GetPopulation(pop_id);
      const size_t N = pop.GetNumOrgs();
      // Make sure the population isn't empty
      if (pop.GetNumOrgs() == 0) {
        return 0;
      }

      if(weight_map.GetSize() == 0) weight_map.Resize(N, base_value);
      size_t selected_idx;
      // Dole out updates
      for(size_t i = 0; i < N * avg_updates; ++i){
        const double total_weight = weight_map.GetWeight();
        if (pop.GetNumOrgs() == 0) {
          return 0;
        }
        if(total_weight > 0.0){ // TODO: cap to max weight
          selected_idx = weight_map.Index(random.GetDouble() * total_weight);
        }
        // TODO: Handle the case where we have < 0 total weight AND empty orgs in population
        else selected_idx = random.GetUInt(pop.GetSize()); // No weights -> pick randomly 
        pop[selected_idx].ProcessStep();
        if(death_age >= 0){
          const size_t num_insts_executed = 
              pop[selected_idx].GetTrait<size_t>(insts_executed_trait);
          const size_t genome_length = 
              pop[selected_idx].GetTrait<size_t>(genome_length_trait);
          if(num_insts_executed >= death_age * genome_length){
            control.ClearOrgAt({pop, selected_idx});
          }
        }
      }
      return weight_map.GetWeight();
    }

    /// When an organism is placed in a population, add its weight to the weight map
    void OnPlacement(OrgPosition placement_pos){
      Population & pop = placement_pos.Pop();
      const size_t N = pop.GetSize();
      if(weight_map.GetSize() < N){
        weight_map.Resize(N, 0);
      }
      size_t org_idx = placement_pos.Pos();
      if(pop[org_idx].IsEmpty()){
        weight_map.Adjust(org_idx, 0);
      }
      else{
        const double trait_val = placement_pos.Pop()[org_idx].GetTrait<double>(trait);
        const double full_val = base_value + (trait_val * merit_scale_factor);
        // Truncate negatives
        if(full_val <= 0.0) weight_map.Adjust(org_idx, 0.0);
        else weight_map.Adjust(org_idx, full_val);
        placement_pos.Pop()[org_idx].SetTrait<bool>(reset_self_trait, false);
      }
    }

    /// When an organism dies, set its weight to zero and refresh weights
    ///
    /// In some situations, an organisms can have HUGE fitness scores.
    /// In the case that only one organism has such a high score and then that organism dies, 
    ///     we need to make sure the other (much much lower score) are not zeroed out due to 
    ///     floating point imprecision
    void BeforeDeath(OrgPosition death_pos) override{
        size_t org_idx = death_pos.Pos();
        emp_assert(org_idx < weight_map.GetSize());
        weight_map.Adjust(org_idx, 0);
        weight_map.DeferRefresh();
    }

    void BeforeRepro(OrgPosition parent_pos) override{
      if(parent_trait.size() > 0){
        Population & pop = parent_pos.Pop();
        const size_t org_idx = parent_pos.Pos();
        const double trait_val = pop[org_idx].GetTrait<double>(parent_trait);
        const double full_val = base_value + (trait_val * merit_scale_factor);
        pop[org_idx].SetTrait<double>(trait, full_val);
        weight_map.Adjust(org_idx, full_val);
      }
    }
  };

  MABE_REGISTER_MODULE(SchedulerProbabilistic, "Rations out updates to organisms based on a specified attribute, using a method akin to roulette wheel selection.");
}

#endif
