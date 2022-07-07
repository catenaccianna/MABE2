/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2022.
 *
 *  @file  AnalyzePangenome.hpp
 *  @brief Crossover operator that tracks all existing genomes in order to randomly generate new genomes.
 * 
 * Things we are changing in expirimentation: P probability in modify_org, whether we have sequence count or no sequence count. 
 * Make these a double and a bool that you can put into the contructor. A member variable/trait that must be specified by the user of the module.
 * Will probably have to change tests after this is done.
 */

#ifndef MABE_ANALYZE_PANGENOME_HPP
#define MABE_ANALYZE_PANGENOME_HPP

#include "../core/MABE.hpp"
#include "../core/Module.hpp"
#include "../core/data_collect.hpp"

#include "../../../../../Desktop/debruijn_pangenomes/pangenomes-for-evolutionary-computation/DeBruijn/DeBruijnGraph.hpp"

namespace mabe {

  class AnalyzePangenome : public Module {
  private:
    double probability; // Probability that the genome of an organism is modified with the DBGraph before it mutates.
    bool count_kmers; // If true, a kmer in the DBGraph can only be used the same number of times it appears in the pangenome.
    DeBruijnGraph pangenome_graph;

  public:
    AnalyzePangenome(mabe::MABE & control,
          const std::string & name="AnalyzePangenome",
          const std::string & desc="Module to generate a random new genetic sequence based on existing pangenome",
          double _probability=1, bool _count_kmers=1)
      : Module(control, name, desc)
      , probability(_probability), count_kmers(_count_kmers)
    {
      SetAnalyzeMod(true);
    }
    ~AnalyzePangenome() {  }

    // Setup member functions associated with this class.
    static void InitType(emplode::TypeInfo & info) {    }

    void SetupConfig() override {
      LinkVar(probability, "probability", "Probability that the genome of an organism is modified with the DBGraph before it mutates.");
      LinkVar(count_kmers, "count_kmers", "If true, a kmer in the DBGraph can only be used the same number of times it appears in the pangenome.");
    }

    void SetupModule() override {    }

    void OnInjectReady(Organism & bit_org, Population & pop) override {
      pangenome_graph.add_sequence(bit_org.ToString());
    }

    void BeforeMutate(Organism & bit_org) override {
      // modify the organism & do the actual crossover
      string new_genome;
      if(count_kmers){
        new_genome = pangenome_graph.modify_org(control.GetRandom(), bit_org.ToString(), probability);
      }
      else{
        new_genome = pangenome_graph.modify_org_NSC(control.GetRandom(), bit_org.ToString(), probability);
      }
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
