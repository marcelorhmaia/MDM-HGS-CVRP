//
// Created by chkwon on 3/23/22.
//

#include "AlgorithmParameters.h"
#include <iostream>

extern "C"
struct AlgorithmParameters default_algorithm_parameters() {
	struct AlgorithmParameters ap{};

	ap.nbGranular = 20;
	ap.mu = 25;
	ap.lambda = 40;
	ap.nbElite = 4;
	ap.nbClose = 5;
	ap.targetFeasible = 0.2;

	ap.seed = 0;
	ap.nbIter = 20000;
	ap.timeLimit = 0;
	ap.useSwapStar = 1;

	ap.randGeneration = -1;
	
	ap.mdmNbElite = -1;
	ap.mdmNbPatterns = 5;
	ap.mdmNURestarts = 0.05;
	ap.mdmMinSup = 0.8;

	return ap;
}

void print_algorithm_parameters(const AlgorithmParameters & ap)
{
	std::cout << "=========== Algorithm Parameters =================" << std::endl;
	std::cout << "---- nbGranular        is set to " << ap.nbGranular << std::endl;
	std::cout << "---- mu                is set to " << ap.mu << std::endl;
	std::cout << "---- lambda            is set to " << ap.lambda << std::endl;
	std::cout << "---- nbElite           is set to " << ap.nbElite << std::endl;
	std::cout << "---- nbClose           is set to " << ap.nbClose << std::endl;
	std::cout << "---- targetFeasible    is set to " << ap.targetFeasible << std::endl;
	std::cout << "---- seed              is set to " << ap.seed << std::endl;
	std::cout << "---- nbIter            is set to " << ap.nbIter << std::endl;
	std::cout << "---- timeLimit         is set to " << ap.timeLimit << std::endl;
	std::cout << "---- useSwapStar       is set to " << ap.useSwapStar << std::endl;
	std::cout << "---- randGeneration    is set to " << ap.randGeneration << std::endl;
	std::cout << "---- mdmNbElite        is set to " << ap.mdmNbElite << std::endl;
	std::cout << "---- mdmNbPatterns     is set to " << ap.mdmNbPatterns << std::endl;
	std::cout << "---- mdmNURestarts     is set to " << ap.mdmNURestarts << std::endl;
	std::cout << "---- mdmMinSup         is set to " << ap.mdmMinSup << std::endl;
	std::cout << "==================================================" << std::endl;
}
