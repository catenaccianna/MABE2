/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2022-2022.
 *
 *  @file  EvalPatchHarvest.hpp
 *  @brief MABE Evaluation module that places organisms in an environment with one or more 
 *      nutrient patches. Organisms are rewarded for consuming these nutrients.
 *
 */

#ifndef MABE_EVAL_PATCH_HARVEST_HPP
#define MABE_EVAL_PATCH_HARVEST_HPP

#include "../../core/MABE.hpp"
#include "../../core/Module.hpp"
#include "../../orgs/VirtualCPUOrg.hpp"
#include "../../tools/StateGrid.hpp"

#include "emp/io/File.hpp"
#include "emp/bits/BitVector.hpp"

namespace mabe {

  /// \brief State of a single organism's progress on the patch harvesting task
  struct PatchHarvestState{
    bool initialized;             ///< Flag indicating if this state has been initialized
    size_t cur_map_idx;           ///< Index of the map being traversed 
    emp::BitVector visited_tiles; ///< A mask showing which tiles have been previously visited
    emp::StateGridStatus status;  ///< Stores position, direction, and interfaces with grid 
    double raw_score;             /**< Number of unique valid tiles visited minus the number
                                       of steps taken off the path (not unique) */

    PatchHarvestState(): initialized(false), cur_map_idx(0), visited_tiles(), status(),
        raw_score(0) { ; }
    
    PatchHarvestState(const PatchHarvestState&){ // Ignore copy, just prep to initialize
      raw_score = 0;
      initialized = false;
    }
    PatchHarvestState(PatchHarvestState&&){ // Ignore move, just prep to initialize
      raw_score = 0;
      initialized = false;
    }
    PatchHarvestState& operator=(PatchHarvestState&){ // Ignore copy, just prep to initialize
      raw_score = 0;
      initialized = false;
      return *this;
    }
    PatchHarvestState& operator=(PatchHarvestState&&){ // Ignore move, just prep to initialize
      raw_score = 0;
      initialized = false;
      return *this;
    }
  };

  /// \brief Information of a single path that was loaded from file
  struct MapData{
    emp::StateGrid grid;  ///< The tile data of the path and surrounding emptiness 
    size_t start_x;       ///< X coordinate of starting position
    size_t start_y;       ///< Y coordinate of starting position
    int start_facing;     /**< Facing direction for new organisms. 
                              0=UL, 1=Up, 2=UR, 3=Right, 4=DR, 5=Down, 6=DL, 
                              7=Left (+=Clockwise) Matches StateGridStatus */
    size_t total_nutrients; ///< Number of nutrients (edge or normal) on this map 

    MapData() : total_nutrients(0){;} 
    MapData(emp::StateGrid& _grid, size_t _total_nutrients) 
        : grid(_grid) , total_nutrients(_total_nutrients) { ; }
  };

  /// \brief Contains all information for multiple paths and can evaluate organisms on them
  struct PatchHarvestEvaluator{
    /// \brief A single tile in a tile map
    enum Tile{
      EMPTY=0,
      NUTRIENT,
      NUTRIENT_CONSUMED,
      NUTRIENT_EDGE
    };

    emp::vector<MapData> map_data_vec; ///< All the relevant data for each map loaded
    emp::Random& rand;          ///< Reference to the main random number generator of MABE
    double score_exp_base = 2; /**< The base of the merit exponential 
                                    (raised to the score power)*/
    
    public: 
    bool verbose = false;

    PatchHarvestEvaluator(emp::Random& _rand) : map_data_vec(), rand(_rand){ ; } 

    /// Fetch the number of maps that are currently stored 
    size_t GetNumMaps(){ return map_data_vec.size(); }

    /// Divide raw score by the length of the current path
    double GetNormalizedScore(PatchHarvestState& state) const{
      if(state.raw_score < 0) return 0;
      return state.raw_score / map_data_vec[state.cur_map_idx].total_nutrients;
    }
    double GetExponentialScore(PatchHarvestState& state) const{
      return std::pow(score_exp_base, state.raw_score);
    }

