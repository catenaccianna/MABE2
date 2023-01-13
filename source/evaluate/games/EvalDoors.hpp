/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2022-2022.
 *
 *  @file  EvalDoors.hpp
 *  @brief MABE Evaluation module that places the organism in a room with N doors. 
 *          In each room, a symbol indicates which door is the correct one. 
 *          The rooms are configurable such that the symbol is either set or random between 
 *          trials. 
 *          One door is the "exit" door. If the wrong door is chosen, the next room
 *          shows the "wrong choice" symbol, and organisms should then return to the 
 *          previous room via the exit door.
 * 
 * Notes: 
 *    - If an organism has taken a wrong door and now should take the exit, we say they are in
 *        an "exit room"
 *      - Otherwise, they are in a "door room"
 *
 */

#ifndef MABE_EVAL_DOORS_HPP
#define MABE_EVAL_DOORS_HPP

#include "../../core/MABE.hpp"
#include "../../core/Module.hpp"
#include "../../orgs/VirtualCPUOrg.hpp"

#include "emp/bits/BitVector.hpp"
#include "emp/io/File.hpp"
#include "emp/tools/string_utils.hpp"

namespace mabe {
    
  /// \brief A collection of all the trait names used in EvalDoors
  struct EvalDoors_TraitNames{
    std::string score_trait = "score"; ///< Name of trait for organism performance
    std::string accuracy_trait = "accuracy"; ///< Name of trait for organism accuracy
    std::string state_trait ="state";  ///< Name of trait that stores the task state
    std::string door_rooms_trait ="door_rooms"; /**< Name of trait that stores the number of
                                                     "door rooms" visited */
    std::string exit_rooms_trait ="exit_rooms"; /**< Name of trait that stores the number of
                                                     "exit rooms" visited */
    std::string correct_doors_trait ="correct_doors"; /**< Name of trait that stores the 
                                                           number of doors correctly taken*/
    std::string incorrect_doors_trait ="incorrect_doors"; /**< Name of trait that stores the
                                                           number of doors incorrectly
                                                           taken*/
    std::string correct_exits_trait ="correct_exits"; /**< Name of trait that stores the 
                                                           number of exits correctly taken*/
    std::string incorrect_exits_trait ="incorrect_exits"; /**< Name of trait that stores the 
                                                           number of exits incorrectly taken*/
    std::string exit_cooldown_trait ="exit_cooldown"; /**< Name of trait that stores the 
                                                           orgs current exit cooldown*/
    std::string doors_taken_prefix = "doors_taken_"; /**< Prefix for multiple traits 
                                                          (one per door) */
    std::string doors_correct_prefix = "doors_correct_"; /**< Prefix for multiple traits 
                                                              (one per door)*/
    emp::vector<std::string> doors_taken_trait_vec; // Names of doors taken traits
    emp::vector<std::string> doors_correct_trait_vec; // Names of doors correct traits
  };

  /// \brief State of a single organism's progress on the doors task
  struct DoorsState{
    using data_t = uint32_t;

    bool initialized = false;     ///< Flag indicating if this state has been initialized
    size_t last_room_cue;         ///< Cue of the last "door room" 
    double score = 0;             ///< Summarized score of the organism 
    emp::vector<data_t> cue_vec;  ///< Vector containing the value of each cue (random or not)
    data_t current_cue;           ///< Cue of the current room the organism is in
    size_t correct_doors_taken = 0;   ///< Number of times the org entered the correct door
    size_t incorrect_doors_taken = 0; ///< Number of times the org entered the wrong door
    size_t correct_exits_taken = 0;   /**< Number of times the org took the exit when it 
                                           should have */
    size_t incorrect_exits_taken = 0; /**< Number of times the org took the exit when it 
                                           should *not* have */
    size_t door_rooms_visited = 0; ///< Number of "door" rooms the organism has visited
    size_t exit_rooms_visited = 0; ///< Number of "exit" rooms the organism has visited
    emp::vector<size_t> doors_taken_vec; ///< Number of times each door was taken
    emp::vector<size_t> doors_correct_vec; ///< Number of times each door was taken correctly
    size_t path_start_pattern_idx = 0; /**< If path start patterns are given, this is the one the organism is currently on */
    size_t path_start_pattern_tracker = 0; ///< How far along the start pattern is the org?

