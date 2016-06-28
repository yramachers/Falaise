//! \file    flsimulatemain.cc
//! \brief   Main program for flsimulate command line application
//! \details Configure, setup and run the Geant4 based simulation
//!          of the SuperNEMO experiment. Configuration is performed
//!          in two steps, a default and then user command line input.
//!          Error handling is through exceptions, except at the top
//!          level where standard error codes are used.
//! \todo    Improve error handling, with more logical exceptions.
//!          Exceptions are good, the problem is to condense them from
//!          the various places they can come from into a system
//!          understandable by the user when errors are reported.
//!          In other words, we don't want the user to see a detailed
//!          low level report, rather a clean summary (e.g. "config
//!          failed"), plus any further detail needed for them to report
//!          the issue. This is also why we try to stop exceptions
//!          propgating into main() so that these reports can be generated.
// Copyright (c) 2013 by Ben Morgan <bmorgan.warwick@gmail.com>
// Copyright (c) 2013 by The University of Warwick
//
// This file is part of Falaise.
//
// Falaise is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Falaise is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Falaise.  If not, see <http://www.gnu.org/licenses/>.

// Standard Library
#include <string>
#include <vector>

// Third Party
// - Boost
#include "boost/filesystem.hpp"
#include "boost/program_options.hpp"
#include "boost/scoped_ptr.hpp"
namespace bpo = boost::program_options;

// - Bayeux
#include "bayeux/version.h"
#include "bayeux/bayeux.h"
#include "bayeux/datatools/multi_properties.h"
#include "bayeux/datatools/things.h"
#include "bayeux/datatools/configuration/variant_service.h"
#include "bayeux/dpp/output_module.h"
#include "bayeux/geomtools/manager.h"
#include "bayeux/mctools/g4/simulation_module.h"
#include "bayeux/mctools/g4/manager_parameters.h"
#include "bayeux/mygsl/random_utils.h"
#include "bayeux/mygsl/seed_manager.h"
namespace dtc = datatools::configuration;

// This Project
#include "falaise/version.h"
#include "falaise/falaise.h"
#include "falaise/exitcodes.h"
#include "falaise/resource.h"
#include "FLSimulateResources.h"

//----------------------------------------------------------------------
// IMPLEMENTATION DETAILS
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//! Exceptions for dialog
class FLDialogHelpRequested : public std::exception {};
class FLDialogOptionsError : public std::exception {};

//! Handle printing of version information to given ostream
void do_version(std::ostream& os, bool isVerbose) {
  os << "flsimulate " << falaise::version::get_version() << "\n";
  if (isVerbose) {
    os << "\n"
       << "Copyright (C) 2013-2016 SuperNEMO Collaboration\n\n"
       << "flsimulate uses the following external libraries:\n"
       << "* Falaise : " << falaise::version::get_version() << "\n"
       << "* Bayeux  : " << bayeux::version::get_version() << "\n"
       << "* Boost   : " << BOOST_VERSION << "\n"
       << "* Geant4  : " << "9.6.4" << "\n"
       << "\n\n";
  }
  return;
}

//! Handle printing of help message to screen
void do_help(const bpo::options_description& od) {
  do_version(std::cout, false);
  std::cout << "Usage:\n"
            << "  flsimulate [options]\n"
    // << "Options\n"
            << od
            << "\n";
  return;
}

//! Collect all needed configuration parameters in one data structure
struct FLSimulateArgs {
  // Application specific parameters:
  datatools::logger::priority     logLevel;                //!< Logging priority threshold
  unsigned int                    numberOfEvents;          //!< Number of events to be processed in the pipeline
  unsigned int                    moduloEvents;            //!< Number of events progress modulo
  std::string                     experimentID;            //!< The label of the virtual experimental setup
  std::string                     setupGeometryVersion;    //!< The version number of the virtual geometry setup
  std::string                     setupSimulationVersion;  //!< The version number of the simulation engine setup
  mctools::g4::manager_parameters simulationManagerParams; /** Parameters for the Geant4 simulation manager
                                                            *  embedded in the simulation module
                                                            */
  std::string                     outputFile;              //!< Output data file for the output module
  // Variants support:
  dtc::variant_service::config    variants;                //!< Variants configuration
};

