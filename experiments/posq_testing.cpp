#include "base/PlannerSettings.h"
#include "base/environments/GridMaze.h"
#include "planners/OMPLPlanner.hpp"
#include "planners/sbpl/SbplPlanner.h"
#include "planners/thetastar/ThetaStar.h"
#include "utils/PathEvaluation.hpp"

namespace og = ompl::geometric;

int main(int argc, char **argv) {
  Log::instantiateRun();

  for (unsigned int i = 0; i < 4; ++i) {
    global::settings.environment =
        GridMaze::createRandomCorridor(50, 50, 4, 30, i + 1);

    global::settings.max_planning_time = 3;
    global::settings.steer.steering_type = Steering::STEER_TYPE_POSQ;
    global::settings.steer.initializeSteering();

    auto info = nlohmann::json({{"plans", {}}});
    global::settings.environment->to_json(info["environment"]);

    //    PathEvaluation::evaluate<ThetaStar>(info);
    PathEvaluation::evaluate<RRTPlanner>(info);
    PathEvaluation::evaluate<RRTstarPlanner>(info);
    PathEvaluation::evaluate<RRTsharpPlanner>(info);
    PathEvaluation::evaluate<InformedRRTstarPlanner>(info);
    //    PathEvaluation::evaluate<SORRTstarPlanner>(info);
    //    PathEvaluation::evaluate<CForestPlanner>(info);
    //    PathEvaluation::evaluate<SbplPlanner>(info);
    Log::log(info);
  }

  Log::save("posq_testing.json");

  return EXIT_SUCCESS;
}
