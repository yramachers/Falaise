# - Basic CMake setup
# Check version meets ou requirements
# Declare project, which will configure compiler for us
cmake_minimum_required(VERSION 3.3)
project(FalaiseTrackFitPlugin VERSION "2.1.0")

# Modules use Falaise, so we need to locate this or fail
# find_package(Falaise REQUIRED)

# Ensure our code can see the Falaise headers
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

############################################################################################
# - TrackFit library:

list(APPEND TrackFit_HEADERS
  source/TrackFit/drawing.h
  source/TrackFit/fit_utils.h
  source/TrackFit/gg_hit.h
  source/TrackFit/helix_fit_mgr.h
  source/TrackFit/i_drift_time_calibration.h
  source/TrackFit/line_fit_mgr.h
)

list(APPEND TrackFit_SOURCES
  source/TrackFit/drawing.cc
  source/TrackFit/fit_utils.cc
  source/TrackFit/gg_hit.cc
  source/TrackFit/helix_fit_mgr.cc
  source/TrackFit/i_drift_time_calibration.cc
  source/TrackFit/line_fit_mgr.cc
)

# Build a dynamic library from our sources
add_library(TrackFit SHARED ${TrackFit_HEADERS} ${TrackFit_SOURCES})

target_link_libraries(TrackFit Bayeux::Bayeux)

# Apple linker requires dynamic lookup of symbols, so we
# add link flags on this platform
if(APPLE)
  set_target_properties(TrackFit
    PROPERTIES LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

############################################################################################
# - TrackFit modules:
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/falaise)

# - Headers:
list(APPEND FalaiseTrackFitPlugin_HEADERS source/falaise/snemo/reconstruction/trackfit_driver.h)
list(APPEND FalaiseTrackFitPlugin_HEADERS source/falaise/snemo/reconstruction/trackfit_tracker_fitting_module.h)

# - Sources:
list(APPEND FalaiseTrackFitPlugin_SOURCES source/falaise/snemo/reconstruction/trackfit_driver.cc)
list(APPEND FalaiseTrackFitPlugin_SOURCES source/falaise/snemo/reconstruction/trackfit_tracker_fitting_module.cc)

############################################################################################

# Build a dynamic library from our sources
add_library(Falaise_TrackFit SHARED
  ${FalaiseTrackFitPlugin_HEADERS}
  ${FalaiseTrackFitPlugin_SOURCES})

target_link_libraries(Falaise_TrackFit TrackFit FalaiseModule)

# Apple linker requires dynamic lookup of symbols, so we
# add link flags on this platform
if(APPLE)
  set_target_properties(Falaise_TrackFit
    PROPERTIES
      LINK_FLAGS "-undefined dynamic_lookup"
      INSTALL_RPATH "@loader_path"
    )
endif()

# Install Targets
install(TARGETS TrackFit Falaise_TrackFit DESTINATION ${CMAKE_INSTALL_PLUGINDIR})


# Test support:
option(FalaiseTrackFitPlugin_ENABLE_TESTING "Build unit testing system for FalaiseTrackFitPlugin" ON)
if(FalaiseTrackFitPlugin_ENABLE_TESTING)
  enable_testing()
  add_subdirectory(testing)
endif()
