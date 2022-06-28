/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2022.
 *
 *  @file AnalyzePangenome.cpp 
 *  @brief Tests the Pangenome Analysis Module on core and edge cases.
 */

// CATCH
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
// Empirical tools
#include "emp/base/vector.hpp"
#include "emp/base/array.hpp"
#include "emp/base/Ptr.hpp"
#include "emp/base/vector.hpp"
#include "emp/control/Signal.hpp"
#include "emp/math/Random.hpp"
// Emplode
#include "../../Emplode/Emplode.hpp"
// MABE
#include "../AnalyzePangenome.hpp"
#include "../../core/Collection.hpp"
#include "../../core/Module.hpp"
#include "../../core/Population.hpp"
#include "../../core/OrgIterator.hpp"
#include "../../core/OrganismManager.hpp"
#include "../../orgs/BitsOrg.hpp"
#include "../../select/SelectTournament.hpp"
#include "../../evaluate/static/EvalNK.hpp"
#include "../../core/MABE.hpp"
#include "../../core/EmptyOrganism.hpp"
#include "../../core/ModuleBase.hpp"
#include "../../core/SigListener.hpp"

// DeBruijn Graph tools
#include "../../../../../../Desktop/debruijn_pangenomes/pangenomes-for-evolutionary-computation/DeBruijn/DeBruijnGraph.h"

/*
///  AGENDA!

hook into automatic testing suite:
- pull in auto testing branch from austin - check with him about this first 
(I am on his empirical branch of virtual_cppu_merge and I believe Emily's master branch of MABE before the merge)
- tests/components -> add analyze directory, model it off the other ones and copy makefiles and replace names.
- at component level, edit makefile to add analyze to the list of names for folders to look in
- commit and maybe push to my fork before merging austins in just in case anything goes wrong


population size, mutation rate, n, k are important variables. we want a problem that will not max out the fitness at first so we can see 
if pangenomes actually help
we want a stich where fitness will plateu consistently but to know there is a higher value out there. small ns and ks so we can count them
k = 3 so we have 4 dependent bits. 2^4 sites = 16 x 100 n (bits) = 1600 possible entire combinations?

is there a way to effectively ennumerate the values we need? They're all kinda overlapping
could also reduce n enough that we can just count the sites

tournanment selection is already aftificially bad, so we can use something better when we get to 
30 replicants for each. Tournament will be good to start with though, because we know that the results can always be better

First, run small trials on my computer to get a sense of what behavior we can expect from changing the different variables
Next, 30 replicates of control runs (w/o analysis module) in hpcc
      30 experiment runs with complete random choices at each branch
      30 expiriment runs with tunable probability (P from emp::random) of selecting non-starting genome from graph
Data we want to keep track of--fitness, # of unique genomes, print full pangenome every 10ish generations so we can see how it changes (julia???)
Will use R for data analysis if possible-can also use python or python wrapped in R for the hard parts of code

If wanted later can calculate a statistical distribution that will tell us if our randomness falls within the expected distribution
I believe this is already done from emp::random generator, but can do it for my genomes

today -- make test cases pass: in DBGraph, make an endpoint check so we have an appropriate lengthed genome
      -- make traversal make more sense if we are displaying things
*/

