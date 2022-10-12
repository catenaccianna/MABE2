/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2021-2022.
 *
 *  @file  NeighborPlacement.h
 *  @brief Population is located on a toroidal grid. Births are placed in an neighboring cell.
 *
 */

#ifndef MABE_NEIGHBOR_PLACEMENT_H
#define MABE_NEIGHBOR_PLACEMENT_H

#include "../core/MABE.hpp"
#include "../core/Module.hpp"

namespace mabe {

  /// Grows population to a given size, then randomly places additional births over existing orgs
  class NeighborPlacement : public Module {
  private:
    Collection target_collect;   ///< Collection of populations to manage
    size_t grid_width = 60;           ///< Width of the grid the population lives on 
    size_t grid_height = 60;          ///< Height of the grid the population lives on 
    bool use_moore_neighborhood = false; ///< If true, use Moore neighborhood, else use von Neumann

  public:
    NeighborPlacement(mabe::MABE & control,
        const std::string & name="NeighborPlacement",
        const std::string & desc="Orgs can only interact with neighboring cells in a grid")
      : Module(control, name, desc), target_collect(control.GetPopulation(0))
    {
      SetPlacementMod(true);
    }
    ~NeighborPlacement() { }

    /// Set up variables for configuration file
    void SetupConfig() override {
      LinkCollection(target_collect, "target", "Population(s) to manage.");
      LinkVar(grid_width, "grid_width", "Width of the grid the population lives on");
      LinkVar(grid_height, "grid_height", "Height of the grid the population lives on");
      LinkVar(use_moore_neighborhood, "use_moore_neighborhood", 
          "If true, use a Moore neighborhood. If false, use a von Neumann neighborhood");
    }

    /// Set birth and inject functions for the specified populations
    void SetupModule() override {
      for(size_t pop_id = 0; pop_id < control.GetNumPopulations(); ++pop_id){
        Population& pop = control.GetPopulation(pop_id);
        if(target_collect.HasPopulation(pop)){
          pop.SetPlaceBirthFun( 
            [this, &pop](Organism & /*org*/, OrgPosition ppos) {
              return PlaceBirth(ppos, pop);
            }
          );
          pop.SetPlaceInjectFun( 
            [this, &pop](Organism & /*org*/){
              return PlaceInject(pop);
            }
          );
        }
      }
    }

    /// Place a birth. Most be located next to parent
    OrgPosition PlaceBirth(OrgPosition ppos, Population & target_pop) {
      if (target_collect.HasPopulation(target_pop)) { // If population is monitored...
        const size_t parent_idx = ppos.Pos();
        const size_t parent_x = parent_idx % grid_width;
        const size_t parent_y = parent_idx / grid_width;
        size_t offspring_x = parent_x;
        size_t offspring_y = parent_y;
        // Moore neighborhood: all 8 sides
        // 7 0 1
        // 6 X 2
        // 5 4 3
        if(use_moore_neighborhood){
          const size_t direction = control.GetRandom().GetUInt(8);
          switch(direction){
            case 0:
              offspring_y = (parent_y <= 0 ? grid_height - 1 : parent_y - 1);
              break;
             case 1:
              offspring_y = (parent_y <= 0 ? grid_height - 1 : parent_y - 1);
              offspring_x = (parent_x >= (grid_width - 1) ? 0 : parent_x + 1);
              break;
             case 2:
              offspring_x = (parent_x >= (grid_width - 1) ? 0 : parent_x + 1);
              break;
             case 3:
              offspring_y = (parent_y >= (grid_height - 1) ? 0 : parent_y + 1);
              offspring_x = (parent_x >= (grid_width - 1) ? 0 : parent_x + 1);
              break;
             case 4:
              offspring_y = (parent_y >= (grid_height - 1) ? 0 : parent_y + 1);
              break;
             case 5:
              offspring_y = (parent_y >= (grid_height - 1) ? 0 : parent_y + 1);
              offspring_x = (parent_x <= 0 ? grid_width - 1 : parent_x - 1);
              break;
             case 6:
              offspring_x = (parent_x <= 0 ? grid_width - 1 : parent_x - 1);
              break;
             case 7:
              offspring_y = (parent_y <= 0 ? grid_height - 1 : parent_y - 1);
              offspring_x = (parent_x <= 0 ? grid_width - 1 : parent_x - 1);
              break;
          }
        }
        // von Neumann neighborhood: 4 cardinal directions 
        //   0 
        // 3 X 1
        //   2  
        else{
          const size_t direction = control.GetRandom().GetUInt(4);
          switch(direction){
            case 0:
              offspring_y = (parent_y <= 0 ? grid_height - 1 : parent_y - 1);
              break;
            case 1:
              offspring_x = (parent_x >= (grid_width - 1) ? 0 : parent_x + 1);
              break;
            case 2:
              offspring_y = (parent_y >= (grid_height - 1) ? 0 : parent_y + 1);
              break;
            case 3:
              offspring_x = (parent_x <= 0 ? grid_width - 1 : parent_x - 1);
              break;
          }
        }
        const size_t offspring_idx = offspring_y * grid_width + offspring_x; 
        if(offspring_idx >= target_pop.GetSize()){
          for(size_t i = target_pop.GetSize(); i < offspring_idx; ++i){
            auto pop_iterator = control.PushEmpty(target_pop);
            //pop_iterator->SetPopulation(target_pop);
          }
          return control.PushEmpty(target_pop);
        }
        else return OrgPosition(target_pop, offspring_idx);
      }

      // Otherwise, don't find a legal place!
      return OrgPosition();      
    }

    /// TODO: make sure inject doesn't hit an existing org
    /// Manually inject an organism. Pick a random position and add empty positions as needed 
    OrgPosition PlaceInject(Population & target_pop) {
      if (target_collect.HasPopulation(target_pop)) { // If population is monitored...
        if(target_pop.GetSize() == 0) return control.PushEmpty(target_pop);
        // Grab a random position in the grid
        size_t pos = control.GetRandom().GetUInt(grid_width * grid_height);
        if(pos >= target_pop.GetSize()){
          for(size_t i = target_collect.GetSize(); i < pos; ++i){
            auto pop_iterator = control.PushEmpty(target_pop);
            pop_iterator->SetPopulation(target_pop);
          }
          return control.PushEmpty(target_pop);
        }
        else return OrgPosition(target_pop, pos);
      }
      // Otherwise, don't find a legal place!
      return OrgPosition();      
    }
    
    double PrintGrid(Collection& list){
      for(size_t row_idx = 0; row_idx < grid_height; ++row_idx){
        for(size_t col_idx = 0; col_idx < grid_width; ++col_idx){
          size_t idx = row_idx * grid_width + col_idx;
          if(idx >= list.GetSize()){
            std::cout << ".";
          } 
          else{
            Organism& org = list[idx];
            if(org.IsEmpty()) std::cout << "o";
            else std::cout << "X";
          }
        }
        std::cout << std::endl;
      }
      return 0;
    }

    // Setup member functions associated with this class.
    static void InitType(emplode::TypeInfo & info) {
      info.AddMemberFunction("PRINT",
                              [](NeighborPlacement & mod, Collection list) { 
                                  return mod.PrintGrid(list); 
                              },
                             "Print empty vs non-empty organisms as a grid");
    }

  };

  MABE_REGISTER_MODULE(NeighborPlacement, "Offspring are placed next to parent on toroidal grid");
}

#endif