    DoorsState() { ; }
    DoorsState(const DoorsState&){ // Ignore copy, just reset
      initialized = false;
      score = 0;
    }
    DoorsState(DoorsState&&){ // Ignore move, just reset
      initialized = false;
      score = 0;
    }
    DoorsState& operator=(DoorsState&){ // Ignore copy assignment, just reset
      initialized = false;
      score = 0;
      return *this;
    }
    DoorsState& operator=(DoorsState&&){ // Ignore move assignment, just reset
      initialized = false;
      score = 0;
      return *this;
    }
  };

  /// \brief Handles all evaluation of the doors task
  struct DoorsEvaluator{
    public:
    using org_t = VirtualCPUOrg;

    public:
    bool verbose = false; ///< Do we print extra info?
    double correct_doors_factor = 1.0; ///< Reward for getting a door correct
    double correct_exits_factor = 0.0; ///< Reward for getting an exit correct
    double incorrect_doors_factor = 1.0; ///< Penalty for getting a door wrong
    double incorrect_exits_factor = 1.0; ///< Penalty for getting an exit wrong
    double incorrect_doors_step = 0.0; ///< Step value, the penalty for each wrong door increases by this amount 
    protected:
    emp::Random& rand;  ///< Reference to the main random number generator of MABE
    emp::vector<int> starting_cue_vec; /**< Vector of set cue values or random cue 
                                            indicators (-1) */ 
    emp::vector<emp::vector<size_t>> path_start_pattern_vec; /**< Users can define multiple 
        "start patterns" that dictate the first few cues seen by organisms. 
        This stores all of those start patterns */
    const size_t exit_cue_idx = 0; ///< Index of the exit in the cue vector 
    
    /// Move the organism through the "exit" door, going back one room  
    double TakeExit(DoorsState& state){
      if(!state.initialized) InitializeState(state);
      // Update score vars and current cue
      if(state.current_cue == state.cue_vec[exit_cue_idx]){
        state.correct_exits_taken++;
        state.current_cue = state.last_room_cue; // Return to previous room
        state.doors_correct_vec[exit_cue_idx]++;
      }
      else{
        state.incorrect_exits_taken++;
        state.current_cue = state.cue_vec[exit_cue_idx];
      }
      return UpdateScore(state);
    }
    
    public: 
    DoorsEvaluator(emp::Random& _rand) : rand(_rand) { ; } 

    /// Fetch the number of doors in each room (includes exit)
    size_t GetNumDoors() const { return starting_cue_vec.size(); }
    
    /// Calculate the score for the given state
    double GetScore(const DoorsState& state) const{
      double score = 1.0 
          + (state.correct_doors_taken * correct_doors_factor)
          + (state.correct_exits_taken * correct_exits_factor)
          - (state.incorrect_doors_taken * 
              (incorrect_doors_factor + incorrect_doors_step * state.incorrect_doors_taken)) 
          - (state.incorrect_exits_taken * incorrect_exits_factor);
      // Truncate negative scores
      return (score >= 0) ? score : 0;
    }

    /// Calculate and store the score for the given state
    double UpdateScore(DoorsState& state){
      state.score = GetScore(state);
      return state.score;
    }

    /// Updates the records in the organism's traits
    void UpdateRecords(const DoorsState& state, org_t& org, 
        const EvalDoors_TraitNames& trait_names){
      org.SetTrait<size_t>(trait_names.door_rooms_trait, state.door_rooms_visited);
      org.SetTrait<size_t>(trait_names.exit_rooms_trait, state.exit_rooms_visited);
      org.SetTrait<size_t>(trait_names.correct_doors_trait, state.correct_doors_taken);
      org.SetTrait<size_t>(trait_names.incorrect_doors_trait, state.incorrect_doors_taken);
      org.SetTrait<size_t>(trait_names.correct_exits_trait, state.correct_exits_taken);
      org.SetTrait<size_t>(trait_names.incorrect_exits_trait, state.incorrect_exits_taken);
      for(size_t door_idx = 0; door_idx < GetNumDoors(); ++door_idx){
        org.SetTrait<size_t>(trait_names.doors_taken_trait_vec[door_idx], 
            state.doors_taken_vec[door_idx]);
        org.SetTrait<size_t>(trait_names.doors_correct_trait_vec[door_idx], 
            state.doors_correct_vec[door_idx]);

      }
    }