//! Handle command line argument dialog
void do_cldialog(int argc, char *argv[], FLSimulateArgs& params) {

  // Bind command line parser to exposed parameters
  namespace bpo = boost::program_options;

  // Application specific options:
  bpo::options_description optDesc("Options");
  optDesc.add_options()
    ("help,h","print this help message")
    ("version","print version number")
    ("verbose,v","increase verbosity of logging")
    ("number,n",
     bpo::value<uint32_t>(&params.numberOfEvents)
     ->default_value(1)
     ->value_name("[events]"),
     "number of events to simulate")
    ("modulo,m",
     bpo::value<uint32_t>(&params.simulationManagerParams.number_of_events_modulo)
     ->default_value(0)
     ->value_name("[period]"),
     "progress modulo on number of events")
    ("experiment",
     bpo::value<std::string>(&params.experimentID)
     ->default_value("default")
     ->value_name("[name]"),
     "experiment to simulate")
    ("vertex-generator,x",
     bpo::value<std::string>(&params.simulationManagerParams.vg_name)
     ->default_value("source_pads_bulk")
     ->value_name("[name]"),
     "the name of the vertex generator")
    ("event-generator,e",
     bpo::value<std::string>(&params.simulationManagerParams.eg_name)
     ->default_value("Se82.0nubb")
     ->value_name("[name]"),
     "the name of the event generator")
    ("input-seeds,s",
     bpo::value<std::string>(&params.simulationManagerParams.input_prng_seeds_file)
     ->default_value("")
     ->value_name("[file]"),
     "file from which to load PRNGs' seeds")
    ("output-profiles,p",
     bpo::value<std::string>(&params.simulationManagerParams.output_profiles_activation_rule)
     ->default_value("")
     ->value_name("[rule]"),
     "the output profiles activation rule (setup the truth hits' level of details)")
    ("output-file,o",
     bpo::value<std::string>(&params.outputFile)->required()->value_name("[file]"),
     "file in which to store simulation results")
    ;

  // Variant service options:
  bpo::options_description optVariants("Variants support");
  uint32_t variant_service_flags = 0;
  variant_service_flags |= dtc::variant_service::NO_CONFIG_FILENAME;
  variant_service_flags |= dtc::variant_service::NO_TUI;
  dtc::variant_service::init_options(optVariants,
                                     params.variants,
                                     variant_service_flags);

  // Public options:
  bpo::options_description optPublic;
  optPublic.add(optDesc).add(optVariants);

  // Hidden options:
  bpo::options_description optHidden;
  optHidden.add_options()
    ("trace", "activate trace logging")
    ;

  // All options:
  bpo::options_description optAll;
  optAll.add(optPublic).add(optHidden);

  // - Parse...
  bpo::variables_map vMap;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, optAll), vMap);
    bpo::notify(vMap);
  } catch (const bpo::required_option& e) {
    // We need to handle help/version even if required_option thrown
    if (!vMap.count("help") && !vMap.count("version")) {
      std::cerr << "[OptionsException] " << e.what() << std::endl;
      throw FLDialogOptionsError();
    }
  } catch (const std::exception& e) {
    std::cerr << "[OptionsException] " << e.what() << std::endl;
    throw FLDialogOptionsError();
  }

  // Handle verbose, which can't be bound yet
  if (vMap.count("verbose")) {
    params.logLevel = datatools::logger::PRIO_INFORMATION;
    params.simulationManagerParams.logging = "information";
  }

  // Handle trace logging (for debugging)
  if (vMap.count("trace")) {
    params.logLevel = datatools::logger::PRIO_TRACE;
    params.simulationManagerParams.logging = "trace";
  }

  // Handle the experiment
  std::string experiment_label = vMap["experiment"].as<std::string>();
  try {
    params.variants.config_filename = FLSimulate::getVariantsConfigFile(experiment_label);
    if (params.variants.profile_load == "__default__") {
      params.variants.profile_load = FLSimulate::getVariantsDefaultProfile(experiment_label);
      if (datatools::logger::is_information(params.logLevel)) {
        std::clog << "[information]: " << "Loading default variant profile '"
                  << params.variants.profile_load << "'..." << std::endl;
      }
    }
    params.simulationManagerParams.manager_config_filename = FLSimulate::getControlFile(experiment_label);
  } catch (FLSimulate::UnknownResourceException& e) {
    std::cerr << "[FLSimulate::UnknownResourceException] "
              << e.what()
              << std::endl;
    throw FLDialogOptionsError();
  }


  if (params.logLevel == datatools::logger::PRIO_INFORMATION) {
    params.simulationManagerParams.tree_dump(std::clog, "", "[information]: ");
  }

  // Handle any non-bound options
  if (vMap.count("help")) {
    do_help(optPublic);
    throw FLDialogHelpRequested();
  }

  if (vMap.count("version")) {
    do_version(std::cout, true);
    throw FLDialogHelpRequested();
  }
  return;
}