    /// Load a single map for the path following task
    template <typename... Ts>
    void LoadMap(Ts &&... args){
      // Create our MapData to be filled
      map_data_vec.emplace_back();
      MapData& map_data = *(map_data_vec.rbegin()); 
      // Set up the possible tile types for the grid (we ignore the score value)
      map_data.grid.AddState(Tile::EMPTY,       'o', 1.0, "empty");
      map_data.grid.AddState(Tile::NUTRIENT,    'N', 1.0, "nutrient");
      map_data.grid.AddState(Tile::NUTRIENT_CONSUMED, '.', 1.0, "nutrient_consumed");
      map_data.grid.AddState(Tile::NUTRIENT_EDGE,     'E', 1.0, "nutrient_edge");
      // Load data
      map_data.grid.Load(std::forward<Ts>(args)...);
      for(size_t row_idx = 0; row_idx < map_data.grid.GetHeight(); ++row_idx){
        for(size_t col_idx = 0; col_idx < map_data.grid.GetWidth(); ++col_idx){
          switch(map_data.grid.GetState(col_idx, row_idx)){
            case Tile::EMPTY:
              break;
            case Tile::NUTRIENT:
              map_data.total_nutrients++; 
              break;
            case Tile::NUTRIENT_CONSUMED:
              break;
            case Tile::NUTRIENT_EDGE:
              map_data.total_nutrients++; 
              break;
          }
        }
      }
      if(!map_data.grid.HasMetadata("start_facing")){
        emp_error("Error! Map does not have metadata \"start_facing\"!");
      }
      map_data.start_facing = 
          static_cast<int>(map_data.grid.GetMetadata("start_facing").AsDouble());
      if(!map_data.grid.HasMetadata("start_x")){
        emp_error("Error! Map does not have metadata \"start_x\"!");
      }
      map_data.start_x = 
          static_cast<int>(map_data.grid.GetMetadata("start_x").AsDouble());
      if(!map_data.grid.HasMetadata("start_y")){
        emp_error("Error! Map does not have metadata \"start_y\"!");
      }
      map_data.start_y = 
          static_cast<int>(map_data.grid.GetMetadata("start_y").AsDouble());
      std::cout << "Map #" << (map_data_vec.size() - 1) << " is " 
        << map_data.grid.GetWidth() << "x" << map_data.grid.GetHeight() << ", with " 
        << map_data.total_nutrients << " total nutrients!" << std::endl;
    }

    /// Load a semi-colon-separated list of maps from disk
    void LoadAllMaps(const std::string& map_filenames_str){
      emp::vector<std::string> map_filename_vec;
      emp::slice(map_filenames_str, map_filename_vec, ';');
      for(auto filename : map_filename_vec){
        LoadMap(filename);
      }
    }
    
    /// Initialize all properties of a PatchHarvestState to prepare it for the path follow task
    void InitializeState(PatchHarvestState& state, bool reset_map = true){
      state.initialized = true;
      if(reset_map) state.cur_map_idx = rand.GetUInt(map_data_vec.size());;
      emp_assert(map_data_vec.size() > state.cur_map_idx, "Cannot initialize state before loading the map!");
      const MapData& map_data = map_data_vec[state.cur_map_idx];
      state.visited_tiles.Resize(map_data.grid.GetSize()); 
      state.visited_tiles.Clear();
      state.status.Set(
        map_data.start_x,
        map_data.start_y,
        map_data.start_facing 
      );
      state.raw_score = 0;
    }
    
    /// Fetch the data of the state's current path
    MapData& GetCurPath(const PatchHarvestState& state){
      return map_data_vec[state.cur_map_idx];
    }
    
    /// Fetch the data of the state's current path
    const MapData& GetCurPath(const PatchHarvestState& state) const{
      return map_data_vec[state.cur_map_idx];
    }

    /// Record the organism's current position as visited
    void MarkVisited(PatchHarvestState& state){
      state.visited_tiles.Set(state.status.GetIndex(GetCurPath(state).grid), true);
    }

    /// Fetch the reward value for organism's current position
    ///
    /// On a nutrient or edge nutrient: +1
    /// On a poison: -1
    /// Else: 0
    double GetCurrentPosScore(const PatchHarvestState& state) const{
      int tile_id = state.status.Scan(GetCurPath(state).grid);
      bool has_been_visited = state.visited_tiles.Get(
          state.status.GetIndex(GetCurPath(state).grid));
      if(verbose){
        std::cout << "Current tile: " << tile_id << 
            "; visited: " << has_been_visited << std::endl;
      }
      if(!has_been_visited && (tile_id == Tile::NUTRIENT || tile_id == Tile::NUTRIENT_EDGE)){
        return 1;
      } 
      else if(tile_id == Tile::EMPTY) return -1;
      return 0; // Otherwise we've seen this tile of the path before, do nothing
    }