    /// Calculate the score for the given state
    double GetDoorAccuracy(const DoorsState& state) const{
      if(state.door_rooms_visited <= 0) return 0;
      return static_cast<double>(state.correct_doors_taken) 
             / static_cast<double>(state.door_rooms_visited);
    }

    /** Extract cues from the given string. Can either be non-negative (used as is) or
            -1 (randomized for each trial) */
    void ParseCues(const std::string& input_str){ 
      std::string s(input_str);
      // Remove all trailing ;
      while(s[s.length() - 1] == ';') s = s.substr(0, s.length() - 1); 
      starting_cue_vec.clear();
      emp::vector<std::string> sliced_str_vec;
      emp::slice(s, sliced_str_vec, ';');
      std::cout << "Eval doors starting cue values: " << std::endl << "\t"; 
      for(std::string& slice : sliced_str_vec){
        const int cue = std::stoi(slice);
        if(cue < -1) emp_error("Error! ParseCues expects values of -1 or greater!");
        if(cue == -1) std::cout << "[random] ";
        else std::cout << "[set: " << cue << "] ";
        starting_cue_vec.push_back(cue);
      }
      std::cout << std::endl;
    }
    
    /** Extract start patterns from the given string. Patterns are separated by semicolons. 
            Values in each pattern are comma separated */
    void ParsePathStartPatterns(const std::string& input_str){ 
      std::string s(input_str);
      // Remove all trailing ;
      while(s[s.length() - 1] == ';') s = s.substr(0, s.length() - 1); 
      path_start_pattern_vec.clear();
      emp::vector<std::string> pattern_vec;
      emp::vector<std::string> index_str_vec;
      emp::slice(s, pattern_vec, ';');
      std::cout << "Number of EvalDoors path start patterns: " << pattern_vec.size() 
                << std::endl; 
      for(std::string& pattern : pattern_vec){
        index_str_vec.clear();
        emp::vector<size_t> index_vec;
        emp::slice(pattern, index_str_vec, ',');
        for(std::string& index_str : index_str_vec){
          const size_t index = std::stoull(index_str);
          if(index == 0){
            emp_error("Error! ParsePathStartPatterns expects values of 1 or greater!");
          }
          else if (index >= starting_cue_vec.size()){
            emp_error("Error! ParsePathStartPatterns received an index greater than (or "
                " equal to) the number of cues!",
                index, starting_cue_vec.size());
          }
          index_vec.push_back(index);
        }
        path_start_pattern_vec.push_back(index_vec);
      }
    }
    
   /// Fetch a random door cue from the set
    DoorsState::data_t GetRandomCue(const DoorsState& state){
      // Offset so we don't return the exit cue
      return state.cue_vec[(rand.GetUInt() % (GetNumDoors() - 1)) + 1];
    }

    /// Fetch a cue, respecting start patterns on the path 
    DoorsState::data_t GetNextCue(const DoorsState& state){
      if(path_start_pattern_vec.size() > 0){
        const emp::vector<size_t> pattern = 
            path_start_pattern_vec[state.path_start_pattern_idx];
        if(state.path_start_pattern_tracker < pattern.size()){
          return state.cue_vec[pattern[state.path_start_pattern_tracker]];
        }
      }
      return GetRandomCue(state);
    }


