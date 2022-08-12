#include "Genetic.h"
#include "commandline.h"
#include "LocalSearch.h"
#include "Split.h"
using namespace std;

int main(int argc, char *argv[])
{
	try
	{
		// Reading the arguments of the program
		CommandLine commandline(argc, argv);

		// Reading the data file and initializing some data structures
		//std::cout << "----- READING DATA SET: " << commandline.pathInstance << std::endl;
		Params params(commandline.pathInstance, commandline.nbVeh, commandline.seed, commandline.nbIter, commandline.mdmNbElite, commandline.mdmNbPatterns, commandline.mdmNURestarts, commandline.mdmMinSup, commandline.randGen, commandline.distanceType);
		
		if (params.nbClients < 200)
		{
			if (params.mdmNbElite) params.mdmNbElite = 5;
			params.mdmNbPatterns = 5;
			params.mdmMinSup = 0.8;
			params.mdmNURestarts = 0.05;
			if (params.randGeneration < 1.0) params.randGeneration = 0.1;
		}
		else if (params.nbClients < 400)
		{
			if (params.mdmNbElite) params.mdmNbElite = 10;
			params.mdmNbPatterns = 5;
			params.mdmMinSup = 0.8;
			params.mdmNURestarts = 0.05;
			if (params.randGeneration < 1.0) params.randGeneration = 0.8;
		}
		else
		{
			if (params.nbClients < 1001)
			{
				if (params.mdmNbElite) params.mdmNbElite = 5;
				params.mdmNbPatterns = 5;
				params.mdmMinSup = 0.8;
				params.mdmNURestarts = 0.05;
			}
			else
			{
				params.mdmNbElite = 0;
				params.mdmNbPatterns = 0;
				params.mdmMinSup = 0;
				params.mdmNURestarts = 0;
			}
			if (params.randGeneration < 1.0) params.randGeneration = 0.2;
		}
		
		std::cout << "Params: timeout = " << commandline.timeLimit << "; e_size = " << params.mdmNbElite << "; patterns = " << params.mdmNbPatterns << "; sup = " << params.mdmMinSup << "; nu_restarts = " << params.mdmNURestarts << "; rg = " << params.randGeneration << "; seed = " << params.seed << std::endl;

		// Creating the Split and local search structures
		Split split(&params);
		LocalSearch localSearch(&params);

		// Initial population
		//std::cout << "----- INSTANCE LOADED WITH " << params.nbClients << " CLIENTS AND " << params.nbVehicles << " VEHICLES" << std::endl;
		//std::cout << "----- BUILDING INITIAL POPULATION" << std::endl;
		Population population(&params, &split, &localSearch);

		// Genetic algorithm
		//std::cout << "----- STARTING GENETIC ALGORITHM" << std::endl;
		Genetic solver(&params, &split, &population, &localSearch);
		solver.run(commandline.nbIter, commandline.timeLimit);
		//std::cout << "----- GENETIC ALGORITHM FINISHED, TIME SPENT: " << (double)clock()/(double)CLOCKS_PER_SEC << std::endl;

		// Exporting the best solution
		/*if (population.getBestFound() != NULL)
		{
			population.getBestFound()->exportCVRPLibFormat(commandline.pathSolution);
			population.exportSearchProgress(commandline.pathSolution + ".PG.csv", commandline.pathInstance, commandline.seed);
			if (commandline.pathBKS != "") population.exportBKS(commandline.pathBKS);
		}*/
	}
	catch (const string& e) { std::cout << "EXCEPTION | " << e << std::endl; }
	catch (const std::exception& e) { std::cout << "EXCEPTION | " << e.what() << std::endl; }
	return 0;
}