TEST_CASE("AnalyzePangenome__member_functions", "[analyze/AnalyzePangenome.hpp]"){
{
// ======================= INITIALIZATION ===================================

    DeBruijnGraph debruijn;
    mabe::MABE control(0, NULL);
    control.SetupEmpty<mabe::EmptyOrganismManager>();
    control.AddModule<mabe::OrganismManager<mabe::BitsOrg>>("BitOrg");
    control.AddModule<mabe::SelectTournament>("fitness");
    control.AddModule<mabe::EvalNK>("bits", "fitness");
    mabe::ModuleBase & analysis_mod = control.AddModule<mabe::AnalyzePangenome>(); // get analysis module so I can manually call the functions that would otherwise be triggered automatically
    //mabe::SigListener<mabe::ModuleBase,void,mabe::Organism &> before_mutate_sig; //error that no matching call (probs needs ())
    control.Setup(); //SET UP EMPTY COMES BEFORE SET UP 

    mabe::Population & pop = control.AddPopulation("test_pop");
    REQUIRE(control.GetPopulation(0).GetName() == "test_pop");
    REQUIRE(pop.GetName() == "test_pop");

    mabe::OrgPosition position = pop.begin(); //note: *position is an organism reference
    mabe::PopIterator position_iter = pop.begin(); //note: can call position_iter.OrgPtr()->ToString() if wanted

    control.Inject(pop, "BitOrg", 400);

    size_t i;
    for(i = 0; i < pop.GetSize(); ++i){
      debruijn.add_sequence(pop[i].ToString()); // add the sequences to the tester debruijn graph
      analysis_mod.OnInjectReady(pop[i], pop);  // call OnInjectReady to add the current population to the module DBGraph
    }
    REQUIRE(i == 400);
    pop.OK();
    REQUIRE(pop.begin() != pop.end());
    REQUIRE(debruijn.get_sequence_size() == int(pop.GetSize()));

    //debruijn.display(); // was curious to see
    //note: some kmers look repeated in the graph, but tested seq.s for uniqueness in the debruijn class,
    // and the repetitions here are just caused by traversing

// ======================= BEFORE MUTATE ===================================
    /* Trigger attempts - revisit
    //BeforeMutate_IsTriggered();
    //before_mutate_sig.Trigger(); //mabebase.hpp line 62

    //control.RescanSignals(); 
    //control.UpdateSignals(); //? //see MABE.hpp Update on line 584 for how this is used*/
    // Manually get the analysis module and call Before_Mutate myself on each organism in the population
    // run through the new population and make sure the modified genomes are still valid with the old graph
    //
    // This will only work if organisms are added to the end of the population and not hashed in I think
    // is_valid only tests if the string is a possible combination based on the kmer connections in the 
    // graph--unsure if these will actually be usable genetic information segments

    std::vector<std::string> old_pop;
    //DeBruijnGraph new_graph;
    // call BeforeMutate to try to change all the genomes to a generated debruijn sequence
    for(i = 0; i < pop.GetSize(); ++i){
      old_pop.push_back(pop[i].ToString());
      analysis_mod.BeforeMutate(pop[i]);
      analysis_mod.OnMutate(pop[i]);
      //new_graph.add_sequence(pop[i]->ToString());
    }
    REQUIRE(i == 400);
    REQUIRE(int(i) == debruijn.get_sequence_size());

    // make sure all the generated sequences are still valid within the graph
    position_iter = pop.begin();
    while(position_iter != pop.end()){
      REQUIRE(debruijn.is_valid(position_iter.OrgPtr()->ToString()));
      position_iter++;
    }

    int differences = 0;                          // run through the population and see how many genomes changed
    for(size_t i = 0; i < pop.GetSize(); ++i){
      if( pop[i].ToString() != old_pop[i]) {
        differences++;
      }
    }
    REQUIRE(differences > 0);


// ======================= BEFORE DEATH - very unfinished ah ha ha ===================================

    //int seq_count = new_graph.get_value(pop.begin()->ToString()).get_sequence_count();
    analysis_mod.BeforeDeath(pop.begin());                      // removes sequence from the module graph
    control.ClearOrgAt(pop.begin());                            // this has a BeforeDeath trigger in it
    debruijn.remove_sequence(pop.begin()->ToString());          // remove sequence from the tester graph (use new_graph? this one wouldn't have the same num seq.s?)
    REQUIRE(int(pop.GetNumOrgs()) == debruijn.get_sequence_size());
    REQUIRE(pop.GetNumOrgs() == 399);                           // number of living organisms should be one less
    REQUIRE(debruijn.get_sequence_size() == 399);               // total number of seq.s in graph (incremented every time an org's sequence is added) should be one less
    

    REQUIRE(1 == 1);
  }
}