    /// Initialize all properties of a DoorsState to prepare it for the task
    void InitializeState(DoorsState& state){
      state.initialized = true;
      state.score = 0;
      state.correct_doors_taken = 0;   
      state.incorrect_doors_taken = 0; 
      state.correct_exits_taken = 0;   
      state.incorrect_exits_taken = 0; 
      state.door_rooms_visited = 0; 
      state.exit_rooms_visited = 0; 
      state.doors_taken_vec.resize(GetNumDoors(), 0);
      state.doors_correct_vec.resize(GetNumDoors(), 0);
      // Randomize the cue vector according to configuration
      state.cue_vec.resize(GetNumDoors());
      // First pass, add all set cues 
      for(size_t idx = 0; idx < GetNumDoors(); ++idx){
        if(starting_cue_vec[idx] >= 0){
          state.cue_vec[idx] = starting_cue_vec[idx];
        }
      }
      // Second pass, randomize other cues
      for(size_t idx = 0; idx < GetNumDoors(); ++idx){
        if(starting_cue_vec[idx] == -1){
          bool pass = false;
          while(!pass){
            pass = true;
            state.cue_vec[idx] = rand.GetUInt();
            // Ensure we didn't choose an existing cue
            for(size_t idx_2 = 0; idx_2 < GetNumDoors(); ++idx_2){
              if(idx != idx_2 && state.cue_vec[idx] == state.cue_vec[idx_2]){
                pass = false;
                break;
              }
            }
          }
        }
      }
      if(path_start_pattern_vec.size() > 0){
        state.path_start_pattern_tracker = 0;
        state.path_start_pattern_idx = rand.GetUInt(path_start_pattern_vec.size()); 
      }
      // Set the initial cue
      state.current_cue = GetNextCue(state);
      state.last_room_cue = state.current_cue;
    }
    
    /// Move the organism through its chosen door
    double Move(DoorsState& state, DoorsState::data_t door_idx){
      if(!state.initialized) InitializeState(state);
      if(verbose){
        size_t target_idx = 0;
        for(size_t i = 0; i < GetNumDoors(); ++i){
          if(state.cue_vec[i] == state.current_cue){
            target_idx = i;
            break;
          }
        }
        std::cout << "[DOORS] " << door_idx << "," << target_idx << std::endl;
      }
      // Increase bookkeeping variables
      state.doors_taken_vec[door_idx]++;
      if(state.current_cue == state.cue_vec[exit_cue_idx]) state.exit_rooms_visited++;
      else state.door_rooms_visited++;
      if(door_idx == exit_cue_idx) return TakeExit(state);
      // Correct door -> Reward and move on!
      if(state.cue_vec[door_idx] == state.current_cue){
        state.path_start_pattern_tracker++;
        state.correct_doors_taken++;
        state.doors_correct_vec[door_idx]++;
        state.current_cue = GetNextCue(state);
        state.last_room_cue = state.current_cue;
      }
      // Wrong door -> Penalize and move into "wrong" room
      else{
        state.incorrect_doors_taken++;
        state.last_room_cue = state.current_cue;
        state.current_cue = state.cue_vec[exit_cue_idx];
      }
      return UpdateScore(state);
    }


    /// Fetch the cue value of the organism's current room 
    //
    // Note: While it sounds like this should be a const method, it is possible this is the
    //  organism's first action, so we may need to initialize it
    DoorsState::data_t Sense(DoorsState& state) { 
      if(!state.initialized) InitializeState(state);
      return state.current_cue;
    }
  };

  /// \brief MABE module that evaluates Avida-esque organisms on how well they can associate cues to doors 
  class EvalDoors : public Module {
  public:
    using data_t = uint32_t;
    using org_t = VirtualCPUOrg;
    using inst_func_t = org_t::inst_func_t;

  protected:
    DoorsEvaluator evaluator;          /**< The evaluator that does all of the actual 
                                            computation and bookkeeping for the task*/
    int pop_id = 0;                    /**< ID of the population to evaluate and provide 
                                            instructions to */
    std::string cues_str; /**< String version of a vector of cue values. Non-negative values 
                               are used as is, while -1 gives a random value for each trial */
    std::string start_patterns_str; /**< String version of a vector of vectors indicating 
                                            possible start patterns for the path. */
    EvalDoors_TraitNames trait_names;   /**<  Struct holding all of the trait names to keep 
                                              things tidy */
    size_t exit_cooldown = 0; /**< How long of a cooldown to apply when exit is taken **/
    size_t exit_cooldown_step = 0;/**< How much exit_cooldown increases when exit is taken **/
    size_t exit_cooldown_step_req = 1;/**< How many exits are required before 
                                              exit_cooldown is increased **/
    double score_exp_base = 2; /**< Merit is this value raised to the "score" power **/
    
