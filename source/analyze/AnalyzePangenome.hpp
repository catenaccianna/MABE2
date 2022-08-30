/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2022.
 *
 *  @file  AnalyzePangenome.hpp
 *  @brief Crossover operator that tracks all existing genomes in order to randomly generate new genomes.
 * 
 *  ok so here's my problem: i would like to get more than one entry for each time step, with each edge that is in the graph.
    not sure how to get it to enter 111->000 and then 111->101 for example, at the same time step. If this can be figured out,
    then could also put in seq count for how many times 111 appears in the genome.
    if i can write my own AddFunction then I could maybe make a loop for the "os<<" but 
    could also just put all the datafile stuff in DBGraph, but then would just have one snap of that graph at one time
    current idea: create multiple ostreams to append to as we iterate throguh the keys and adjs and slap into csv
    data.Add(pangenome_graph.csv(), "Count", "number of times this sequence appears in the pangenome");
    data.Add(pangenome_graph.csv(), "Adjacencies", "graph data");
    data.Add(pangenome_graph.csv(), "From", "graph data");
    data.Add(pangenome_graph.csv(), "To", "graph data");

    OK SO go the csv without using datafile and just ofstream, but it's only for one graph at one time
    can i pass the file back and forth so i keep appending to the same one at each time
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
  /// Function of module.
    double probability; // Probability that the genome of an organism is modified with the DBGraph before it mutates
    bool count_kmers; // If true, a kmer in the DBGraph can only be used the same number of times it appears in the pangenome
    DeBruijnGraph pangenome_graph; // DeBruijn Graph storing pangenome

  /// Datafile variables
    emp::DataFile data; // The data file object
  
  /// Variables to add to datafile to describe DeBruijn graph.
    int count; // Number of times this sequence (kmer) appears in the pangenome
    string from; // ID of a kmer/sequence/node
    string to; // ID to an adjacency (represents a directed edge)

  public:
    AnalyzePangenome(mabe::MABE & control,
          const std::string & name="AnalyzePangenome",
          const std::string & desc="Module to generate a random new genetic sequence based on existing pangenome",
          double _probability=1, bool _count_kmers=1)
      : Module(control, name, desc)
      , probability(_probability), count_kmers(_count_kmers)
      , data(""), count(0), from(""), to("")
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

    void SetupModule() override {
      data = emp::DataFile("DeBruijnGraph.csv");

      std::function<size_t ()> updatefun = [this](){return control.GetUpdate();};
      data.AddFun(updatefun,"Time", "The current time step/generation");
      
      data.AddFun<int>([this]() 
      {
        tuple<int, int> tup = pangenome_graph.kmer_count(count, from, to);
        count = std::get<1>(tup);
        return std::get<0>(tup);
      }, "Count", "Number of times a kmer appears in the entire pangenome");
     // seq count runs, we have updated seq count, and old from and old to.

      data.AddFun<string>([this]()
      {
        tuple<string, string> tup = pangenome_graph.from(count, from, to);
        from = std::get<1>(tup);
        return std::get<0>(tup);
      }, "From", "A kmer node on the graph");
      // from runs, we have updated seq count and from, and old to. 

      data.AddFun<string>([this]()
      {
        tuple<string, string> tup = pangenome_graph.to(count, from, to);
        to = std::get<1>(tup);
        return std::get<0>(tup);
      }, "To", "An adjacent kmer");
      // seq count runs, we have updated seq count and old from and to.

      data.PrintHeaderKeys();
    }

    void OnUpdate(size_t update) override {
      if(update%100==0){
        tuple<int, string, string> tup = pangenome_graph.csv_start_values();
        count = std::get<0>(tup);
        from = std::get<1>(tup);
        to = std::get<2>(tup);

        for(int i = 0; i < pangenome_graph.edge_count(); ++i){
          data.Update(); //update 4 vars as many times as we need to to go through all graph vertices
        }
        pangenome_graph.reset_vertex_flags();
      }      
    }

    void BeforePlacement(Organism & bit_org, OrgPosition pos, OrgPosition parent_pos) override {
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

    void BeforeDeath(OrgPosition position) override {
      pangenome_graph.remove_sequence(position.OrgPtr()->ToString()); // remove_sequence in the graph before the organism dies
    }

  };

  MABE_REGISTER_MODULE(AnalyzePangenome, "Generate a random new genetic sequence based on existing pangenome.");
}

#endif