/*
TEST_CASE("AnalyzePangenome__update", "[analyze/AnalyzePangenome.hpp]"){
{
  // call Update and see what happens--see if there's anything I'm missing in my manually triggered tests

    DeBruijnGraph debruijn;
    mabe::MABE control(0, NULL);
    control.SetupEmpty<mabe::EmptyOrganismManager>();
    auto & org_manager = control.AddModule<mabe::OrganismManager<mabe::BitsOrg>>("BitOrg");
    control.AddModule<mabe::SelectTournament>("fitness");
    control.AddModule<mabe::AnalyzePangenome>();
    control.AddModule<mabe::EvalNK>("bits", "fitness");
    //mabe::SigListener<mabe::ModuleBase,void,mabe::Organism &> before_mutate_sig; //error that no matching call (probs needs ())
    //control.Setup(); //SET UP EMPTY COMES BEFORE SET UP 
    mabe::Population & pop = control.AddPopulation("current_pop");
    //mabe::Population & next_pop = control.AddPopulation("next_gen");
    int random_seed = 0;
    control.Setup();

    mabe::OrgPosition position = pop.begin(); // *position is an organism reference
    mabe::PopIterator position_iter = pop.begin(); //can call position_iter.OrgPtr()->ToString() if wanted

    control.Inject(pop, "BitOrg", 28); //INJECT POP WHOOP WHOOP
    
    for(size_t i = 0; i < pop.GetSize(); ++i){
      debruijn.add_sequence(pop[i].ToString()); // add the sequence to the tester debruijn graph
    }

    std::vector<std::string> old_pop;
    for(size_t i = 0; i < pop.GetSize(); ++i) old_pop.push_back(pop[i].ToString());


/// @attention this is where it's different from the previous test case

    std::cout<<"\nbefore:\nsize- "<<pop.GetSize()<<"\nnum living orgs- "<<pop.GetNumOrgs()<<"\n";


    control.Update(1);

    std::cout<<"\now:\nsize- "<<pop.GetSize()<<"\nnum living orgs- "<<pop.GetNumOrgs()<<"\n";


    int differences = 0;                        // run through the population and see how many genomes changed
    for(size_t i = 0; i < pop.GetSize(); ++i){
      //std::cout<<"old genome "<<old_pop[i]<<std::endl;
      //std::cout<<"new genome "<<pop[i].ToString()<<std::endl;
      if(!debruijn.is_valid(pop[i].ToString())){
        std::cout<<"MABE mutation!"<<std::endl;
      }
      if( pop[i].ToString() != old_pop[i]) {
          differences++;
      }
    }
    REQUIRE(differences > 0);

  }
}*/

/*
TEST_CASE("AnalyzePangenome__controlled", "[analyze/AnalyzePangenome.hpp]"){
{
  // test controlled versions of a bitvector in the graph (add one, add another, remove one, make sure it all stays ok)
  // putting more tests in DeBuijn so that this is more optional than before

    DeBruijnGraph debruijn;
    mabe::MABE control(0, NULL);
    control.SetupEmpty<mabe::EmptyOrganismManager>();
    auto & org_manager = control.AddModule<mabe::OrganismManager<mabe::BitsOrg>>("BitOrg");
    control.AddModule<mabe::SelectTournament>("fitness");
    control.AddModule<mabe::AnalyzePangenome>();
    control.AddModule<mabe::EvalNK>("bits", "fitness");
    control.Setup(); //SET UP EMPTY COMES BEFORE SET UP 
    mabe::Population & pop = control.AddPopulation("test_pop");
    mabe::OrgPosition position = pop.begin(); // *position is an organism reference
    mabe::PopIterator position_iter = pop.begin(); //can call position_iter.OrgPtr()->ToString() if wanted

//here do we want to inject just 1 organism?
    control.Inject(pop, "BitOrg", 28); //INJECT POP WHOOP WHOOP

    for(size_t i = 0; i < pop.GetSize(); ++i){
      debruijn.add_sequence(pop[i].ToString()); // add the sequence to the tester debruijn graph
    }



/// @attention this is where it's different from the previous test case


  }
}*/