  public:
    EvalDoors(mabe::MABE & control,
                const std::string & name="EvalDoors",
                const std::string & desc="Evaluate organisms by how well they can associate symbols to doors.")
      : Module(control, name, desc)
      , evaluator(control.GetRandom())
    {
      SetEvaluateMod(true);
    }
    ~EvalDoors() { }

    /// Set up variables for configuration script
    void SetupConfig() override {
      LinkPop(pop_id, "target_pop", "Population to evaluate.");
      LinkVar(evaluator.verbose, "verbose", 
           "Should we print extra info?");
      LinkVar(evaluator.correct_doors_factor, "correct_door_reward", 
           "Reward for getting a single door correct");
      LinkVar(evaluator.correct_exits_factor, "correct_exit_reward", 
           "Reward for getting a single exit correct");
      LinkVar(evaluator.incorrect_doors_factor, "incorrect_door_penalty", 
           "Penalty for getting a single door incorrect");
      LinkVar(evaluator.incorrect_doors_step, "incorrect_door_step", 
           "How much the incorrect door penalty increases each time it is applied");
      LinkVar(evaluator.incorrect_exits_factor, "incorrect_exit_penalty", 
           "Penalty for getting a single exit incorrect");
      LinkVar(trait_names.score_trait, "score_trait", 
           "Which trait stores task performance?");
      LinkVar(trait_names.accuracy_trait, "accuracy_trait", 
          "Which trait stores organism accuracy?");
      LinkVar(trait_names.state_trait, "state_trait", 
          "Which trait stores organisms' task state?");
      LinkVar(trait_names.door_rooms_trait, "door_rooms_trait", 
          "Which trait stores the number of \"door rooms\" visited?");
      LinkVar(trait_names.exit_rooms_trait, "exit_rooms_trait", 
          "Which trait stores the number of \"exit rooms\" visited?");
      LinkVar(trait_names.correct_doors_trait, "correct_doors_trait", 
          "Which trait stores the number of doors correctly taken?");
      LinkVar(trait_names.incorrect_doors_trait, "incorrect_doors_trait", 
          "Which trait stores the number of doors incorrectly taken?");
      LinkVar(trait_names.correct_exits_trait, "correct_exits_trait", 
          "Which trait stores the number of exits correctly taken?");
      LinkVar(trait_names.incorrect_exits_trait, "incorrect_exits_trait", 
          "Which trait stores the number of exits incorrectly taken?");
      LinkVar(trait_names.doors_taken_prefix, "doors_taken_prefix", 
          "Prefix of multiple traits (one per door) for the number of times that door "
          "was taken");
      LinkVar(trait_names.doors_correct_prefix, "doors_correct_prefix", 
          "Prefix of multiple traits (one per door) for the number of times that door "
          "was taken correctly");
      LinkVar(cues_str, "cue_values", "A semicolon-separated string of cue values. " 
          "A non-negative value is used as is, -1 gives a random cue for each trial "
          "(first value is the exit)");
      LinkVar(start_patterns_str, "start_patterns", 
          "List of all possible start patterns for the paths. Empty for random. " 
          "Semicolons separate patterns, while commas separate door indices in each pattern. "
          "Indices start at 1 for non-exit doors.");
      LinkVar(exit_cooldown, "exit_cooldown",
          "How many instruction executions the org will miss after taking an exit");
      LinkVar(exit_cooldown_step, "exit_cooldown_step",
          "How much exit_cooldown increasess each time the org takes an exit");
      LinkVar(exit_cooldown_step_req, "exit_cooldown_step_req",
          "How many exits are required before exit_cooldown increases by exit_cooldown_step");
      LinkVar(score_exp_base, "score_exp_base",
          "Merit is equal to score_exp_base^(org's score). "
          "A base of zero instead just returns the exponent itself.");
    }
    
