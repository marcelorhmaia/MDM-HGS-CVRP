/*MIT License

Copyright(c) 2020 Thibaut Vidal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <iostream>
#include <string>
#include <climits>

class CommandLine
{
public:

	int nbIter		= 20000;		// Number of iterations without improvement until termination. Default value: 20,000 iterations
	int timeLimit   = INT_MAX;		// CPU time limit until termination in seconds. Default value: infinity
	int seed		= 0;			// Random seed. Default value: 0
	int nbVeh		= INT_MAX;		// Number of vehicles. Default value: infinity
	std::string pathInstance;		// Instance path
	std::string pathSolution;		// Solution path
	std::string pathBKS = "";		// BKS path
	double randGen		= 0.5;		// Reference proportion for the number of individuals to be generated using the original randomized initialization algorithm
	int mdmNbElite		= 10;		// Maximum number of individuals in the MDM elite
	int mdmNbPatterns	= 10;		// Number of patterns to be mined from elite
	double mdmNURestarts	= 0.05;		// Proportion of restarts without update of the MDM elite to trigger data mining
	double mdmMinSup		= 0.2;		// Minimum support for elite data mining
	int distanceType	= 1;

	// Reads the line of command and extracts possible options
	CommandLine(int argc, char* argv[])
	{
		if (argc < 4 || argc > 5)
		{
			std::cout << "----- NUMBER OF COMMANDLINE ARGUMENTS IS INCORRECT: " << argc << std::endl;
			display_help(); throw std::string("Incorrect line of command");
		}
		else
		{
			pathInstance = std::string(argv[1]);
			distanceType = atoi(argv[2]);
			timeLimit = atoi(argv[3]);
			if (argc == 5)
				seed = atoi(argv[4]);
		}
	}

	// Printing information about how to use the code
	void display_help()
	{
		std::cout << std::endl;
		std::cout << "-------------------------------------------------- MDM-HGS-CVRP algorithm (2021) ----------------------------------------------" << std::endl;
		std::cout << "Call with: ./mdmhgs instancePath distanceType timeLimit [seed]                                                                       " << std::endl;
		std::cout << "-------------------------------------------------------------------------------------------------------------------------------" << std::endl;
		std::cout << std::endl;
	};
};
#endif
