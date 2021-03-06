#include "SbplPlanner.h"

#include <utils/PlannerUtils.hpp>
#include <utils/Stopwatch.hpp>

#define SAVE_SBPL_MAZE_IMAGE

#ifdef SAVE_SBPL_MAZE_IMAGE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <third_party/stb/stb_image_write.h>

#include <ctime>
#endif

template <sbpl::Planner PlannerT>
SbplPlanner<PlannerT>::SbplPlanner()
    : AbstractPlanner(name()),
      _solution(og::PathGeometric(global::settings.ompl.space_info)) {
  _env = new EnvironmentNAVXYTHETALAT;

  // define the robot shape
  vector<sbpl_2Dpt_t> perimeterptsV;

  if (global::settings.env.collision.collision_model == robot::ROBOT_POLYGON) {
    sbpl_2Dpt_t shape_point;
    const Polygon robot = global::settings.env.collision.robot_shape.value();
    for (const auto &point : robot.points) {
      // The points in the environments should not be scaled up
      shape_point.x = point.x / global::settings.sbpl.scaling;
      shape_point.y = point.y / global::settings.sbpl.scaling;
      perimeterptsV.push_back(shape_point);
    }
  }

  OMPL_DEBUG("Constructing %s Planner", name().c_str());

  std::cout << "SBPL motion primitive filename: "
            << global::settings.sbpl.motion_primitive_filename << std::endl;

  const auto cells_x = static_cast<int>(global::settings.environment->width() /
                                        global::settings.sbpl.resolution /
                                        global::settings.sbpl.scaling);
  const auto cells_y = static_cast<int>(global::settings.environment->height() /
                                        global::settings.sbpl.resolution /
                                        global::settings.sbpl.scaling);

  std::cout << "Environment, W: " << global::settings.environment->width()
            << ", H: " << global::settings.environment->height() << std::endl;
  std::cout << "Environment, cells_x: " << cells_x << ", cells_y: " << cells_y
            << std::endl;

  try {
    _env->InitializeEnv(
        cells_x, cells_y,
        nullptr,  // mapdata
        0, 0, 0,  // start (x, y, theta, t)
        0, 0, 0,  // goal (x, y, theta)
        // goal tolerance
        global::settings.sbpl.goal_tolerance_x,
        global::settings.sbpl.goal_tolerance_y,
        global::settings.sbpl.goal_tolerance_theta, perimeterptsV,
        global::settings.sbpl.resolution,  // cell size
        global::settings.sbpl.forward_velocity,
        global::settings.sbpl.time_to_turn_45_degs_in_place,
        20u,  // obstacle threshold
        global::settings.sbpl.motion_primitive_filename.value().c_str());
  } catch (...) {
    OMPL_ERROR("Error occurred while creating SBPL environment.");
    delete _env;
    throw;
  }

  double min_x = global::settings.environment->getBounds().low[0];
  double min_y = global::settings.environment->getBounds().low[1];

  double max_x = global::settings.environment->getBounds().high[0];
  double max_y = global::settings.environment->getBounds().high[1];

  // convert SBPL cell coordinates to environment coordinates
  auto index2env = [&min_x, &min_y](int ix, int iy, double *x, double *y) {
    *x = ix * global::settings.sbpl.resolution * global::settings.sbpl.scaling +
         min_x;
    *y = iy * global::settings.sbpl.resolution * global::settings.sbpl.scaling +
         min_y;
  };
  // convert environment coordinates to SBPL cell coordinates
  auto env2index = [&min_x, &min_y](double x, double y, int *ix,
                                                int *iy) {
    *ix = (x - min_x) / global::settings.sbpl.resolution /
          global::settings.sbpl.scaling;
    *iy = (y - min_y) / global::settings.sbpl.resolution /
          global::settings.sbpl.scaling;
  };

#ifdef SAVE_SBPL_MAZE_IMAGE
  unsigned char maze_data[cells_x * cells_y];
  // XXX flip maze image vertically to appear the same as the SVG
  stbi_flip_vertically_on_write(1);
#endif

  double env_x, env_y;
  for (int ix = 0; ix < cells_x; ++ix) {
    for (int iy = 0; iy < cells_y; ++iy) {
      index2env(ix, iy, &env_x, &env_y);
      const bool collides =
          global::settings.environment->collides(env_x, env_y);
      _env->UpdateCost(ix, iy, static_cast<unsigned char>(collides ? 20u : 1u));
#ifdef SAVE_SBPL_MAZE_IMAGE
      maze_data[iy * cells_x + ix] = collides ? 127u : 255u;
#endif
    }
  }
  std::cout << "Initialized " << cells_x << "x" << cells_y
            << " SBPL environment.\n";

#ifdef SAVE_SBPL_MAZE_IMAGE
  std::string maze_image_filename =
      "sbpl_" + std::to_string(std::time(0)) + ".bmp";
  if (!stbi_write_bmp(maze_image_filename.c_str(), cells_x, cells_y, 1,
                      maze_data)) {
    OMPL_ERROR("Failed to save SBPL maze as Bitmap file.");
  } else {
    std::cout << "Saved SBPL maze at " << maze_image_filename << std::endl;
  }
#endif

  // Initialize MDP Info
  MDPConfig MDPCfg{};
  if (!_env->InitializeMDPCfg(&MDPCfg)) {
    throw SBPL_Exception("ERROR: InitializeMDPCfg failed");
  }

  switch (PlannerT) {
    case sbpl::SBPL_ARASTAR:
      _sbPlanner = new ARAPlanner(_env, ForwardSearch);
      break;
    case sbpl::SBPL_LAZY_ARA:
      _sbPlanner = new LazyARAPlanner(_env, ForwardSearch);
      break;
    case sbpl::SBPL_ADSTAR:
      _sbPlanner = new ADPlanner(_env, ForwardSearch);
      break;
    case sbpl::SBPL_RSTAR:
      _sbPlanner = new RSTARPlanner(_env, ForwardSearch);
      break;
    case sbpl::SBPL_ANASTAR:
      _sbPlanner = new anaPlanner(_env, ForwardSearch);
      break;
    case sbpl::SBPL_MHA:
      _heuristic = new EmbeddedHeuristic(_env);
      _sbPlanner = new MHAPlanner(_env, _heuristic, nullptr, 0);
      break;
    default:
      throw SBPL_Exception("ERROR: This SBPL planner is not supported.");
  }

  OMPL_DEBUG("Initialized %s Planner", name().c_str());

  std::cout << "Environment bounds: ";
  std::cout << min_x << " " << max_x << ", " << min_y << " " << max_y
            << std::endl;

  int startTheta = 0, goalTheta = 0;
  double fraction = global::settings.sbpl.num_theta_dirs / (2. * M_PI);
  startTheta = static_cast<int>(std::round(
                   global::settings.environment->startTheta() * fraction)) %
               global::settings.sbpl.num_theta_dirs;
  goalTheta = static_cast<int>(std::round(
                  (global::settings.environment->goalTheta()) * fraction)) %
              global::settings.sbpl.num_theta_dirs;

  std::cout << "SBPL startTheta: "
            << (global::settings.environment->startTheta() * 180. / M_PI)
            << " deg   " << startTheta << std::endl;
  std::cout << "SBPL goalTheta: "
            << (global::settings.environment->goalTheta() * 180. / M_PI)
            << " deg   " << goalTheta << std::endl;

  std::cout << "Start in real world: "
            << global::settings.environment->start().x << ", "
            << global::settings.environment->start().y << std::endl;

  std::cout << "Goal in real world: " << global::settings.environment->goal().x
            << ", " << global::settings.environment->goal().y << std::endl;

  std::cout << "SBPL scaling factor: " << global::settings.sbpl.scaling
            << std::endl;

  int start_x, start_y;
  env2index(global::settings.environment->start().x,
            global::settings.environment->start().y, &start_x, &start_y);
  int goal_x, goal_y;
  env2index(global::settings.environment->goal().x,
            global::settings.environment->goal().y, &goal_x, &goal_y);

  // try{
  //   int ret = _env->SetStart(global::settings.environment->start().x + min_x,
  //   global::settings.environment->start().y + min_y,
  //   global::settings.environment->startTheta() ); if(ret < 0 ||
  //   _sbPlanner->set_start(ret) == 0){
  //     std::cout << "ERROR: failed to set start state" << std::endl;
  //     // return false;
  //   }
  // }
  // catch(SBPL_Exception *e){
  //   std::cout << "SBPL_Exception: failed to set start state" << std::endl;
  //   // return false;
  // }

  // try{
  //   int ret = _env->SetGoal(global::settings.environment->goal().x + min_x,
  //   global::settings.environment->goal().y  + min_y,
  //   global::settings.environment->goalTheta() ); if(ret < 0 ||
  //   _sbPlanner->set_goal(ret) == 0){
  //     std::cout << "ERROR: failed to set goal state" << std::endl;
  //     // return false;
  //   }
  // }
  // catch(SBPL_Exception *e){
  //     std::cout << "SBPL_Exception: failed to set goal state" << std::endl;
  //   // return false;
  // }

  _sbPlanner->set_start(_env->GetStateFromCoord(start_x, start_y, startTheta));
  _sbPlanner->set_goal(_env->GetStateFromCoord(goal_x, goal_y, goalTheta));

  std::cout << "SBPL start: " << start_x << ", " << start_y << std::endl;
  std::cout << "SBPL goal:  " << goal_x << ", " << goal_y << std::endl;

  _sbPlanner->set_search_mode(
      global::settings.sbpl.search_until_first_solution);
  _sbPlanner->set_initialsolution_eps(
      global::settings.sbpl.initial_solution_eps);
}

