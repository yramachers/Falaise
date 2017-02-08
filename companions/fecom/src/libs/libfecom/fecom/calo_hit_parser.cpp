// fecom/calo_hit_parser.cpp

// Ourselves:
#include <fecom/calo_hit_parser.hpp>

// Third party:
// - Boost:
#include <boost/spirit/include/qi.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/phoenix/phoenix.hpp>
// #include <boost/fusion/include/at_c.hpp>
#include <boost/algorithm/string.hpp>
// - Bayeux:
#include <bayeux/datatools/exception.h>
#include <bayeux/datatools/utils.h>

namespace fecom {

  // static
  const std::size_t calo_hit_parser::NB_CALO_HEADER_LINES;

  calo_hit_parser::calo_hit_parser()
  {
    return;
  }

  bool calo_hit_parser::parse(std::istream & in_, calo_hit & hit_)
  {
    DT_LOG_TRACE_ENTERING(logging);
    bool success = false;
    try {
      // Header:
      for (std::size_t ih = 0; ih < NB_CALO_HEADER_LINES; ih++) {
        std::string hline;
        std::getline(in_, hline);
        DT_LOG_DEBUG(logging, "Calo hit parsing header line number " << ih << " : '" << hline << "'");
        _parse_header_(hline, ih, hit_);
        in_ >> std::ws;
      }
      // Data:
      std::string raw_waveform_data_line;
      std::getline(in_, raw_waveform_data_line);
      DT_LOG_DEBUG(logging, "Parsing raw waveform data line '" << raw_waveform_data_line << "'");
      _parse_samples_(raw_waveform_data_line, hit_);
      in_ >> std::ws;
      success = true;
    } catch (std::exception & error) {
      DT_LOG_ERROR(logging, error.what());
      success = false;
    }
    DT_LOG_TRACE_EXITING(logging);
    return success;
  }

