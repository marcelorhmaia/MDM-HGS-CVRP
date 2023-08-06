# MDM-HGS-CVRP: An Improved Hybrid Genetic Search with Data Mining for the CVRP

MDM-HGS-CVRP is a modified version of [HGS-CVRP](https://github.com/vidalt/HGS-CVRP), an implementation of the Hybrid Genetic Search (HGS) specialized to the Capacitated Vehicle Routing Problem (CVRP) by Thibaut Vidal [2].

It incorporates a new solution generation method into the (re-)initialization process, based on the Multi Data Mining (MDM) approach, which applies patterns extracted from good solutions using data mining, and on a randomized version of the Clarke and Wright savings heuristic.

MDM-HGS-CVRP ranked 2nd in the CVRP track of the [12th DIMACS Implementation Challenge](http://dimacs.rutgers.edu/programs/challenge/vrp/) and is described in [1].
The code used in the Challenge (based on HGS-CVRP v1.0.0) is in the [dimacs](https://github.com/marcelorhmaia/MDM-HGS-CVRP/tree/dimacs) branch.

The main branch is based on HGS-CVRP v2.0.0.

## References

When using this algorithm (or part of it) in derived academic studies, please refer to the following papers:

[1] Maia, M. R. H., Plastino, A., Souza, U. S. (2022). 
An Improved Hybrid Genetic Search with Data Mining for the CVRP. In: 12th DIMACS Implementation Challenge: Vehicle Routing Problems. 
(Available [HERE](http://dimacs.rutgers.edu/events/details?eID=2073)).

[2] Vidal, T. (2022). Hybrid genetic search for the CVRP: Open-source implementation and SWAP* neighborhood. Computers & Operations Research, 140, 105643.
https://doi.org/10.1016/j.cor.2021.105643 (Available [HERE](https://arxiv.org/abs/2012.10384) in technical report form).

## Dependencies

This program uses [FPmax* LIB](https://github.com/marcelorhmaia/FPmax-LIB).

You should download and build it before compiling the program (see instructions on the link above).

After building FPmax* LIB, copy the required files into the project directory:
1. Create a directory named "external" with two subdirectories named "include" and "lib":
```console
mkdir -p external/include external/lib
```
2. Copy the required FPmax* LIB header files into the "external/include" directory.
3. Copy the FPmax* LIB shared library (.so on Linux or .dll on Windows) into the "external/lib" directory.

## Compiling the executable 

You need [`CMake`](https://cmake.org) to compile.

Build with:
```console
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make bin
```
This will generate the executable file `hgs` in the `build` directory.

Test with:
```console
ctest -R bin --verbose
```

## Running the algorithm

After building the executable, try an example: 
```console
./hgs ../Instances/CVRP/X-n157-k13.vrp mySolution.sol -seed 1 -t 30
```

The following options are supported:
```
Call with: ./hgs instancePath solPath [-it nbIter] [-t myCPUtime] [-bks bksPath] [-seed mySeed] [-veh nbVehicles] [-log verbose]
[-it <int>] sets a maximum number of iterations without improvement. Defaults to 20,000                                     
[-t <double>] sets a time limit in seconds. If this parameter is set, the code will be run iteratively until the time limit           
[-seed <int>] sets a fixed seed. Defaults to 0                                                                                    
[-veh <int>] sets a prescribed fleet size. Otherwise a reasonable UB on the fleet size is calculated                      
[-round <bool>] rounding the distance to the nearest integer or not. It can be 0 (not rounding) or 1 (rounding). Defaults to 1. 
[-log <bool>] sets the verbose level of the algorithm log. It can be 0 or 1. Defaults to 1.                                       

Additional Arguments:
[-nbGranular <int>] Granular search parameter, limits the number of moves in the RI local search. Defaults to 20               
[-mu <int>] Minimum population size. Defaults to 25                                                                            
[-lambda <int>] Number of solutions created before reaching the maximum population size (i.e., generation size). Defaults to 40
[-nbElite <int>] Number of elite individuals. Defaults to 5                                                                    
[-nbClose <int>] Number of closest solutions/individuals considered when calculating diversity contribution. Defaults to 4     
[-targetFeasible <double>] target ratio of feasible individuals in the last 100 generatied individuals. Defaults to 0.2  
[-randGen <double>] Ratio of randomly generated individuals (complemented using RCW). Dynamic default based on instance size
[-mdmNbElite <int>] Number of individuals in the MDM elite set. Dynamic default based on instance size 
[-mdmNbPatterns <int>] Number of (largest) patterns mined from the MDM elite set. Defaults to 5 
[-mdmNURestarts <double>] Maximum percentage of restarts without updating the MDM elite set. Defaults to 0.05
[-mdmMinSup <double>] Minimum support of patterns mined from the MDM elite set. Defaults to 0.8      
```

There exist different conventions regarding distance calculations in the academic literature.
The default code behavior is to apply integer rounding, as it should be done on the X instances of Uchoa et al. (2017).
To change this behavior (e.g., when testing on the CMT or Golden instances), give a flag `-round 0`, when you run the executable.

## Code structure

The main classes containing the logic of the algorithm are the following:
* **Params**: Stores the main data structures for the method
* **Individual**: Represents an individual solution in the genetic algorithm, also provides I/O functions to read and write individual solutions in CVRPLib format.
* **Population**: Stores the solutions of the genetic algorithm into two different groups according to their feasibility. Also includes the functions in charge of diversity management.
* **Genetic**: Contains the main procedures of the genetic algorithm as well as the crossover
* **LocalSearch**: Includes the local search functions, including the SWAP* neighborhood
* **Split**: Algorithms designed to decode solutions represented as giant tours into complete CVRP solutions
* **CircleSector**: Small code used to represent and manage arc sectors (to efficiently restrict the SWAP* neighborhood)

In addition, additional classes have been created to facilitate interfacing:
* **AlgorithmParameters**: Stores the parameters of the algorithm
* **CVRPLIB** Contains the instance data and functions designed to read input data as text files according to the CVRPLIB conventions
* **commandline**: Reads the line of command
* **main**: Main code to start the algorithm
* **C_Interface**: Provides a C interface for the method

## Compiling the shared library

You can also build a shared library to call the MDM-HGS-CVRP algorithm from your code.

```console
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make lib
```
This will generate the library file, `libhgscvrp.so` (Linux), `libhgscvrp.dylib` (macOS), or `hgscvrp.dll` (Windows),
in the `build` directory.

To test calling the shared library from a C code:
```console
make lib_test_c
ctest -R lib --verbose
```

## License

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](http://badges.mit-license.org)

- **[MIT license](http://opensource.org/licenses/mit-license.php)**
- Copyright(c) 2020 Thibaut Vidal ([HGS-CVRP](https://github.com/vidalt/HGS-CVRP))
- Copyright(c) 2022 Marcelo Maia
