#include <cfloat>

#include "Individual.h" 

void Individual::evaluateCompleteCost(const Params & params)
{
	eval = EvalIndiv();
	for (int r = 0; r < params.nbVehicles; r++)
	{
		if (!chromR[r].empty())
		{
			double distance = params.timeCost[0][chromR[r][0]];
			double load = params.cli[chromR[r][0]].demand;
			double service = params.cli[chromR[r][0]].serviceDuration;
			predecessors[chromR[r][0]] = 0;
			for (int i = 1; i < (int)chromR[r].size(); i++)
			{
				distance += params.timeCost[chromR[r][i-1]][chromR[r][i]];
				load += params.cli[chromR[r][i]].demand;
				service += params.cli[chromR[r][i]].serviceDuration;
				predecessors[chromR[r][i]] = chromR[r][i-1];
				successors[chromR[r][i-1]] = chromR[r][i];
			}
			successors[chromR[r][chromR[r].size()-1]] = 0;
			distance += params.timeCost[chromR[r][chromR[r].size()-1]][0];
			eval.distance += distance;
			eval.nbRoutes++;
			if (load > params.vehicleCapacity) eval.capacityExcess += load - params.vehicleCapacity;
			if (distance + service > params.durationLimit) eval.durationExcess += distance + service - params.durationLimit;
		}
	}

	eval.penalizedCost = eval.distance + eval.capacityExcess*params.penaltyCapacity + eval.durationExcess*params.penaltyDuration;
	eval.isFeasible = (eval.capacityExcess < MY_EPSILON && eval.durationExcess < MY_EPSILON);
}

Individual::Individual(Params & params, bool rcws)
{
	successors = std::vector <int>(params.nbClients + 1);
	predecessors = std::vector <int>(params.nbClients + 1);
	chromR = std::vector < std::vector <int> >(params.nbVehicles);
	chromT = std::vector <int>(params.nbClients);

	if (rcws)	// initialize the individual with a randomized version of the Clarke & Wright savings heuristic
	{
		std::vector <bool> inRoute = std::vector <bool>(params.nbClients + 1, false);
		std::vector <bool> interior = std::vector <bool>(params.nbClients + 1, false);
		std::vector <double> load = std::vector <double>(params.nbVehicles, 0);
		std::vector <Savings> tournamentSavings = std::vector <Savings>(6);
		std::vector <double> selectionProbabilities = std::vector <double>(6);
		int tournamentSavingsOccupancy = 0;
		unsigned savingsCount = 0;
		int nextEmptyRoute = 0;
		int nbVehicles = params.nbVehicles;

		while (savingsCount < params.savingsList.size() || tournamentSavingsOccupancy > 0)
		{
			int tournamentSize = std::min(2 + std::rand() % 5, (int)params.savingsList.size() - (int)savingsCount + tournamentSavingsOccupancy);

			while (tournamentSavingsOccupancy < tournamentSize)
			{
				Savings s = params.savingsList[savingsCount++];
				if (s.value > 0)
					tournamentSavings[tournamentSavingsOccupancy++] = s;
				else
				{
					tournamentSize = tournamentSavingsOccupancy;
					savingsCount = params.savingsList.size();
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
					if (params.cli[tournamentSavings[i].c1].demand + params.cli[tournamentSavings[i].c2].demand <= params.vehicleCapacity)
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
							load[nextEmptyRoute] += params.cli[tournamentSavings[i].c1].demand + params.cli[tournamentSavings[i].c2].demand;
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
										if (load[r] + params.cli[tournamentSavings[i].c2].demand <= params.vehicleCapacity)
										{
											chromR[r].insert(chromR[r].begin(), tournamentSavings[i].c2);
											load[r] += params.cli[tournamentSavings[i].c2].demand;
											inRoute[tournamentSavings[i].c2] = true;
											if (chromR[r].size() > 2)
												interior[tournamentSavings[i].c1] = true;
										}
										break;
									}
									else if (chromR[r].back() == tournamentSavings[i].c1)
									{
										if (load[r] + params.cli[tournamentSavings[i].c2].demand <= params.vehicleCapacity)
										{
											chromR[r].push_back(tournamentSavings[i].c2);
											load[r] += params.cli[tournamentSavings[i].c2].demand;
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
										if (load[r] + params.cli[tournamentSavings[i].c1].demand <= params.vehicleCapacity)
										{
											chromR[r].insert(chromR[r].begin(), tournamentSavings[i].c1);
											load[r] += params.cli[tournamentSavings[i].c1].demand;
											inRoute[tournamentSavings[i].c1] = true;
											if (chromR[r].size() > 2)
												interior[tournamentSavings[i].c2] = true;
										}
										break;
									}
									else if (chromR[r].back() == tournamentSavings[i].c2)
									{
										if (load[r] + params.cli[tournamentSavings[i].c1].demand <= params.vehicleCapacity)
										{
											chromR[r].push_back(tournamentSavings[i].c1);
											load[r] += params.cli[tournamentSavings[i].c1].demand;
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
										if (r1 != r2 && load[r1] + load[r2] <= params.vehicleCapacity)
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

		for (int i = 0; i < nbVehicles - params.nbVehicles; i++) chromR.pop_back();

		for (int i = 1; i <= params.nbClients; i++)
			if (!inRoute[i])
			{
				int bestRoute = -1;
				double bestInsertionCost = DBL_MAX;
				for (int r = 0; r < params.nbVehicles; r++)
					if (load[r] + params.cli[i].demand <= params.vehicleCapacity)
					{
						int c = 0;
						if (!chromR[r].empty())
							c = chromR[r].back();

						if (params.timeCost[c][i] < bestInsertionCost)
						{
							bestRoute = r;
							bestInsertionCost = params.timeCost[c][i];
						}
					}

				if (bestRoute < 0)
					for (int r = 0; r < params.nbVehicles; r++)
					{
						int c = chromR[r].back();
						double penalizedCost = params.timeCost[c][i] + (load[r] + params.cli[i].demand - params.vehicleCapacity)*params.penaltyCapacity;
						if (penalizedCost < bestInsertionCost)
						{
							bestRoute = r;
							bestInsertionCost = penalizedCost;
						}
					}

				chromR[bestRoute].push_back(i);
				load[bestRoute] += params.cli[i].demand;
			}

		int c = 0;
		for (unsigned i = 0; i < chromR.size(); i++)
			for (unsigned j = 0; j < chromR[i].size(); j++)
				chromT[c++] = chromR[i][j];

		evaluateCompleteCost(params);
	}
	else	// initialize the individual with a random permutation
	{
		for (int i = 0; i < params.nbClients; i++) chromT[i] = i + 1;
		std::shuffle(chromT.begin(), chromT.end(), params.ran);
		eval.penalizedCost = 1.e30;
	}
}
