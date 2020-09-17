#pragma once

#include <ompl/geometric/planners/rrt/RRTstar.h>

#include "PlannerSettings.h"
#include "planners/OMPLControlPlanner.hpp"
#include "planners/OMPLPlanner.hpp"

namespace ob = ompl::base;
namespace og = ompl::geometric;

class PlannerConfigurator {
 public:
  PlannerConfigurator() = delete;

  template <typename PLANNER>
  static void configure(PLANNER &planner) {}

  template <typename OMPL_PLANNER>
  static void configure(OMPLPlanner<OMPL_PLANNER> &planner) {
    configure(*planner.omplPlanner(),
              global::settings.ompl.geometric_planner_settings.value());
  }

  template <typename OMPL_PLANNER>
  static void configure(OMPLControlPlanner<OMPL_PLANNER> &planner) {
    configure(*planner.omplPlanner(),
              global::settings.ompl.control_planner_settings.value());
  }

 private:
  static void configure(ob::Planner &planner, const nlohmann::json &settings) {
    const auto name = planner.getName();
    auto params = planner.params();
    if (settings.find(name) != settings.end()) {
      for (auto it = settings[name].begin(); it!= settings[name].end(); ++it) {
        params.setParam(it.key(), it.value());
      }
    }
  }
};