//----------------------------------------------------------------------
//! Exceptions for configuration
class FLConfigDefaultError : public std::exception {};
class FLConfigHelpHandled : public std::exception {};
class FLConfigUserError : public std::exception {};

//! Parse command line arguments to configure the simulation parameters
void do_configure(int argc, char *argv[], FLSimulateArgs& params) {
  // - Default Config
  try {
    params.logLevel = datatools::logger::PRIO_ERROR;
    params.experimentID = "demonstrator";
    params.setupGeometryVersion = "4.0";
    params.setupSimulationVersion = "2.0";
    params.simulationManagerParams.set_defaults();
    params.simulationManagerParams.logging = "error";
    params.simulationManagerParams.manager_config_filename = FLSimulate::getControlFile("default");
    params.simulationManagerParams.vg_name = "source_pads_bulk";
    params.simulationManagerParams.eg_name = "Se82.0nubb";
    params.simulationManagerParams.input_prng_seeds_file = "";
    // Explicitely set the seeds of the 4 embedded PRNGs to
    // 'mygsl::random_utils::SEED_INVALID' (-1). This leads to
    // an exception at PRNG startup if nothing is done explicitely at configuration step:
    params.simulationManagerParams.vg_seed   = mygsl::random_utils::SEED_INVALID; // PRNG for the vertex generator
    params.simulationManagerParams.eg_seed   = mygsl::random_utils::SEED_INVALID; // PRNG for the primary event generator
    params.simulationManagerParams.shpf_seed = mygsl::random_utils::SEED_INVALID; // PRNG for the back end true hit processors
    params.simulationManagerParams.mgr_seed  = mygsl::random_utils::SEED_INVALID; // PRNG for the Geant4 engine itself
    params.simulationManagerParams.output_profiles_activation_rule = "";
  } catch (std::exception& e) {
    throw FLConfigDefaultError();
  }

  // - CL Dialog Config
  try {
    do_cldialog(argc, argv, params);
  } catch (FLDialogHelpRequested& e) {
    throw FLConfigHelpHandled();
  } catch (FLDialogOptionsError& e) {
    throw FLConfigUserError();
  }
  return;
}

