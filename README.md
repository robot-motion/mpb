[![CircleCI](https://circleci.com/gh/circleci/circleci-docs.svg?style=shield)](https://circleci.com/gh/robot-motion/bench-mr)
[![Doxygen](https://github.com/robot-motion/bench-mr/actions/workflows/main.yml/badge.svg)](https://github.com/robot-motion/bench-mr/actions/workflows/main.yml)

# Motion Planning Benchmark
Benchmarking motion planners for wheeled mobile robots in cluttered environments on scenarios close to real-world autonomous driving settings.

## Dependencies
* [libccd 1.4+](https://github.com/danfis/libccd/releases/tag/v1.4) (because of the `chomp` implementation used here), included as submodule and automatically built
* [OMPL 1.5](https://github.com/ompl/ompl/tree/1.5.0) - included as submodule, needs to be installed first
* [nlohmann/json](https://github.com/nlohmann/json) - not provided, needs to be installed first
* [SBPL 1.3.1](https://github.com/sbpl/sbpl/tree/1.3.1) - not provided, needs to be installed
* [Jupyter Lab](https://github.com/jupyterlab/jupyterlab) with [Python 3 kernel](https://ipython.readthedocs.io/en/latest/install/kernel_install.html#kernels-for-python-2-and-3) for plotting and evaluation (see [python/README.md](python/README.md))

The following boost libraries (version 1.58+) need to be installed:
* `boost_serialization`
* `boost_filesystem`
* `boost_system`
* `boost_program_options`

The provided CHOMP implementation requires, GLUT and other OpenGL libraries to be present, which can be installed through the `freeglut3-dev` package. PNG via `libpng-dev`, expat via `libexpat1-dev`.

Optionally, to support visual debugging, Qt5 with the `Charts` and `Svg` modules needs to be installed.

The Python front-end dependencies are defined in [`python/requirements.txt`](python/requirements.txt) which can be installed through
```
pip install -r python/requirements.txt
```

## Using Docker

1. Build the Docker image
    ```bash
    docker build -t mpb .
    ```

2. Run the image to be able to access the Jupyter Lab instance on port 8888 in your browser from where you can run and evaluate benchmarks:
    ```bash
    docker run -p 8888:8888 -it mpb
    ```
   Optionally, you can mount your local `mpb` copy to its respective folder inside the docker via
   ```bash
   docker run -p 8888:8888 -v $(pwd):/root/code/mpb -it mpb
   # use %cd% in place of $(pwd) on Windows
   ```
   Now you can edit files from outside the docker and use this container to build and run the experiments.

   You can connect multiple times to this same running docker, for example if you want to access it from multiple shell instances via
   ```bash
   docker exec -it $(docker ps -qf "ancestor=mpb") bash
   ```
   Alternatively, run the provided script `./docker_connect.sh` that executes this command.

## Build instructions
1.  Check out the submodules
    ```bash
    git submodule init && git submodule update
    ```
    
2.  Create build and log folders
    ```bash
    mkdir build
    ```

3.  Build project
    ```bash
    cd build
    cmake ..
    cmake --build . -- -j4
    ```
    If you see an error during the `cmake ..` command that Qt or one of the Qt modules could
    not be found, you can ignore this message as this dependency is optional.

## Getting started
This project contains several build targets in the `experiments/` folder.
The main application for benchmarking is the `benchmark` executable that gets built
in the `bin/` folder in the project directory.

### Running a benchmark
> ⚠ **It is recommended to run the benchmarks from the Jupyter front-end.**
> 
> Run `jupyter lab` from the project folder and navigate to the `python/` directory where you can find several notebooks that can execute experiments and allow you to plot and analyze the benchmark results.

Alternatively, you have the option to manually run benchmarks via JSON configuration files that define which planners to execute, and many other settings concerning environments, steer functions, etc.

In the `bin/` folder, start a benchmark via
```bash
./benchmark configuration.json
```
where `configuration.json` is any of the `json` files in the `benchmarks/` folder.

Optionally, if multiple CPUs are available, multiple benchmarks can be run in parallel
using [GNU Parallel](https://www.gnu.org/software/parallel/), e.g., via
```bash
parallel -k ./benchmark ::: ../benchmarks/corridor_radius_*
```
This command will execute the experiments with varying corridor sizes in parallel.
For more information, consult the GNU Parallel [tutorial](https://www.gnu.org/software/parallel/parallel_tutorial.html).


This will eventually output a line similar to
```
Info:    Saved path statistics log file <...>
```

The resulting JSON log file can be used for visualizing the planning results and plotting
the statistics. To get started, check out the Jupyter notebooks inside the `python/` folder 
where all the plotting tools are provided.

## Third-party libraries
This project uses forks from some of the following repositories:

* [Open Motion Planning Library (OMPL)](http://ompl.kavrakilab.org/)
* [Search-Based Planning Library (SBPL)](https://github.com/sbpl/sbpl/)
* [hbanzhaf/steering_functions](https://github.com/hbanzhaf/steering_functions)

Besides the above contributions, the authors thank Nathan Sturtevant's Moving AI Lab
for providing the [`2D Pathfinding "MovingAI" Datasets`](https://www.movingai.com/benchmarks/grids.html).

## Developers
* Eric Heiden (University of Southern California, Los Angeles, USA)
* Luigi Palmieri (Robert Bosch GmbH, Corporate Research, Stuttgart, Germany)
* Leonard Bruns (KTH Royal Institute of Technology, Stockholm, Sweden)
* Ziang Liu (University of Southern California, Los Angeles, USA)

## Citation
Please consider citing our corresponding [article](https://ieeexplore.ieee.org/document/9387068):

```
@article{heiden2021benchmr,
  author={Heiden, Eric and Palmieri, Luigi and Bruns, Leonard and Arras, Kai O. and Sukhatme, Gaurav S. and Koenig, Sven},
  journal={IEEE Robotics and Automation Letters}, 
  title={Bench-MR: A Motion Planning Benchmark for Wheeled Mobile Robots}, 
  year={2021},
  volume={6},
  number={3},
  pages={4536-4543},
  doi={10.1109/LRA.2021.3068913}}
```
