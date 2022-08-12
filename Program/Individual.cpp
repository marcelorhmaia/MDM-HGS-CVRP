#include <cfloat>
#include <iomanip>

#include "Individual.h" 

void Individual::evaluateCompleteCost()
{
	myCostSol = CostSol();
	for (int r = 0; r < params->nbVehicles; r++)
	{
		if (!chromR[r].empty())
		{
			double distance = params->timeCost[0][chromR[r][0]];
			double load = params->cli[chromR[r][0]].demand;
			double service = params->cli[chromR[r][0]].serviceDuration;
			predecessors[chromR[r][0]] = 0;
			for (int i = 1; i < (int)chromR[r].size(); i++)
			{
				distance += params->timeCost[chromR[r][i-1]][chromR[r][i]];
				load += params->cli[chromR[r][i]].demand;
				service += params->cli[chromR[r][i]].serviceDuration;
				predecessors[chromR[r][i]] = chromR[r][i-1];
				successors[chromR[r][i-1]] = chromR[r][i];
			}
			successors[chromR[r][chromR[r].size()-1]] = 0;
			distance += params->timeCost[chromR[r][chromR[r].size()-1]][0];
			myCostSol.distance += distance;
			myCostSol.nbRoutes++;
			if (load > params->vehicleCapacity) myCostSol.capacityExcess += load - params->vehicleCapacity;
			if (distance + service > params->durationLimit) myCostSol.durationExcess += distance + service - params->durationLimit;
		}
	}

	myCostSol.penalizedCost = myCostSol.distance + myCostSol.capacityExcess*params->penaltyCapacity + myCostSol.durationExcess*params->penaltyDuration;
	isFeasible = (myCostSol.capacityExcess < MY_EPSILON && myCostSol.durationExcess < MY_EPSILON);
}

void Individual::removeProximity(Individual * indiv)
{
	auto it = indivsPerProximity.begin();
	while (it->second != indiv) ++it;
	indivsPerProximity.erase(it);
}

double Individual::brokenPairsDistance(Individual * indiv2)
{
	int differences = 0;
	for (int j = 1; j <= params->nbClients; j++)
	{
		if (successors[j] != indiv2->successors[j] && successors[j] != indiv2->predecessors[j]) differences++;
		if (predecessors[j] == 0 && indiv2->predecessors[j] != 0 && indiv2->successors[j] != 0) differences++;
	}
	return (double)differences/(double)params->nbClients;
}

double Individual::averageBrokenPairsDistanceClosest(int nbClosest) 
{
	double result = 0 ;
	int maxSize = std::min<int>(nbClosest, indivsPerProximity.size());
	auto it = indivsPerProximity.begin();
	for (int i=0 ; i < maxSize; i++)
	{
		result += it->first ;
		++it ;
	}
	return result/(double)maxSize ;
}

void Individual::exportCVRPLibFormat(std::string fileName)
{
	std::cout << "----- WRITING SOLUTION WITH VALUE " << myCostSol.penalizedCost << " IN : " << fileName << std::endl;
	std::ofstream myfile(fileName);
	if (myfile.is_open())
	{
		for (int k = 0; k < params->nbVehicles; k++)
		{
			if (!chromR[k].empty())
			{
				myfile << "Route #" << k+1 << ":"; // Route IDs start at 1 in the file format
				for (int i : chromR[k]) myfile << " " << i;
				myfile << std::endl;
			}
		}
		myfile << "Cost " << myCostSol.penalizedCost << std::endl;
		myfile << "Time " << (double)clock()/(double)CLOCKS_PER_SEC << std::endl;
	}
	else std::cout << "----- IMPOSSIBLE TO OPEN: " << fileName << std::endl;
}

bool Individual::readCVRPLibFormat(std::string fileName, std::vector<std::vector<int>> & readSolution, double & readCost)
{
	readSolution.clear();
	std::ifstream inputFile(fileName);
	if (inputFile.is_open())
	{
		std::string inputString;
		inputFile >> inputString;
		// Loops as long as the first line keyword is "Route"
		for (int r = 0; inputString == "Route" ; r++) 
		{
			readSolution.push_back(std::vector<int>());
			inputFile >> inputString;
			getline(inputFile, inputString);
			std::stringstream ss(inputString);
			int inputCustomer;
			while (ss >> inputCustomer) // Loops as long as there is an integer to read
				readSolution[r].push_back(inputCustomer);
			inputFile >> inputString;
		}
		if (inputString == "Cost")
		{
			inputFile >> readCost;
			return true;
		}
		else std::cout << "----- UNEXPECTED WORD IN SOLUTION FORMAT: " << inputString << std::endl;
	}
	else std::cout << "----- IMPOSSIBLE TO OPEN: " << fileName << std::endl;
	return false;
}

