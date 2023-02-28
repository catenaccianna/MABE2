/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2019-2021.
 *
 *  @file  EvalGeneric.hpp
 *  @brief Calls the organisms' GenerateOutput
 */

#ifndef MABE_EVAL_GENERIC_H
#define MABE_EVAL_GENERIC_H

#include "../../core/MABE.hpp"
#include "../../core/Module.hpp"
#include "../../tools/NK.hpp"

#include "emp/datastructs/reference_vector.hpp"

namespace mabe {

  class EvalGeneric : public Module {
  public:
    EvalGeneric(mabe::MABE & control,
           const std::string & name="EvalGeneric",
           const std::string & desc="Calls the organisms' GenerateOutput")
      : Module(control, name, desc)
    {
      SetEvaluateMod(true);
    }
    ~EvalGeneric() { }

    // Setup member functions associated with this class.
    static void InitType(emplode::TypeInfo & info) {
      info.AddMemberFunction("EVAL",
                              [](EvalGeneric & mod, Collection list) { 
                                return mod.Evaluate(list); 
                              },
                             "Calls the organisms' GenerateOutput");
    }

    void SetupConfig() override {
    }

    void SetupModule() override {
    }

    double Evaluate(const Collection & orgs) {
      mabe::Collection alive_orgs( orgs.GetAlive() );
      for (Organism & org : alive_orgs) {
        org.GenerateOutput();
      }
      return alive_orgs.GetSize();
    }

    // If a population is provided to Evaluate, first convert it to a Collection.
    double Evaluate(Population & pop) { return Evaluate( Collection(pop) ); }

    // If a string is provided to Evaluate, convert it to a Collection.
    double Evaluate(const std::string & in) { return Evaluate( control.ToCollection(in) ); }
  };

  MABE_REGISTER_MODULE(EvalGeneric, "Calls the organisms' GenerateOutput");
}

#endif
