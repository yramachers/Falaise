// snemo/digitization/coincidence_trigger_algorithm.h
// Author(s): Yves LEMIERE <lemiere@lpccaen.in2p3.fr>
// Author(s): Guillaume OLIVIERO <goliviero@lpccaen.in2p3.fr>

#ifndef FALAISE_DIGITIZATION_PLUGIN_SNEMO_DIGITIZATION_COINCIDENCE_TRIGGER_ALGORITHM_H
#define FALAISE_DIGITIZATION_PLUGIN_SNEMO_DIGITIZATION_COINCIDENCE_TRIGGER_ALGORITHM_H

// Standard library :
#include <string>
#include <bitset>

// This project :
#include <snemo/digitization/calo_ctw_data.h>
#include <snemo/digitization/geiger_ctw_data.h>
#include <snemo/digitization/calo_trigger_algorithm.h>
#include <snemo/digitization/tracker_trigger_algorithm.h>
#include <snemo/digitization/electronic_mapping.h>
#include <snemo/digitization/mapping.h>

namespace datatools {
  class properties;
}

namespace snemo {
  
  namespace digitization {
		
		class coincidence_trigger_algorithm
		{
		
		public :
			
			struct coincidence_calo_record
			{
				coincidence_calo_record();
				void fetch_active_zones();
				uint32_t clocktick_1600ns;
				std::bitset<calo_trigger_algorithm::ZONING_PER_SIDE_BITSET_SIZE> zoning_word[mapping::NUMBER_OF_SIDES];
				std::bitset<calo::ctw::HTM_BITSET_SIZE> total_multiplicity_side_0;
				std::bitset<calo::ctw::HTM_BITSET_SIZE> total_multiplicity_side_1;
				bool LTO_side_0;
				bool LTO_side_1;
				std::bitset<calo::ctw::HTM_BITSET_SIZE> total_multiplicity_gveto;
				bool LTO_gveto;
				std::bitset<calo_trigger_algorithm::XT_INFO_BITSET_SIZE> xt_info_bitset;
				bool single_side_coinc;
				bool total_multiplicity_threshold;
				bool calo_finale_decision;
			};
		
			/// Default constructor
			coincidence_trigger_algorithm();

			/// Destructor
			virtual ~coincidence_trigger_algorithm();

			/// Set the electronic mapping object
			void set_electronic_mapping(const electronic_mapping & my_electronic_mapping_);

			/// Check if calorimeter gate size is set
			bool has_calorimeter_gate_size() const;

			/// Set calorimeter gate size
			void set_calorimeter_gate_size(unsigned int calorimeter_gate_size_);

			/// Initializing
			void initialize_simple();

			/// Initializing
			void initialize(const datatools::properties & config_);

			/// Check if the algorithm is initialized 
			bool is_initialized() const;

			/// Reset the object
			void reset();
			
			/// Reset internal datas
			void reset_data();

			/// General process
			void process(const std::vector<calo_trigger_algorithm::calo_summary_record> & calo_records_,
									 const std::vector<tracker_trigger_algorithm::tracker_record> & tracker_records_);
			
		protected :
			
			/// Preparing coincidence calo records structure
			void _preparing_calo_coincidence(const std::vector<calo_trigger_algorithm::calo_summary_record> & calo_records_);
			/// Compute clocktick 1600ns for calo records
			void _compute_clocktick_1600ns(const uint32_t clocktick_25ns_, uint32_t & clocktick_1600ns_);
						
			/// Process calo records and tracker records
			void _process(const std::vector<calo_trigger_algorithm::calo_summary_record> & calo_records_,
										const std::vector<tracker_trigger_algorithm::tracker_record> & tracker_records_);
			
		private :

      // Configuration :
      bool _initialized_; //!< Initialization flag
      const electronic_mapping * _electronic_mapping_; //!< Convert geometric ID into electronic ID
			unsigned int _calorimeter_gate_size_;
			std::vector<coincidence_trigger_algorithm::coincidence_calo_record> _coincidence_calo_records_; //!< Vector of coincidence calo records 
			
		};
			
	} // end of namespace digitization
	
} // end of namespace snemo

#endif // FALAISE_DIGITIZATION_PLUGIN_SNEMO_DIGITIZATION_COINCDIDENCE_TRIGGER_ALGORITHM_H

/* 
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/