void Individual::printCVRPLibFormat() const
{
	std::cout << "\n";
	for (int k = 0; k < params->nbVehicles; k++)
	{
		if (!chromR[k].empty())
		{
			std::cout << "Route #" << k+1 << ":"; // Route IDs start at 1 in the file format
			for (int i : chromR[k]) std::cout << " " << i;
			std::cout << "\n";
		}
	}
	std::cout << "Cost " << std::setprecision(2) << std::fixed << myCostSol.penalizedCost << std::endl;
}

Individual::Individual(Params * params, bool rcws, std::vector < std::vector <int> >* pattern) : params(params)
{
	successors = std::vector <int>(params->nbClients + 1);
	predecessors = std::vector <int>(params->nbClients + 1);
	chromR = std::vector < std::vector <int> >(params->nbVehicles);
	chromT = std::vector <int>(params->nbClients);
	
	if (rcws)
	{
		std::vector <bool> inRoute = std::vector <bool>(params->nbClients + 1, false);
		std::vector <bool> interior = std::vector <bool>(params->nbClients + 1, false);
		std::vector <double> load = std::vector <double>(params->nbVehicles, 0);
		std::vector <Savings> tournamentSavings = std::vector <Savings>(6);
		std::vector <double> selectionProbabilities = std::vector <double>(6);
		int tournamentSavingsOccupancy = 0;
		unsigned savingsCount = 0;
		int nextEmptyRoute = 0;
		int nbVehicles = params->nbVehicles;
		
		if (pattern)
			for (unsigned r = 0; r < pattern->size(); r++)
			{
				if (nextEmptyRoute == nbVehicles)
				{
					nbVehicles++;
					chromR.push_back(std::vector <int>());
					load.push_back(0);
				}
				for (unsigned c = 0; c < (*pattern)[r].size(); c++)
				{
					chromR[nextEmptyRoute].push_back((*pattern)[r][c]);
					load[nextEmptyRoute] += params->cli[(*pattern)[r][c]].demand;
					inRoute[(*pattern)[r][c]] = true;
					if (c && c != (*pattern)[r].size() - 1)
						interior[(*pattern)[r][c]] = true;
				}
				while (nextEmptyRoute < nbVehicles && !chromR[nextEmptyRoute].empty()) nextEmptyRoute++;
			}
		
		while (savingsCount < params->savingsList.size() || tournamentSavingsOccupancy > 0)
		{
			int tournamentSize = std::min(2 + std::rand() % 5, (int)params->savingsList.size() - (int)savingsCount + tournamentSavingsOccupancy);
			
			while (tournamentSavingsOccupancy < tournamentSize)
			{
				Savings s = params->savingsList[savingsCount++];
				if (s.value > 0)
					tournamentSavings[tournamentSavingsOccupancy++] = s;
				else
				{
					tournamentSize = tournamentSavingsOccupancy;
					savingsCount = params->savingsList.size();
				}
			}
			
			double tournamentSavingsSum = 0;
			for (int i = 0; i < tournamentSize; i++)
				tournamentSavingsSum += tournamentSavings[i].value;
			
			for (int i = 0; i < tournamentSize; i++)
				selectionProbabilities[i] = tournamentSavings[i].value / tournamentSavingsSum;
			
			double cumulativeProbability = 0;
			double rand = (double)std::rand() / RAND_MAX;
			
			for (int i = 0; i < tournamentSize; i++)
			{
				if (rand <= selectionProbabilities[i] + cumulativeProbability)
				{
					//Process tournamentSavings[i]
					if (params->cli[tournamentSavings[i].c1].demand + params->cli[tournamentSavings[i].c2].demand <= params->vehicleCapacity)
					{
						if (!inRoute[tournamentSavings[i].c1] && !inRoute[tournamentSavings[i].c2])
						{
							if (nextEmptyRoute == nbVehicles)
							{
								nbVehicles++;
								chromR.push_back(std::vector <int>());
								load.push_back(0);
							}
							chromR[nextEmptyRoute].push_back(tournamentSavings[i].c1);
							chromR[nextEmptyRoute].push_back(tournamentSavings[i].c2);
							load[nextEmptyRoute] += params->cli[tournamentSavings[i].c1].demand + params->cli[tournamentSavings[i].c2].demand;
							inRoute[tournamentSavings[i].c1] = true;
							inRoute[tournamentSavings[i].c2] = true;
							while (nextEmptyRoute < nbVehicles && !chromR[nextEmptyRoute].empty()) nextEmptyRoute++;
						}
						else if (inRoute[tournamentSavings[i].c1] && !interior[tournamentSavings[i].c1] && !inRoute[tournamentSavings[i].c2])
						{
							for (int r = 0; r < nbVehicles; r++)
								if (!chromR[r].empty())
								{
									if (chromR[r].front() == tournamentSavings[i].c1)
									{
										if (load[r] + params->cli[tournamentSavings[i].c2].demand <= params->vehicleCapacity)
										{
											chromR[r].insert(chromR[r].begin(), tournamentSavings[i].c2);
											load[r] += params->cli[tournamentSavings[i].c2].demand;
											inRoute[tournamentSavings[i].c2] = true;
											if (chromR[r].size() > 2)
												interior[tournamentSavings[i].c1] = true;
										}
										break;
									}
									else if (chromR[r].back() == tournamentSavings[i].c1)
									{
										if (load[r] + params->cli[tournamentSavings[i].c2].demand <= params->vehicleCapacity)
										{
											chromR[r].push_back(tournamentSavings[i].c2);
											load[r] += params->cli[tournamentSavings[i].c2].demand;
											inRoute[tournamentSavings[i].c2] = true;
											if (chromR[r].size() > 2)
												interior[tournamentSavings[i].c1] = true;
										}
										break;
									}
								}
						}
						else if (inRoute[tournamentSavings[i].c2] && !interior[tournamentSavings[i].c2] && !inRoute[tournamentSavings[i].c1])
						{
							for (int r = 0; r < nbVehicles; r++)
								if (!chromR[r].empty())
								{
									if (chromR[r].front() == tournamentSavings[i].c2)
									{
										if (load[r] + params->cli[tournamentSavings[i].c1].demand <= params->vehicleCapacity)
										{
											chromR[r].insert(chromR[r].begin(), tournamentSavings[i].c1);
											load[r] += params->cli[tournamentSavings[i].c1].demand;
											inRoute[tournamentSavings[i].c1] = true;
											if (chromR[r].size() > 2)
												interior[tournamentSavings[i].c2] = true;
										}
										break;
									}
									else if (chromR[r].back() == tournamentSavings[i].c2)
									{
										if (load[r] + params->cli[tournamentSavings[i].c1].demand <= params->vehicleCapacity)
										{
											chromR[r].push_back(tournamentSavings[i].c1);
											load[r] += params->cli[tournamentSavings[i].c1].demand;
											inRoute[tournamentSavings[i].c1] = true;
											if (chromR[r].size() > 2)
												interior[tournamentSavings[i].c2] = true;
										}
										break;
									}
								}
						}
						else if (inRoute[tournamentSavings[i].c1] && !interior[tournamentSavings[i].c1] && inRoute[tournamentSavings[i].c2] && !interior[tournamentSavings[i].c2])
						{
							int r1 = -1;
							int r2 = -1;
							int pos1 = 0;
							int pos2 = 0;
							
							for (int r = 0; r < nbVehicles; r++)
								if (!chromR[r].empty())
								{
									if (chromR[r].front() == tournamentSavings[i].c1)
									{
										r1 = r;
										pos1 = 0;
									}
									else if (chromR[r].back() == tournamentSavings[i].c1)
									{
										r1 = r;
										pos1 = chromR[r].size() - 1;
									}
									
									if (chromR[r].front() == tournamentSavings[i].c2)
									{
										r2 = r;
										pos2 = 0;
									}
									else if (chromR[r].back() == tournamentSavings[i].c2)
									{
										r2 = r;
										pos2 = chromR[r].size() - 1;
									}
									
									if (r1 > -1 && r2 > -1)
									{
										if (r1 != r2 && load[r1] + load[r2] <= params->vehicleCapacity)
										{
											if (pos1 == 0)
											{
												if (pos2 == 0)
												{
													chromR[r1].insert(chromR[r1].begin(), chromR[r2].rbegin(), chromR[r2].rend());
													load[r1] += load[r2];
													chromR[r2].clear();
													load[r2] = 0;
													if (r2 < nextEmptyRoute) nextEmptyRoute = r2;
												}
												else
												{
													chromR[r2].insert(chromR[r2].end(), chromR[r1].begin(), chromR[r1].end());
													load[r2] += load[r1];
													chromR[r1].clear();
													load[r1] = 0;
													if (r1 < nextEmptyRoute) nextEmptyRoute = r1;
												}
										
												interior[tournamentSavings[i].c1] = true;
												interior[tournamentSavings[i].c2] = true;
											}
											else if (pos2 == 0)
											{
												chromR[r1].insert(chromR[r1].end(), chromR[r2].begin(), chromR[r2].end());
												load[r1] += load[r2];
												chromR[r2].clear();
												load[r2] = 0;
												if (r2 < nextEmptyRoute) nextEmptyRoute = r2;
											
												interior[tournamentSavings[i].c1] = true;
												interior[tournamentSavings[i].c2] = true;
											}
											else
											{
												chromR[r1].insert(chromR[r1].end(), chromR[r2].rbegin(), chromR[r2].rend());
												load[r1] += load[r2];
												chromR[r2].clear();
												load[r2] = 0;
												if (r2 < nextEmptyRoute) nextEmptyRoute = r2;
											
												interior[tournamentSavings[i].c1] = true;
												interior[tournamentSavings[i].c2] = true;
											}
										}
											
										break;
									}
								}
						}
					}
					
					//Update tournament structures			
					for (int j = i; j < tournamentSavingsOccupancy - 1; j++)
						tournamentSavings[j] = tournamentSavings[j + 1];
					
					tournamentSavingsOccupancy--;
					break;
				}
				cumulativeProbability += selectionProbabilities[i];
			}
		}
		
		int i = nextEmptyRoute + 1;
		while (nextEmptyRoute < nbVehicles && i < nbVehicles)
		{
			if (!chromR[i].empty())
			{
				std::vector <int> temp = chromR[nextEmptyRoute];
				chromR[nextEmptyRoute] = chromR[i];
				chromR[i] = temp;
				while (nextEmptyRoute < nbVehicles && !chromR[nextEmptyRoute].empty()) nextEmptyRoute++;
				i = nextEmptyRoute;
			}
			i++;
		}
				
		for (int i = 0; i < nbVehicles - params->nbVehicles; i++) chromR.pop_back();
		
		for (int i = 1; i <= params->nbClients; i++)
			if (!inRoute[i])
			{
				int bestRoute = -1;
				double bestInsertionCost = DBL_MAX;
				for (int r = 0; r < params->nbVehicles; r++)
					if (load[r] + params->cli[i].demand <= params->vehicleCapacity)
					{
						int c = 0;
						if (!chromR[r].empty())
							c = chromR[r].back();
						
						if (params->timeCost[c][i] < bestInsertionCost)
						{
							bestRoute = r;
							bestInsertionCost = params->timeCost[c][i];
						}
					}
				
				if (bestRoute < 0)
					for (int r = 0; r < params->nbVehicles; r++)
					{
						int c = chromR[r].back();
						double penalizedCost = params->timeCost[c][i] + (load[r] + params->cli[i].demand - params->vehicleCapacity)*params->penaltyCapacity;
						if (penalizedCost < bestInsertionCost)
						{
							bestRoute = r;
							bestInsertionCost = penalizedCost;
						}
					}
				
				chromR[bestRoute].push_back(i);
				load[bestRoute] += params->cli[i].demand;
			}
		
		int c = 0;
		for (unsigned i = 0; i < chromR.size(); i++)
			for (unsigned j = 0; j < chromR[i].size(); j++)
				chromT[c++] = chromR[i][j];
			
		evaluateCompleteCost();
	}
	else
	{
		for (int i = 0; i < params->nbClients; i++) chromT[i] = i + 1;
		std::random_shuffle(chromT.begin(), chromT.end());
	}
}

Individual::Individual()
{
	myCostSol.penalizedCost = 1.e30;
}