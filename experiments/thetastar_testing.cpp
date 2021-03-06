#include <base/environments/PolygonMaze.h>

#include <planners/OMPLPlanner.hpp>

#include "base/PlannerSettings.h"
#include "base/environments/GridMaze.h"
#include "planners/thetastar/ThetaStar.h"
#include "utils/PathEvaluation.hpp"

namespace og = ompl::geometric;

int main(int argc, char **argv) {
  global::settings.max_planning_time = 60;
  int successes = 0;
  int total = 1;
  for (int i = 0; i < total; ++i) {
    //    global::settings.environment =
    //        GridMaze::createRandomCorridor(50, 50, 6, 30, i + 1);
    //    std::string maze_filename = "polygon_mazes/parking1.svg";
    //      std::string maze_filename = "polygon_mazes/parking2.svg";
    //      std::string maze_filename = "polygon_mazes/parking3.svg";
    std::string maze_filename = "polygon_mazes/warehouse.svg";
    double scaling = 1.;
    global::settings.env.polygon.scaling =
        scaling / 22.;  // XXX important divide by 22 !!!
    auto maze = PolygonMaze::loadFromSvg(maze_filename);

    //    maze->setStart({0.0 * scaling, -2.27 * scaling});
    //    maze->setGoal({7.72 * scaling, -7.72 * scaling});
    //    maze->setThetas(0, -M_PI_2);

    //      maze->setStart({0.* scaling, -2.27* scaling});
    //      maze->setGoal({10.91 * scaling,  2.73 * scaling});
    //      maze->setThetas(0, M_PI_2);

    //      maze->setStart({15.45 * scaling, -2.27 * scaling});
    //      maze->setGoal({3.82 * scaling, -0.34 * scaling});
    //      maze->setThetas(M_PI, M_PI);

    maze->setStart({-2.27 * scaling, 4.55 * scaling});
    maze->setGoal({63.64 * scaling, -55 * scaling});
    maze->setThetas(-M_PI_2, 0);

    global::settings.environment = maze;

    global::settings.steer.steering_type = Steering::STEER_TYPE_REEDS_SHEPP;
    global::settings.steer.initializeSteering();

    global::settings.env.collision.robot_shape_source = "polygon_mazes/car.svg";
    global::settings.env.collision.collision_model = robot::ROBOT_POLYGON;
    //      global::settings.env.polygon.scaling =  1.1 / 22.;
    global::settings.env.collision.initializeCollisionModel();

    std::cout << "Start valid? " << std::boolalpha
              << maze->checkValidity(maze->start().toState(maze->startTheta()))
              << std::endl;
    std::cout << "Goal valid?  " << std::boolalpha
              << maze->checkValidity(maze->goal().toState(maze->goalTheta()))
              << std::endl;

    if (i == 0) Log::instantiateRun();

    auto info = nlohmann::json({{"plans", {}}});
    global::settings.environment->to_json(info["environment"]);

    successes += PathEvaluation::evaluateSmoothers<ThetaStar>(info);
    //      successes +=
    //      PathEvaluation::evaluateSmoothers<InformedRRTstarPlanner>(info);
    Log::log(info);
  }

  std::cout << "Theta* found a solution " << successes << " out of " << total
            << " times.\n";

  Log::save("thetastar_testing.json");

  return EXIT_SUCCESS;
}