template <sbpl::Planner PlannerT>
SbplPlanner<PlannerT>::~SbplPlanner() {
  delete _sbPlanner;
  delete _env;
  delete _heuristic;
}

template <sbpl::Planner PlannerT>
ob::PlannerStatus SbplPlanner<PlannerT>::run() {
  _solution.clear();
  std::vector<int> stateIDs;
  Stopwatch stopwatch;
  _sbPlanner->force_planning_from_scratch_and_free_memory();
  stopwatch.start();
  if (dynamic_cast<ARAPlanner *>(_sbPlanner) != nullptr) {
    // used by ARA* planner (non-incremental)
    ((ARAPlanner *)_sbPlanner)->costs_changed();
  }
  //    else if (dynamic_cast<ADPlanner*> (_sbPlanner) != NULL) {
  //        // get the affected states
  //        environment_navxythetalat.GetPredsofChangedEdges(&changedcellsV,
  //        &preds_of_changededgesIDV);
  //        // let know the incremental planner about them
  //        //use by AD* planner (incremental)
  //        ((ADPlanner*)_sbPlanner)->update_preds_of_changededges(&preds_of_changededgesIDV);
  //        printf("%d states were affected\n",
  //        (int)preds_of_changededgesIDV.size());
  //    }
  //    _sbPlanner->InitializeSearchStateSpace();
  int result =
      _sbPlanner->replan(global::settings.max_planning_time, &stateIDs);
  OMPL_DEBUG("%s finished.", name().c_str());
  _planningTime = stopwatch.stop();
  if (result) {
    OMPL_INFORM("%s found a solution!", name().c_str());

    double min_x = global::settings.environment->getBounds().low[0];
    double min_y = global::settings.environment->getBounds().low[1];
    std::vector<sbpl_xy_theta_pt_t> xythetaPath;
    _env->ConvertStateIDPathintoXYThetaPath(&stateIDs, &xythetaPath);
    for (auto &xyt : xythetaPath) {
      if (std::abs(xyt.x) < 1e-3 && std::abs(xyt.y) < 1e-3) continue;
      double env_x, env_y;
      env_x = xyt.x / global::settings.sbpl.scaling /
                  global::settings.sbpl.resolution +
              min_x;
      env_y = xyt.y / global::settings.sbpl.scaling /
                  global::settings.sbpl.resolution +
              min_y;
      // std::cout << "Grid: " << xyt.x << " " << xyt.y << " , World: " << env_x
      //           << " " << env_y << std::endl;
      _solution.append(base::StateFromXYT(
          xyt.x * global::settings.sbpl.scaling + min_x,
          xyt.y * global::settings.sbpl.scaling + min_y, xyt.theta));
    }
  } else {
    OMPL_WARN("%s found no solution.", name().c_str());
  }
  return {result == 1, false};
}

template <sbpl::Planner PlannerT>
og::PathGeometric SbplPlanner<PlannerT>::solution() const {
  return _solution;
}

template <sbpl::Planner PlannerT>
bool SbplPlanner<PlannerT>::hasReachedGoalExactly() const {
  if (_solution.getStateCount() == 0) return false;
  const auto *last =
      _solution
          .getState(static_cast<unsigned int>(_solution.getStateCount() - 1))
          ->template as<State>();
  return last->getX() == global::settings.environment->goal().x &&
         last->getY() == global::settings.environment->goal().y;
}

template <sbpl::Planner PlannerT>
double SbplPlanner<PlannerT>::planningTime() const {
  return _planningTime;
}

// template specializations
template class SbplPlanner<sbpl::SBPL_ADSTAR>;
template class SbplPlanner<sbpl::SBPL_ARASTAR>;
template class SbplPlanner<sbpl::SBPL_RSTAR>;
template class SbplPlanner<sbpl::SBPL_ANASTAR>;
template class SbplPlanner<sbpl::SBPL_MHA>;
template class SbplPlanner<sbpl::SBPL_LAZY_ARA>;
