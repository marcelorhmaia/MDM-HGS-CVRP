#include "Genetic.h"
#include "commandline.h"
#include "LocalSearch.h"
#include "Split.h"
#include "InstanceCVRPLIB.h"
using namespace std;

int main(int argc, char *argv[])
{
	try
	{
		// Reading the arguments of the program
		CommandLine commandline(argc, argv);

		// Reading the data file and initializing some data structures
		if (commandline.verbose) std::cout << "----- READING INSTANCE: " << commandline.pathInstance << std::endl;
		InstanceCVRPLIB cvrp(commandline.pathInstance, commandline.isRoundingInteger);

		Params params(cvrp.x_coords,cvrp.y_coords,cvrp.dist_mtx,cvrp.service_time,cvrp.demands,
			          cvrp.vehicleCapacity,cvrp.durationLimit,commandline.nbVeh,cvrp.isDurationConstraint,commandline.verbose,commandline.ap);
		
		// Dynamic default parameter values
		if (params.ap.randGeneration < 0)
			if (params.nbClients < 200)
				params.ap.randGeneration = 0.1;
			else if (params.nbClients < 400)
				params.ap.randGeneration = 0.8;
			else
				params.ap.randGeneration = 0.2;
		if (params.ap.mdmNbElite < 0)
			if (params.nbClients < 200)
				params.ap.mdmNbElite = 5;
			else if (params.nbClients < 400)
				params.ap.mdmNbElite = 10;
			else if (params.nbClients < 1001)
				params.ap.mdmNbElite = 5;
			else
				params.ap.mdmNbElite = 0;

		// Print all algorithm parameter values
		if (commandline.verbose) print_algorithm_parameters(params.ap);

		// Running HGS
		Genetic solver(params);
		solver.run();
		
		// Exporting the best solution
		if (solver.population.getBestFound() != NULL)
		{
			if (params.verbose) std::cout << "----- WRITING BEST SOLUTION IN : " << commandline.pathSolution << std::endl;
			solver.population.exportCVRPLibFormat(*solver.population.getBestFound(),commandline.pathSolution);
			solver.population.exportSearchProgress(commandline.pathSolution + ".PG.csv", commandline.pathInstance);
		}
	}
	catch (const string& e) { std::cout << "EXCEPTION | " << e << std::endl; }
	catch (const std::exception& e) { std::cout << "EXCEPTION | " << e.what() << std::endl; }
	return 0;
}