    /// Move the organism in the direction it is facing, then update and return score
    double Move(PatchHarvestState& state, int scale_factor = 1){
      if(!state.initialized) InitializeState(state);
      // Mark *old* tile as visited
      MarkVisited(state);
      if(verbose) std::cout << "[HARVEST] move" << std::endl;
      state.status.Move(GetCurPath(state).grid, scale_factor);
      double score = GetCurrentPosScore(state);
      state.raw_score += score;
      if(verbose) std::cout << "Score: " << state.raw_score << std::endl;
      return GetExponentialScore(state);
    }
    
    /// Rotate the organism clockwise by 45 degrees
    void RotateRight(PatchHarvestState& state){
      if(!state.initialized) InitializeState(state);
      if(verbose) std::cout << "[HARVEST] rot_right" << std::endl;
      state.status.Rotate(1);
    }

    /// Rotate the organism counterclockwise by 45 degrees
    void RotateLeft(PatchHarvestState& state){
      if(!state.initialized) InitializeState(state);
      if(verbose) std::cout << "[HARVEST] rot_left" << std::endl;
      state.status.Rotate(-1);
    }

    /// Fetch the cue value of the tile the organism is currently on
    //
    // Note: While it sounds like this should be a const method, it is possible this is the
    //  organism's first interaction with the path, so we may need to initialize it
    uint32_t Sense(PatchHarvestState& state) { 
      if(!state.initialized) InitializeState(state);
      switch(state.status.Scan(GetCurPath(state).grid)){
        case Tile::EMPTY:
          return -1; 
          break;
        case Tile::NUTRIENT:
          if(state.visited_tiles.Get(state.status.GetIndex(GetCurPath(state).grid))) return 1;
          return 3; 
          break;
        case Tile::NUTRIENT_CONSUMED:
          return 1; 
          break;
        case Tile::NUTRIENT_EDGE:
          if(state.visited_tiles.Get(state.status.GetIndex(GetCurPath(state).grid))) return 1;
          return 0; 
          break;
      }
    }
  };

  /** \brief MABE module that evaluates Avida-esque organisms on how well they can 
             navigate a nutrient-cued path*/
  class EvalPatchHarvest : public Module {
    using inst_func_t = VirtualCPUOrg::inst_func_t;

  private:
    std::string score_trait = "score";      ///< Name of trait for organism performance
    std::string state_trait ="state";       /**< Name of trait that stores the patch
                                                  harvest state **/
    std::string map_filenames="";           ///< ;-separated list map filenames to load
    std::string movement_trait="movements"; ///< Trait holding all org movements 
    std::string map_idx_trait="map_idx";    ///< Trait holding the index of the current map
    bool track_movement = true;             /**< If true, track every move or turn the 
                                                  organism performs **/
    PatchHarvestEvaluator evaluator;        /**< The evaluator that does all of the actually 
                                                  computing and bookkeeping for the patch 
                                                  harvest task */
    int pop_id = 0;              /**< ID of the population to evaluate 
                                         (and provide instructions to) */

  public:
    EvalPatchHarvest(mabe::MABE & control,
                const std::string & name="EvalPatchHarvest",
                const std::string & desc="Evaluate organisms by how well they can harvest resource patches.")
      : Module(control, name, desc)
      , evaluator(control.GetRandom())
    {
      SetEvaluateMod(true);
    }
    ~EvalPatchHarvest() { }

    /// Set up variables for configuration script
    void SetupConfig() override {
      LinkPop(pop_id, "target_pop", 
          "Population to evaluate.");
      LinkVar(score_trait, "score_trait", 
          "Which trait stores path following performance?");
      LinkVar(state_trait, "state_trait", 
          "Which trait stores organisms' path follow state?");
      LinkVar(map_filenames, "map_filenames", 
          "List of map files to load, separated by semicolons(;)");
      LinkVar(movement_trait, "movement_trait", 
          "Which trait will store a string containing the organism's sequence of moves?");
      LinkVar(map_idx_trait, "map_idx_trait", 
          "Which trait will store the index of the current map?");
      LinkVar(evaluator.verbose, "verbose", 
          "If true (1), prints extra information about the organisms actions");
      LinkVar(evaluator.score_exp_base, "score_exp_base", 
          "Base of the merit exponential. Merit = this^score.");
      LinkVar(track_movement, "track_movement", 
          "If true (1), track every move or turn the organism performs");
    }
    