//----------------------------------------------------------------------
//! Perform simulation using command line args as given
falaise::exit_code do_flsimulate(int argc, char *argv[])
{
  // - Configure
  FLSimulateArgs flSimParameters;
  try {
    do_configure(argc, argv, flSimParameters);
  } catch (FLConfigDefaultError& e) {
    std::cerr << "Unable to configure core of flsimulate" << std::endl;
    return falaise::EXIT_UNAVAILABLE;
  } catch (FLConfigHelpHandled& e) {
    return falaise::EXIT_OK;
  } catch (FLConfigUserError& e) {
    return falaise::EXIT_USAGE;
  }

  // Variants support:
  dtc::variant_service vserv;
  try {
    if (flSimParameters.variants.is_active()) {
      vserv.configure(flSimParameters.variants);
      // Start and lock the variant service:
      vserv.start();
      // From this point, all other services and/or processing modules can
      // benefit of the variant service during their configuration steps.
    }
  } catch (std::exception & e) {
    std::cerr << "flsimulate : Variant service threw exception" << std::endl;
    std::cerr << e.what() << std::endl;
    return falaise::EXIT_UNAVAILABLE;
  }

  // - Run
  falaise::exit_code code = falaise::EXIT_OK;
  try {

    if (flSimParameters.simulationManagerParams.input_prng_seeds_file.empty()) {
      // If no input file is given with explicit PRNG seeding inside, the
      // seeds are explicitely set to the 'mygsl::random_utils::SEED_AUTO'
      // value for an automated randomization  of seeds  from a  possible source  of entropy.
      // This mechanism works well for a single run. However, it is not recommended when one plans
      // to launch several statistical independant runs. In such case on should enforce
      // the user to explicitely set the seeds for each simulation run.
      flSimParameters.simulationManagerParams.vg_seed   = mygsl::random_utils::SEED_AUTO; // PRNG for the vertex generator
      flSimParameters.simulationManagerParams.eg_seed   = mygsl::random_utils::SEED_AUTO; // PRNG for the primary event generator
      flSimParameters.simulationManagerParams.shpf_seed = mygsl::random_utils::SEED_AUTO; // PRNG for the back end true hit processors
      flSimParameters.simulationManagerParams.mgr_seed  = mygsl::random_utils::SEED_AUTO; // PRNG for the Geant4 engine itself
    }

    // Analyse the simulation manager configuration:
    datatools::multi_properties flSimProperties("name", "");
    flSimProperties.read(flSimParameters.simulationManagerParams.manager_config_filename);

    // Output profiles:
    /*
    // Fetch the list of supported output profiles:
    if (flSimProperties.get_section("manager").has_key("output_profiles")) {
    datatools::properties flSimOutputProfilesProperties;
    std::vector<std::string> supported_output_profiles;
    flSimProperties.get_section("manager").fetch("output_profiles", supported_output_profiles);
    std::clog << "Supported output profiles : " << std::endl << std::endl;
    for (size_t i(0); i < supported_output_profiles.size(); i++) {
    std::ostringstream sop_desc_key;
    sop_desc_key << "output_profiles." << supported_output_profiles[i] << ".description";
    std::string sop_desc = "Not documented";
    if (flSimProperties.get_section("manager").has_key(sop_desc_key.str())) {
    sop_desc = flSimProperties.get_section("manager").fetch_string(sop_desc_key.str());
    }
    std::clog << "  - " << supported_output_profiles[i] << " : " << sop_desc << std::endl;
    }
    std::clog << std::endl;
    }
    */

    // Have to setup geometry:
    // datatools::properties flSimGeoManagerProperties;
    std::string geoManagerFile = flSimProperties.get_section("geometry").fetch_path("manager.config");
    geomtools::manager geoManager;
    datatools::properties geoManagerProperties;
    datatools::properties::read_config(geoManagerFile, geoManagerProperties);
    geoManager.initialize(geoManagerProperties);

    // Simulation module:
    mctools::g4::simulation_module flSimModule;
    flSimModule.set_name("G4SimulationModule");
    flSimModule.set_sd_label("SD");
    flSimModule.set_geometry_manager(geoManager);
    flSimModule.set_simulation_manager_params(flSimParameters.simulationManagerParams);
    flSimModule.initialize_simple();

    // Output module:
    dpp::output_module simOutput;
    simOutput.set_name("FLSimulateOutput");
    simOutput.set_single_output_file(flSimParameters.outputFile);
    // Metadata management:
    datatools::multi_properties & metadataStore = simOutput.grab_metadata_store();
    metadataStore = flSimProperties;
    simOutput.initialize_simple();

    // Manual Event loop....
    datatools::things workItem;
    dpp::base_module::process_status status;

    for (unsigned int i(0); i < flSimParameters.numberOfEvents; ++i) {
      workItem.clear();

      status = flSimModule.process(workItem);
      if (status != dpp::base_module::PROCESS_OK) {
        std::cerr << "flsimulate : Simulation module failed" << std::endl;
        code = falaise::EXIT_UNAVAILABLE;
      }

      status = simOutput.process(workItem);
      if (status != dpp::base_module::PROCESS_OK) {
        std::cerr << "flsimulate : Output module failed" << std::endl;
        code = falaise::EXIT_UNAVAILABLE;
      }

      if (code != falaise::EXIT_OK) {
        break;
      }
    }
  } catch (std::exception& e) {
    std::cerr << "flsimulate : Setup/run of simulation threw exception" << std::endl;
    std::cerr << e.what() << std::endl;
    code = falaise::EXIT_UNAVAILABLE;
  }

  if (vserv.is_started()) {
    // Terminate the variant service:
    vserv.stop();
  }

  return code;
}


//----------------------------------------------------------------------
// MAIN PROGRAM
//----------------------------------------------------------------------
int main(int argc, char *argv[]) {
  // - Needed, but nasty
  falaise::initialize();

  // - Do the simulation.
  // Ideally, exceptions SHOULD NOT propagate out of this  - the error
  // code should be enough.
  falaise::exit_code ret = do_flsimulate(argc, argv);

  // - Needed, but nasty
  falaise::terminate();
  return ret;
}
