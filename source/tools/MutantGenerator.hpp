/**
 *  @note This file is part of MABE, https://github.com/mercere99/MABE2
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2019-2021.
 *
 *  @file  MutantGenerator.hpp
 *  @brief Generates one-step mutants for Avida-style genomes 
 */

#ifndef MABE_MUTANT_GENERATOR_H
#define MABE_MUTANT_GENERATOR_H

#include "../core/MABE.hpp"
#include "../core/Module.hpp"
#include "emp/tools/string_utils.hpp"

namespace mabe {

  class MutantGenerator : public Module {

  public:
    MutantGenerator(mabe::MABE & control,
        const std::string & name="MutantGenerator",
        const std::string & desc="Generates one-step mutants for Avida-style genomes")
        : Module(control, name, desc)
    { ; }
    ~MutantGenerator() {  ; }

    size_t GetNumPointMutations(const std::string& genome, size_t alphabet_size){
      return genome.size() * alphabet_size;
    }
    size_t GetNumDeletionMutations(const std::string& genome){
      return genome.size();
    }
    size_t GetNumInsertionMutations(const std::string& genome, size_t alphabet_size){
      return (genome.size() + 1) * alphabet_size;
    }
    char GetAvidaSymbol(size_t id){
      if(id < 26) return 'a' + id;
      else if (id < 52) return 'A' + (id - 26);
      return '!';
    }
    std::string GetPointMutation(const std::string& genome, size_t alphabet_size, 
        size_t mut_seed){
      size_t mut_site = mut_seed / alphabet_size;
      size_t mut_inst = mut_seed % alphabet_size;
      std::string out_str = genome;
      out_str.replace(mut_site, 1, std::string(1, GetAvidaSymbol(mut_inst)));
      return out_str;
    }
    std::string GetDeletionMutation(const std::string& genome, size_t mut_seed){
      emp_assert(mut_seed < genome.size());
      std::string out_str = genome;
      out_str.replace(mut_seed, 1, "");
      return out_str;
    }
    std::string GetInsertionMutation(const std::string& genome, size_t alphabet_size, 
        size_t mut_seed){
      size_t mut_site = mut_seed / alphabet_size;
      size_t mut_inst = mut_seed % alphabet_size;
      std::string out_str = genome;
      out_str.insert(mut_site, 1, GetAvidaSymbol(mut_inst));
      return out_str;
    }

    // Setup member functions associated with this class.
    static void InitType(emplode::TypeInfo & info) {
      info.AddMemberFunction("GET_NUM_POINT_MUTATIONS",
          [](MutantGenerator & mod, const std::string& genome, size_t alphabet_size) { 
            return mod.GetNumPointMutations(genome, alphabet_size);
          },
          "Return the total number of possible deletion mutations");
      info.AddMemberFunction("GET_NUM_DELETION_MUTATIONS",
          [](MutantGenerator & mod, const std::string& genome) { 
            return mod.GetNumDeletionMutations(genome);
          },
          "Return the total number of possible deletion mutations");
      info.AddMemberFunction("GET_NUM_INSERTION_MUTATIONS",
          [](MutantGenerator & mod, const std::string& genome, size_t alphabet_size) { 
            return mod.GetNumInsertionMutations(genome, alphabet_size);
          },
          "Return the total number of possible deletion mutations");
      info.AddMemberFunction("GET_AVIDA_SYMBOL",
          [](MutantGenerator & mod, size_t id) { 
            return mod.GetAvidaSymbol(id);
          },
          "Return the symbol (char) for the Nth instruction in the library");
      info.AddMemberFunction("GET_POINT_MUTATION",
          [](MutantGenerator & mod, const std::string& genome, size_t alphabet_size, 
              size_t seed) { 
            return mod.GetPointMutation(genome, alphabet_size, seed);
          },
          "Returns the Nth point mutation");
      info.AddMemberFunction("GET_DELETION_MUTATION",
          [](MutantGenerator & mod, const std::string& genome, size_t seed) { 
            return mod.GetDeletionMutation(genome, seed);
          },
          "Returns the Nth deletion mutation");
      info.AddMemberFunction("GET_INSERTION_MUTATION",
          [](MutantGenerator & mod, const std::string& genome, size_t alphabet_size, 
              size_t seed) { 
            return mod.GetInsertionMutation(genome, alphabet_size, seed);
          },
          "Returns the Nth insertion mutation");
    }

  };

  MABE_REGISTER_MODULE(MutantGenerator, "Generates one-step mutants for Avida-style genomes");
}

#endif