    /// Set up organism traits, load maps, and provide instructions to organisms
    void SetupModule() override {
      AddSharedTrait<double>(score_trait, "Path following score", 0.0);
      AddOwnedTrait<PatchHarvestState>(state_trait, "Organism's patch harvest state", { }); 
      if(track_movement){
        AddOwnedTrait<std::string>(movement_trait, "Organism's movements", { }); 
      }
      AddOwnedTrait<size_t>(map_idx_trait, "Organism's current map (as an index)", { }); 
      evaluator.LoadAllMaps(map_filenames);
      SetupInstructions();
    }

    /// Package path following actions (e.g., move, turn) into instructions and provide 
    /// them to the organisms via ActionMap
    void SetupInstructions(){
      ActionMap& action_map = control.GetActionMap(pop_id);
      { // Move
        inst_func_t func_move = 
          [this](VirtualCPUOrg& hw, const VirtualCPUOrg::inst_t& /*inst*/){
            PatchHarvestState& state = hw.GetTrait<PatchHarvestState>(state_trait);
            double score = evaluator.Move(state);
            hw.SetTrait<double>(score_trait, score);
            if(track_movement){
              hw.SetTrait<std::string>(movement_trait, 
                  hw.GetTrait<std::string>(movement_trait) + "M");
            }
            hw.SetTrait<size_t>(map_idx_trait, state.cur_map_idx);
          };
        action_map.AddFunc<void, VirtualCPUOrg&, const VirtualCPUOrg::inst_t&>(
            "sg-move", func_move);
      }
      { // Rotate right 
        inst_func_t func_rotate_right = 
          [this](VirtualCPUOrg& hw, const VirtualCPUOrg::inst_t& /*inst*/){
            PatchHarvestState& state = hw.GetTrait<PatchHarvestState>(state_trait);
            evaluator.RotateRight(state);
            if(track_movement){
              hw.SetTrait<std::string>(movement_trait, 
                  hw.GetTrait<std::string>(movement_trait) + "R");
            }
            hw.SetTrait<size_t>(map_idx_trait, state.cur_map_idx);
          };
        action_map.AddFunc<void, VirtualCPUOrg&, const VirtualCPUOrg::inst_t&>(
            "sg-rotate-r", func_rotate_right);
      }
      { // Rotate left 
        inst_func_t func_rotate_left = 
          [this](VirtualCPUOrg& hw, const VirtualCPUOrg::inst_t& /*inst*/){
            PatchHarvestState& state = hw.GetTrait<PatchHarvestState>(state_trait);
            evaluator.RotateLeft(state);
            if(track_movement){
              hw.SetTrait<std::string>(movement_trait, 
                  hw.GetTrait<std::string>(movement_trait) + "L");
            }
            hw.SetTrait<size_t>(map_idx_trait, state.cur_map_idx);
          };
        action_map.AddFunc<void, VirtualCPUOrg&, const VirtualCPUOrg::inst_t&>(
            "sg-rotate-l", func_rotate_left);
      }
      { // Sense 
        inst_func_t func_sense = 
          [this](VirtualCPUOrg& hw, const VirtualCPUOrg::inst_t& inst){
            PatchHarvestState& state = hw.GetTrait<PatchHarvestState>(state_trait);
            uint32_t val = evaluator.Sense(state);
            size_t reg_idx = inst.nop_vec.empty() ? 1 : inst.nop_vec[0];
            hw.regs[reg_idx] = val;
            if(!inst.nop_vec.empty()) hw.AdvanceIP(1);
            hw.SetTrait<size_t>(map_idx_trait, state.cur_map_idx);
          };
        action_map.AddFunc<void, VirtualCPUOrg&, const VirtualCPUOrg::inst_t&>(
            "sg-sense", func_sense);
      }
    }
  };

  MABE_REGISTER_MODULE(EvalPatchHarvest, 
      "Evaluate organisms on their ability to harvest patches of nutrients.");
}

#endif
