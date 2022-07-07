/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2022.
 *
 *  @file  AnalyzePangenome.hpp
 *  @brief Crossover operator that tracks all existing genomes in order to randomly generate new genomes.
 */

#ifndef MABE_ANALYZE_PANGENOME_HPP
#define MABE_ANALYZE_PANGENOME_HPP

#include "../core/MABE.hpp"
#include "../core/Module.hpp"
#include "../core/data_collect.hpp"

#include "../../../../../Desktop/debruijn_pangenomes/pangenomes-for-evolutionary-computation/DeBruijn/DeBruijnGraph.h"

namespace mabe {

  class AnalyzePangenome : public Module {
  private:
    //std::shared_ptr<DeBruijnGraph> pangenome_graph;
    DeBruijnGraph pangenome_graph;
    std::string unique_genotypes;

  public:
    AnalyzePangenome(mabe::MABE & control,
          const std::string & name="AnalyzePangenome",
          const std::string & desc="Module to generate a random new genetic sequence based on existing pangenome",
          const std::string & _unique="unique_genotypes")
      : Module(control, name, desc)
      , unique_genotypes(_unique)
    {
      SetAnalyzeMod(true);
      //pangenome_graph = std::make_shared<DeBruijnGraph>();
    }
    ~AnalyzePangenome() {  }

    // Setup member functions associated with this class.
    static void InitType(emplode::TypeInfo & info) {    }

    void SetupConfig() override {
      LinkVar(unique_genotypes, "unique_genotypes", "Number of unique genotypes in the population");
    }

    // data_collect.hpp Unique function for unique genotypes in population?!
    // maybe make a member function for this? unsure because if I put it in an On or Before function,
    // would it go through the whole population to calculate the unique number like 1000 times? n^2 then
    // so maybe make my own member function like Evaluate in EvalNK.hpp

    void SetupModule() override {
      AddOwnedTrait<int>(unique_genotypes, "unique genotypes in population", 0);
    }

    void OnInjectReady(Organism & bit_org, Population & pop) override {
      pangenome_graph.add_sequence(bit_org.ToString());
      //bit_org.SetTrait<int>(unique_genotypes);
    }

    void BeforeMutate(Organism & bit_org) override {
      // modify the organism & do the actual crossover
      //when we want to do the probability test, all we need to change is to put a number in for the third parameter 
      //string new_genome = pangenome_graph.modify_org(control.GetRandom(), bit_org.ToString(), 0.01);
      string new_genome = pangenome_graph.modify_org(control.GetRandom(), bit_org.ToString());
      bit_org.GenomeFromString(new_genome); //pass in organism reference and change its genome
    }
    
   void OnMutate(Organism & bit_org) override {
      pangenome_graph.add_sequence(bit_org.ToString()); // add_sequence of the organism to the graph
   }

   void BeforeDeath(OrgPosition position) override {
      pangenome_graph.remove_sequence(position.OrgPtr()->ToString()); // remove_sequence in the graph before the organism dies
    }

  };

  MABE_REGISTER_MODULE(AnalyzePangenome, "Generate a random new genetic sequence based on existing pangenome.");
}

#endif
