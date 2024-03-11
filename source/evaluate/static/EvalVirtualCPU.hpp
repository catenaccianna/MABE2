/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2019-2021.
 *
 *  @file  EvalNK.hpp
 *  @brief MABE Evaluation module for NK Landscapes
 */

#ifndef MABE_EVAL_VIRTUALCPU_H
#define MABE_EVAL_VIRTUALCPU_H

#include "../../core/MABE.hpp"
#include "../../core/Module.hpp"

#include "emp/datastructs/reference_vector.hpp"

namespace mabe {

  class EvalVirtualCPU : public Module {
  public:
    EvalVirtualCPU(mabe::MABE & control,
           const std::string & name="EvalVirtualCPU",
           const std::string & desc="Module to evaluate EvalVirtual CPUs"
           )
      : Module(control, name, desc)
    {
      SetEvaluateMod(true);
    }
    ~EvalVirtualCPU() { }

    // Setup member functions associated with this class.
    static void InitType(emplode::TypeInfo & info) {
      info.AddMemberFunction("EVAL",
                             [](EvalVirtualCPU & mod, Collection list) { return mod.Evaluate(list); },
                             "Use NK landscape to evaluate all orgs in an OrgList.");
    }


    double Evaluate(const Collection & orgs) {
      // Loop through the population and evaluate each organism.
      double max_fitness = 0.0;
      emp::Ptr<Organism> max_org = nullptr;
      mabe::Collection alive_orgs( orgs.GetAlive() );
      for (Organism & org : alive_orgs) {
        org.GenerateOutput();
        // const auto & bits = org.GetTrait<emp::BitVector>(bits_trait);
        // if (bits.size() != N) {
        //   emp::notify::Error("Org returns ", bits.size(), " bits, but ",
        //                      N, " bits needed for NK landscape.",
        //                      "\nOrg: ", org.ToString());
        // }
        // double fitness = landscape.GetFitness(bits);
        // org.SetTrait<double>(fitness_trait, fitness);

        // if (fitness > max_fitness || !max_org) {
        //   max_fitness = fitness;
        //   max_org = &org;
        // }
      }

      return max_fitness;
    }

    // If a population is provided to Evaluate, first convert it to a Collection.
    double Evaluate(Population & pop) { return Evaluate( Collection(pop) ); }

    // If a string is provided to Evaluate, convert it to a Collection.
    double Evaluate(const std::string & in) { return Evaluate( control.ToCollection(in) ); }
  };

  MABE_REGISTER_MODULE(EvalVirtualCPU, "Evaluate Virtual CPUs.");
}

#endif
