
# MDM-HGS-CVRP: An Improved Hybrid Genetic Search with Data Mining for the CVRP

MDM-HGS-CVRP is a modified version of [HGS-CVRP](https://github.com/vidalt/HGS-CVRP), an implementation of the Hybrid Genetic Search (HGS) specialized to the Capacitated Vehicle Routing Problem (CVRP) by Thibaut Vidal [2].

This version incorporates a new solution generation method into the (re-)initialization process, based on the Multi Data Mining (MDM) approach, which applies patterns extracted from good solutions using data mining, and on a randomized version of the Clarke and Wright savings heuristic. It ranked 2nd in the CVRP track of the [12th DIMACS Implementation Challenge](http://dimacs.rutgers.edu/programs/challenge/vrp/) and is described in [1].

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

## Compiling the program

1. Enter the Program directory: `cd Program`
2. Create a directory named "include": `mkdir include`
3. Copy the required FPmax* LIB header files into the "include" directory.
4. Copy the FPmax* LIB shared library (.so on Linux or .dll on Windows) into the Program directory.
5. Compile the program with `make`.

## Running the algorithm

* Enter the Program directory: `cd Program`
* Run the make command: `make test`
* Try another example: `./mdmhgs ../Instances/CVRP/X-n157-k13.vrp 1 30`

```
Usage:
  ./mdmhgs instancePath distanceType timeLimit [seed]
Arguments:
  instancePath  Path to the input instance file.
  distanceType  0 for "not rounded", 1 for "rounded", or 2 for "explicit".
  timeLimit     Sets a time limit in seconds (wall clock time).
  seed          Sets a fixed seed. Defaults to 0.
```

## License

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](http://badges.mit-license.org)

- **[MIT license](http://opensource.org/licenses/mit-license.php)**
- Copyright(c) 2020 Thibaut Vidal, 2022 Marcelo Maia