    /// Set up organism traits, load maps, and provide instructions to organisms
    void SetupModule() override {
      evaluator.ParseCues(cues_str);
      evaluator.ParsePathStartPatterns(start_patterns_str);
      AddSharedTrait<double>(trait_names.score_trait, "EvalDoors score", 0.0);
      AddSharedTrait<double>(trait_names.accuracy_trait, "EvalDoors accuracy", 0.0);
      AddOwnedTrait<DoorsState>(trait_names.state_trait, "Organism's EvalDoors state", { }); 
      AddOwnedTrait<size_t>(trait_names.door_rooms_trait, "\"Door rooms\" visited", 0);
      AddOwnedTrait<size_t>(trait_names.exit_rooms_trait, "\"Exit rooms\" visited", 0);
      AddOwnedTrait<size_t>(trait_names.correct_doors_trait, "Correct doors taken", 0);
      AddOwnedTrait<size_t>(trait_names.incorrect_doors_trait, "Incorrect doors taken", 0);
      AddOwnedTrait<size_t>(trait_names.correct_exits_trait, "Correct exits taken", 0);
      AddOwnedTrait<size_t>(trait_names.incorrect_exits_trait, "Incorrect exits taken", 0);
      AddOwnedTrait<size_t>(trait_names.exit_cooldown_trait, "Exit cooldown", exit_cooldown);
      for(size_t door_idx = 0; door_idx < evaluator.GetNumDoors(); ++door_idx){
        trait_names.doors_taken_trait_vec.push_back(
            trait_names.doors_taken_prefix + emp::to_string(door_idx));
        trait_names.doors_correct_trait_vec.push_back(
            trait_names.doors_correct_prefix + emp::to_string(door_idx));
        AddOwnedTrait<size_t>(trait_names.doors_taken_trait_vec[door_idx], 
            "Number of times door #" + emp::to_string(door_idx) + "was taken", 0);
        AddOwnedTrait<size_t>(trait_names.doors_correct_trait_vec[door_idx], 
            "Number of times door #" + emp::to_string(door_idx) + "was correctly taken", 0);

      }
      SetupInstructions();
    }
    
    /// Package actions (e.g., sense, take door N) into instructions and provide them to the 
    /// organisms via ActionMap
    void SetupInstructions(){
      ActionMap& action_map = control.GetActionMap(pop_id);
      // Add the correct number of door instructions
      for(size_t door_idx = 0; door_idx < evaluator.GetNumDoors(); ++door_idx){
        inst_func_t func_move = [this, door_idx](org_t& hw, const org_t::inst_t& /*inst*/){
          DoorsState& state = hw.GetTrait<DoorsState>(trait_names.state_trait);
          double score = evaluator.Move(state, door_idx);
          if(score_exp_base == 0){
            hw.SetTrait<double>(trait_names.score_trait, score);
          }
          else{
            if(score > 300) score = 300; // Cap to avoid infinite values
            hw.SetTrait<double>(trait_names.score_trait, std::pow(score_exp_base, score));
          }
          hw.SetTrait<double>(trait_names.accuracy_trait, evaluator.GetDoorAccuracy(state));
          evaluator.UpdateRecords(state, hw, trait_names);
          if(door_idx == 0){
            // Increase the cooldown and update cooldown value if needed
            if(exit_cooldown_step > 0){
              size_t & org_cooldown = hw.GetTrait<size_t>(trait_names.exit_cooldown_trait);
              hw.IncreaseCooldown(org_cooldown);
              const size_t exits_taken = 
                  hw.GetTrait<size_t>(trait_names.doors_taken_trait_vec[0]);
              if(exits_taken % exit_cooldown_step_req == 0){
                org_cooldown += exit_cooldown_step;
              }
            }
            else{ // Constant cooldown
              hw.IncreaseCooldown(exit_cooldown);
            }
          }
        };
        std::stringstream sstr;
        sstr << "doors-move-" << door_idx;
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            sstr.str(), func_move);
      }
      { // Sense 
        inst_func_t func_sense = [this](org_t& hw, const org_t::inst_t& inst){
          uint32_t val = evaluator.Sense(hw.GetTrait<DoorsState>(trait_names.state_trait));
          size_t reg_idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
          hw.regs[reg_idx] = val;
          if(!inst.nop_vec.empty()) hw.AdvanceIP(1);
        };
        action_map.AddFunc<void, org_t&, const org_t::inst_t&>(
            "doors-sense", func_sense);
      }
    }

  };

  MABE_REGISTER_MODULE(EvalDoors, 
      "Evaluate organisms on their ability to associate symbols to doors.");
}

#endif