  void calo_hit_parser::_parse_header_(const std::string & header_line_,
                                       const int index_,
                                       calo_hit & hit_)
  {
    DT_LOG_TRACE_ENTERING(logging);
    namespace qi = boost::spirit::qi;
    bool res = false;
    std::clog << "INFO : INDEX = " << index_ << " header line = " << header_line_ << std::endl;

    // Already done in hit_parser::parse()

    if (index_ == 0) {
      std::string header_line = header_line_;
      // We use a trick because of nasty syntax from the DAQ ascii output:
      boost::replace_all(header_line, "Slot", "Slot ");
      boost::replace_all(header_line, " Ch", " Ch ");
      DT_LOG_DEBUG(logging, "header_line = '" << header_line << "'");
      std::string::const_iterator str_iter = header_line.begin();
      std::string::const_iterator end_iter = header_line.end();
      uint32_t slotid    = 0xFFFFFFFF;
      uint32_t channelid = 16;
      uint32_t eventid   = 0xFFFFFFFF;
      uint64_t rawtdc    = 0xFFFFFFFFFFFFFFFF; // 64bits ? to be checked
      uint32_t trigcount;
      uint32_t timecount;
      int32_t  rawbaseline;
      int32_t  rawpeak;
      int32_t  rawcharge;
      uint32_t overflow;
      uint32_t risingcell = 1024;
      uint32_t risingoffset;
      uint32_t fallingcell = 1024;
      uint32_t fallingoffset;
      uint32_t fcr = 1024;
      res = qi::phrase_parse(str_iter,
                             end_iter,
                             //  Begin grammar
                             (
                              qi::lit("Slot")          >> qi::uint_[boost::phoenix::ref(slotid) = boost::spirit::qi::_1]
                              >> qi::lit("Ch")            >> qi::uint_[boost::phoenix::ref(channelid) = boost::spirit::qi::_1]
                              >> qi::lit("EvtID")         >> qi::uint_[boost::phoenix::ref(eventid) = boost::spirit::qi::_1]
                              >> qi::lit("RawTDC")        >> qi::ulong_[boost::phoenix::ref(rawtdc) = boost::spirit::qi::_1]
                              >> qi::lit("TrigCount")     >> qi::uint_[boost::phoenix::ref(trigcount) = boost::spirit::qi::_1]
                              >> qi::lit("Timecount")     >> qi::uint_[boost::phoenix::ref(timecount) = boost::spirit::qi::_1]
                              >> qi::lit("RawBaseline")   >> qi::int_[boost::phoenix::ref(rawbaseline) = boost::spirit::qi::_1]
                              >> qi::lit("RawPeak")       >> qi::int_[boost::phoenix::ref(rawpeak) = boost::spirit::qi::_1]
                              >> qi::lit("RawCharge")     >> qi::int_[boost::phoenix::ref(rawcharge) = boost::spirit::qi::_1]
                              >> qi::lit("Overflow")      >> qi::uint_[boost::phoenix::ref(overflow) = boost::spirit::qi::_1]
                              >> qi::lit("RisingCell")    >> qi::uint_[boost::phoenix::ref(risingcell) = boost::spirit::qi::_1]
                              >> qi::lit("RisingOffset")  >> qi::uint_[boost::phoenix::ref(risingoffset) = boost::spirit::qi::_1]
                              >> qi::lit("FallingCell")   >> qi::uint_[boost::phoenix::ref(fallingcell) = boost::spirit::qi::_1]
                              >> qi::lit("FallingOffset") >> qi::uint_[boost::phoenix::ref(fallingoffset) = boost::spirit::qi::_1]
                              >> qi::lit("FCR")           >> qi::uint_[boost::phoenix::ref(fcr) = boost::spirit::qi::_1]
                             ),
                             //  End grammar
                             qi::space);
      DT_THROW_IF(!res || str_iter != end_iter,
                  std::logic_error,
                  "Cannot parse file header line #" << index_ << "; failed at '" << *str_iter << "'!");
      DT_LOG_DEBUG(logging, "slotid        = " << slotid);
      DT_LOG_DEBUG(logging, "channelid     = " << channelid);
      DT_LOG_DEBUG(logging, "eventid       = " << eventid);
      DT_LOG_DEBUG(logging, "rawtdc        = " << rawtdc);
      DT_LOG_DEBUG(logging, "trigcount     = " << trigcount);
      DT_LOG_DEBUG(logging, "timecount     = " << timecount);
      DT_LOG_DEBUG(logging, "rawbaseline   = " << rawbaseline);
      DT_LOG_DEBUG(logging, "rawpeak       = " << rawpeak);
      DT_LOG_DEBUG(logging, "rawcharge     = " << rawcharge);
      DT_LOG_DEBUG(logging, "overflow      = " << overflow);
      DT_LOG_DEBUG(logging, "risingcell    = " << risingcell);
      DT_LOG_DEBUG(logging, "risingoffset  = " << risingoffset);
      DT_LOG_DEBUG(logging, "fallingcell   = " << fallingcell);
      DT_LOG_DEBUG(logging, "fallingoffset = " << fallingoffset);
      DT_LOG_DEBUG(logging, "fcr           = " << fcr);
      hit_.slot_index = slotid;
      hit_.channel = channelid;
      hit_.event_id = eventid;
      hit_.raw_tdc = rawtdc;
      hit_.low_threshold_trig_count = trigcount;
      hit_.low_threshold_time_count = timecount;
      hit_.fcr = fcr;
      hit_.raw_baseline = rawbaseline;
      hit_.raw_peak = rawpeak;
      hit_.raw_charge = rawcharge;
      hit_.raw_charge_overflow = (overflow == 0 ? false: true);
      // Waveform size is not saved in the header !!!
      // hit_.waveform_data_size = 1024;
    }

    DT_LOG_TRACE_EXITING(logging);
    return;
  }

  void calo_hit_parser::_parse_samples_(const std::string & data_line_,
                                        calo_hit & hit_)
  {
    DT_LOG_TRACE_ENTERING(logging);
    hit_._reset_raw_waveform_data_();
    namespace qi = boost::spirit::qi;

    std::string::const_iterator str_iter = data_line_.begin();
    std::string::const_iterator end_iter = data_line_.end();
    std::vector<int> raw_waveform_data;
    //     std::size_t waveform_data_size = 0; // unknown from the header !!! so we let Spirit guess it...
    bool res = false;
    res = qi::phrase_parse(str_iter,
                           end_iter,
                           //  Begin grammar
                           (
                            // qi::repeat(hit_.waveform_data_size)[qi::int_]
                            +qi::int_
                            ),
                           //  End grammar
                           qi::space,
                           raw_waveform_data);
     DT_THROW_IF(!res || str_iter != end_iter,
                  std::logic_error,
                 "Cannot parse event samples!");
     // DT_THROW_IF(raw_waveform_data.size() != hit_.waveform_data_size,
     //             std::logic_error, "Invalid waveform size!");
     // Waveform:
     hit_.waveform_data_size = raw_waveform_data.size();
     for (uint16_t icell = 0; icell < raw_waveform_data.size(); icell++) {
       hit_.set_raw_sample(icell, (int16_t) raw_waveform_data[icell]);
     }
     DT_LOG_TRACE_EXITING(logging);
     return;
  }

} // namespace fecom
