// fldigi_delayed_alpha_geiger_cells_hit.cxx
// Standard libraries :
#include <fstream>
#include <iostream>

// - Bayeux/datatools:
#include <datatools/clhep_units.h>
#include <datatools/io_factory.h>
#include <datatools/properties.h>
#include <datatools/utils.h>

// - Bayeux/geomtools:
#include <bayeux/geomtools/manager.h>

// - Bayeux/mctools:
#include <mctools/simulated_data.h>

// - Bayeux/dpp:
#include <dpp/input_module.h>
#include <dpp/output_module.h>

// Falaise:
#include <falaise/falaise.h>

// Third part :
// Root :
#include <TH1F.h>
#include <TH2F.h>
#include "TFile.h"
#include "TTree.h"

int main(int argc_, char** argv_) {
  falaise::initialize(argc_, argv_);
  int error_code = EXIT_SUCCESS;
  datatools::logger::priority logging = datatools::logger::PRIO_FATAL;

  // Parsing arguments
  int iarg = 1;
  bool is_input_file = false;
  bool is_output_path = false;
  bool is_event_number = false;
  bool is_verbose = false;
  bool is_help = false;

  std::string input_filename;
  std::string output_path;
  int arg_event_number = -1;

  while (iarg < argc_) {
    std::string arg = argv_[iarg];

    if (arg == "-i" || arg == "--input") {
      is_input_file = true;
      input_filename = argv_[++iarg];
    }

    else if (arg == "-op" || arg == "--output-path") {
      is_output_path = true;
      output_path = argv_[++iarg];
    }

    else if (arg == "-n" || arg == "--number") {
      is_event_number = true;
      arg_event_number = atoi(argv_[++iarg]);
    }

    else if (arg == "-v" || arg == "--verbosity") {
      is_verbose = true;
    }

    else if (arg == "-h" || arg == "--help") {
      is_help = true;
    }

    iarg++;
  }

  if (is_help) {
    std::cerr << std::endl
              << "Usage :" << std::endl
              << std::endl
              << "$ BuildProducts/bin/fldigi_delayed_alpha_geiger_cells_hit [OPTIONS] [ARGUMENTS]"
              << std::endl
              << std::endl
              << "Allowed options: " << std::endl
              << "-h  [ --help ]        produce help message" << std::endl
              << "-i  [ --input ]       set an input file" << std::endl
              << "-op [ --output_path ] set the output path for producted files" << std::endl
              << "-n  [ --number ]      set the number of events" << std::endl
              << "-v  [ --verbosity]    add verbosity level" << std::endl
              << "-p  [ --print ]       print files in PDF/PNG" << std::endl
              << "Example : " << std::endl
              << std::endl
              << "$ BuildProducts/bin/fldigi_delayed_alpha_geiger_cells_hit -n 1000 -i "
                 "${FALAISE_DIGITIZATION_TESTING_DIR}/data/"
                 "Se82_0nubb-source_strips_bulk_SD_10_events.brio -op "
                 "${FALAISE_DIGITIZATION_TESTING_DIR}/output_default -v -p"
              << std::endl
              << std::endl;
    return 0;
  }

  // Process
  try {
    std::clog << "Little test program for analysis of alpha delayed number of geiger cells hit for "
                 "some physical process and vertexes generator !"
              << std::endl;

    // Manager.conf
    std::string manager_config_file;
    manager_config_file = "@falaise:config/snemo/demonstrator/geometry/3.0/manager.conf";
    datatools::fetch_path_with_env(manager_config_file);
    datatools::properties manager_config;
    datatools::properties::read_config(manager_config_file, manager_config);
    geomtools::manager my_manager;
    manager_config.update("build_mapping", true);
    if (manager_config.has_key("mapping.excluded_categories")) {
      manager_config.erase("mapping.excluded_categories");
    }
    my_manager.initialize(manager_config);

    std::string pipeline_simulated_data_filename;
    // Simulated Data "SD" bank label :
    std::string SD_bank_label = "SD";
    datatools::fetch_path_with_env(input_filename);
    if (is_input_file) {
      pipeline_simulated_data_filename = input_filename;
    } else {
      pipeline_simulated_data_filename =
          "${FALAISE_DIGITIZATION_TESTING_DIR}/data/"
          "Se82_0nubb-source_strips_bulk_SD_10_events.brio";
      input_filename = pipeline_simulated_data_filename;
    }
    datatools::fetch_path_with_env(pipeline_simulated_data_filename);

    // Number of events :
    int event_number = -1;
    if (is_event_number)
      event_number = arg_event_number;
    else
      event_number = 10;
    // Output path :
    if (is_output_path)
      output_path = output_path;
    else
      output_path = "${FALAISE_DIGITIZATION_TESTING_DIR}/output_default/";
    datatools::fetch_path_with_env(output_path);

    // Event reader :
    dpp::input_module reader;
    datatools::properties reader_config;
    reader_config.store("logging.priority", "debug");
    reader_config.store("max_record_total", event_number);
    reader_config.store("files.mode", "single");
    reader_config.store("files.single.filename", pipeline_simulated_data_filename);
    reader.initialize_standalone(reader_config);
    reader.tree_dump(std::clog, "Simulated data reader module");

    static const int LIMIT_SUP_PROMPT_TIME = 25;  // in nanosecond,
    static const int LIMIT_INF_DELAYED_TIME =
        8000;  // in nanosecond, linked with the digi trigger and geiger drift
    static const int LIMIT_SUP_DELAYED_TIME =
        1000000;  // in nanosecond, linked with the digi trigger and geiger drift

    // Event record :
    datatools::things ER;

    std::string vtx_filename = input_filename;
    std::size_t found = 0;
    found = vtx_filename.find_last_of("/");
    vtx_filename.erase(0, found);
    found = vtx_filename.find(".brio");
    vtx_filename.erase(found, vtx_filename.size());

    int psd_count = 0;  // Event counter
    std::string output_filename =
        output_path + vtx_filename + "_delayed_alpha_geiger_cells_hit.root";

    TFile* output_file = new TFile(output_filename.c_str(), "RECREATE");
    // TTree * output_tree = new TTree("DelayedAlphaCellsHit", "Delayed Alpha Geiger cells hit");

    TH1F* vertex_distribution_TH1F =
        new TH1F("Delayed alpha Geiger cells hit", "Delayed alpha GG cells hit", 15, 0, 14);
    TH1F* delayed_gg_inf_distribution_TH1F =
        new TH1F("Delayed alpha Geiger cells hit inf at 8 us",
                 "Delayed alpha GG cells hit inf at 8 us", 15, 0, 14);
    TH1F* delayed_gg_sup_distribution_TH1F =
        new TH1F("Delayed alpha Geiger cells hit sup at 1 ms",
                 "Delayed alpha GG cells hit sup at 1 ms", 15, 0, 14);

    while (!reader.is_terminated()) {
      reader.process(ER);
      // A plain `mctools::simulated_data' object is stored here :
      if (ER.has(SD_bank_label) && ER.is_a<mctools::simulated_data>(SD_bank_label)) {
        // Access to the "SD" bank with a stored `mctools::simulated_data' :
        const mctools::simulated_data& SD = ER.get<mctools::simulated_data>(SD_bank_label);

        int number_of_delayed_gg_cells_hit = 0;
        int number_of_delayed_gg_inf_8us = 0;
        int number_of_delayed_gg_sup_1ms = 0;

        if (SD.has_step_hits("gg")) {
          const size_t number_of_hits = SD.get_number_of_step_hits("gg");

          // New sd bank
          mctools::simulated_data flaged_sd = SD;

          // We have to flag the gg cells already hit before (maybe take into account the dead time
          // of a GG cell)
          for (size_t ihit = 0; ihit < number_of_hits; ihit++) {
            mctools::base_step_hit& geiger_hit = flaged_sd.grab_step_hit("gg", ihit);
            for (size_t jhit = ihit + 1; jhit < number_of_hits; jhit++) {
              mctools::base_step_hit& other_geiger_hit = flaged_sd.grab_step_hit("gg", jhit);
              if (geiger_hit.get_geom_id() == other_geiger_hit.get_geom_id()) {
                const double gg_hit_time = geiger_hit.get_time_start();
                const double other_gg_hit_time = other_geiger_hit.get_time_start();
                if (gg_hit_time > other_gg_hit_time) {
                  bool geiger_already_hit = true;
                  if (!geiger_hit.get_auxiliaries().has_flag("geiger_already_hit"))
                    geiger_hit.grab_auxiliaries().store("geiger_already_hit", geiger_already_hit);
                } else {
                  bool geiger_already_hit = true;
                  if (!other_geiger_hit.get_auxiliaries().has_flag("geiger_already_hit"))
                    other_geiger_hit.grab_auxiliaries().store("geiger_already_hit",
                                                              geiger_already_hit);
                }
              }  // end of if get_geom_id
            }    // end of jhit
          }      // end of ihit

          mctools::simulated_data::hit_handle_collection_type BSHC = flaged_sd.get_step_hits("gg");
          if (is_verbose) std::clog << "BSCH step hits # = " << BSHC.size() << std::endl;
          int count = 0;
          for (mctools::simulated_data::hit_handle_collection_type::const_iterator i = BSHC.begin();
               i != BSHC.end(); i++) {
            const mctools::base_step_hit& BSH = i->get();

            if (BSH.get_auxiliaries().has_flag("geiger_already_hit") ||
                BSH.get_auxiliaries().has_flag("other_geiger_already_hit")) {
            } else {
              // extract the corresponding geom ID:
              // const geomtools::geom_id & geiger_gid = BSH.get_geom_id();

              double time_start = BSH.get_time_start();
              geomtools::vector_3d position_start_vector = BSH.get_position_start();
              geomtools::vector_3d position_stop_vector = BSH.get_position_stop();
              geomtools::vector_3d momentum_start_vector = BSH.get_momentum_start();
              if (time_start > LIMIT_INF_DELAYED_TIME && time_start < LIMIT_SUP_DELAYED_TIME)
                number_of_delayed_gg_cells_hit++;
              if (time_start > LIMIT_SUP_PROMPT_TIME && time_start < LIMIT_INF_DELAYED_TIME)
                number_of_delayed_gg_inf_8us++;
              if (time_start > LIMIT_SUP_DELAYED_TIME) number_of_delayed_gg_sup_1ms++;

              count++;
            }
          }  // end of for
        }    // end of if has step hits "gg"
        vertex_distribution_TH1F->Fill(number_of_delayed_gg_cells_hit);
        delayed_gg_inf_distribution_TH1F->Fill(number_of_delayed_gg_inf_8us);
        delayed_gg_sup_distribution_TH1F->Fill(number_of_delayed_gg_sup_1ms);

      }  // end of ER
      ER.clear();
      psd_count++;
      if (is_verbose) std::clog << "DEBUG : psd count " << psd_count << std::endl;
    }  // end of while reader

    output_file->Write();
    output_file->Close();
    std::clog << "The end." << std::endl;
  }  // end of try

  catch (std::exception& error) {
    DT_LOG_FATAL(logging, error.what());
    error_code = EXIT_FAILURE;
  }

  catch (...) {
    DT_LOG_FATAL(logging, "Unexpected error!");
    error_code = EXIT_FAILURE;
  }

  falaise::terminate();
  return error_code;
}
