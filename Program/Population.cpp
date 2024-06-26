#include "fpmax.h"

#include "Population.h"

void Population::generatePopulation()
{
	if (params.verbose) std::cout << "----- BUILDING INITIAL POPULATION" << std::endl;
	
	// A randomized version of the Clarke & Wright savings heuristic is used to generate better individuals faster
	for (int i = 0; i < params.ap.mu * (1.0 - params.ap.randGeneration) && (i == 0 || params.ap.timeLimit == 0 || (double)(clock() - params.startTime) / (double)CLOCKS_PER_SEC < params.ap.timeLimit) ; i++)
	{
		Individual indiv(params, true, nextMDMPattern());
		localSearch.run(indiv, params.penaltyCapacity, params.penaltyDuration);
		addIndividual(indiv, true);
	}

	// Another part is ramdomly generated to keep diversity
	for (int i = 0; i < params.ap.mu * params.ap.randGeneration && (i == 0 || params.ap.timeLimit == 0 || (double)(clock() - params.startTime) / (double)CLOCKS_PER_SEC < params.ap.timeLimit) ; i++)
	{
		Individual randomIndiv(params);
		split.generalSplit(randomIndiv, params.nbVehicles);
		localSearch.run(randomIndiv, params.penaltyCapacity, params.penaltyDuration);
		addIndividual(randomIndiv, true);
		if (!randomIndiv.eval.isFeasible && params.ran() % 2 == 0)  // Repair half of the solutions in case of infeasibility
		{
			localSearch.run(randomIndiv, params.penaltyCapacity*10., params.penaltyDuration*10.);
			if (randomIndiv.eval.isFeasible) addIndividual(randomIndiv, false);
		}
	}
}

bool Population::addIndividual(const Individual & indiv, bool updateFeasible)
{
	if (updateFeasible)
	{
		listFeasibilityLoad.push_back(indiv.eval.capacityExcess < MY_EPSILON);
		listFeasibilityDuration.push_back(indiv.eval.durationExcess < MY_EPSILON);
		listFeasibilityLoad.pop_front();
		listFeasibilityDuration.pop_front();
	}

	// Find the adequate subpopulation in relation to the individual feasibility
	SubPopulation & subpop = (indiv.eval.isFeasible) ? feasibleSubpop : infeasibleSubpop;

	// Create a copy of the individual and updade the proximity structures calculating inter-individual distances
	Individual * myIndividual = new Individual(indiv);
	for (Individual * myIndividual2 : subpop)
	{
		double myDistance = brokenPairsDistance(*myIndividual,*myIndividual2);
		myIndividual2->indivsPerProximity.insert({ myDistance, myIndividual });
		myIndividual->indivsPerProximity.insert({ myDistance, myIndividual2 });
	}

	// Identify the correct location in the subpopulation and insert the individual
	int place = (int)subpop.size();
	while (place > 0 && subpop[place - 1]->eval.penalizedCost > indiv.eval.penalizedCost - MY_EPSILON) place--;
	subpop.emplace(subpop.begin() + place, myIndividual);

	// Trigger a survivor selection if the maximimum subpopulation size is exceeded
	if ((int)subpop.size() > params.ap.mu + params.ap.lambda)
		while ((int)subpop.size() > params.ap.mu)
			removeWorstBiasedFitness(subpop);

	// Track best solution
	if (indiv.eval.isFeasible && indiv.eval.penalizedCost < bestSolutionRestart.eval.penalizedCost - MY_EPSILON)
	{
		updateMDMElite(indiv);
		bestSolutionRestart = indiv; // Copy
		if (indiv.eval.penalizedCost < bestSolutionOverall.eval.penalizedCost - MY_EPSILON)
		{
			bestSolutionOverall = indiv;
			searchProgress.push_back({ clock() - params.startTime , bestSolutionOverall.eval.penalizedCost });
		}
		return true;
	}
	else
		return false;
}

void Population::updateBiasedFitnesses(SubPopulation & pop)
{
	// Ranking the individuals based on their diversity contribution (decreasing order of distance)
	std::vector <std::pair <double, int> > ranking;
	for (int i = 0 ; i < (int)pop.size(); i++) 
		ranking.push_back({-averageBrokenPairsDistanceClosest(*pop[i],params.ap.nbClose),i});
	std::sort(ranking.begin(), ranking.end());

	// Updating the biased fitness values
	if (pop.size() == 1) 
		pop[0]->biasedFitness = 0;
	else
	{
		for (int i = 0; i < (int)pop.size(); i++)
		{
			double divRank = (double)i / (double)(pop.size() - 1); // Ranking from 0 to 1
			double fitRank = (double)ranking[i].second / (double)(pop.size() - 1);
			if ((int)pop.size() <= params.ap.nbElite) // Elite individuals cannot be smaller than population size
				pop[ranking[i].second]->biasedFitness = fitRank;
			else 
				pop[ranking[i].second]->biasedFitness = fitRank + (1.0 - (double)params.ap.nbElite / (double)pop.size()) * divRank;
		}
	}
}

void Population::removeWorstBiasedFitness(SubPopulation & pop)
{
	updateBiasedFitnesses(pop);
	if (pop.size() <= 1) throw std::string("Eliminating the best individual: this should not occur in HGS");

	Individual * worstIndividual = NULL;
	int worstIndividualPosition = -1;
	bool isWorstIndividualClone = false;
	double worstIndividualBiasedFitness = -1.e30;
	for (int i = 1; i < (int)pop.size(); i++)
	{
		bool isClone = (averageBrokenPairsDistanceClosest(*pop[i],1) < MY_EPSILON); // A distance equal to 0 indicates that a clone exists
		if ((isClone && !isWorstIndividualClone) || (isClone == isWorstIndividualClone && pop[i]->biasedFitness > worstIndividualBiasedFitness))
		{
			worstIndividualBiasedFitness = pop[i]->biasedFitness;
			isWorstIndividualClone = isClone;
			worstIndividualPosition = i;
			worstIndividual = pop[i];
		}
	}

	// Removing the individual from the population and freeing memory
	pop.erase(pop.begin() + worstIndividualPosition); 

	// Cleaning its distances from the other individuals in the population
	for (Individual * indiv2 : pop)
	{
		auto it = indiv2->indivsPerProximity.begin();
		while (it->second != worstIndividual) ++it;
		indiv2->indivsPerProximity.erase(it);
	}

	// Freeing memory
	delete worstIndividual; 
}

// Inserts the individual in MDM elite set if: 
// (1) it is different from those already in the set; and 
// (2) the MDM elite set is not full OR this individual has a better penalized cost than at least one of those already in the set
void Population::updateMDMElite(const Individual & indiv)
{
	unsigned old_elite_size = mdmElite.size();
	mdmElite.insert(indiv);
	if (mdmElite.size() > params.ap.mdmNbElite)	// If max number of elite individuals is exceeded, remove the worst
	{
		std::set<Individual>::iterator it = --mdmElite.end();
		if (it->eval.penalizedCost > indiv.eval.penalizedCost)
		{
			mdmEliteNonUpdatingRestarts = 0;
			mdmEliteUpdated = true;
		}
		mdmElite.erase(it);
	}
	else if (mdmElite.size() > old_elite_size)
	{
		mdmEliteNonUpdatingRestarts = 0;
		mdmEliteUpdated = true;
	}
}

void Population::mineElite()
{
	if (mdmEliteUpdated && mdmEliteNonUpdatingRestarts >= mdmEliteMaxNonUpdatingRestarts && mdmElite.size() > 1)
	{
		if (params.verbose) std::cout << "----- MINING PATTERNS FROM MDM ELITE SET" << std::endl;
		
		int minSup = std::max(2, (int) (params.ap.mdmMinSup * mdmElite.size()));
		int numPatterns = params.ap.mdmNbPatterns;

		int nbNodes = params.nbClients + 1; // all clients + depot

		Dataset* dataset = new Dataset;
		for (auto it = mdmElite.begin(); it != mdmElite.end(); ++it)
		{
			Individual indiv = *it;
			std::set<int> transaction;
			for (int r = 0; r < params.nbVehicles; r++)
				for (int c = 0; c < (int) indiv.chromR[r].size() - 1; c++)
				{
					int index = indiv.chromR[r][c] * nbNodes + indiv.chromR[r][c + 1]; // maps 2D matrix cell indices to vector index
					transaction.insert(index);
				}
			dataset->push_back(transaction);
		}

		FISet* frequentItemsets = fpmax(dataset, minSup, numPatterns);

		mdmPatterns.clear();
		for (FISet::iterator it=frequentItemsets->begin(); it!=frequentItemsets->end(); ++it)
		{
			std::vector < std::vector <int> > tempPattern;
			std::list<std::vector <int>*> routes;

			for (std::set<int>::iterator it2=it->begin(); it2!=it->end(); ++it2)
			{
				unsigned index = *it2;

				int n1 = index / nbNodes;
				int n2 = index % nbNodes;

				std::vector <int> *lhs = NULL;
				std::vector <int> *rhs = NULL;
				std::vector <int> *tempRoute;
				for (std::list<std::vector <int>*>::iterator r = routes.begin(); r != routes.end(); r++)
					if (n1 && n1 == (*r)->back())
						lhs = *r;
					else if (n2 && n2 == (*r)->front())
						rhs = *r;
				if (lhs)
					if (rhs)
					{
						for (std::vector<int>::iterator n = rhs->begin(); n != rhs->end(); n++)
							lhs->push_back(*n);
						routes.remove(rhs);
						delete rhs;
					}
					else
						lhs->push_back(n2);
				else if (rhs)
					rhs->insert(rhs->begin(), n1);
				else
				{
					tempRoute = new std::vector <int>;
					tempRoute->push_back(n1);
					tempRoute->push_back(n2);
					routes.push_back(tempRoute);
				}
			}

			for (std::list<std::vector <int>*>::iterator r = routes.begin(); r != routes.end(); r++)
				tempPattern.push_back(**r);

			mdmPatterns.push_back(tempPattern);
		}
		
		delete dataset;
		delete frequentItemsets;

		mdmEliteUpdated = false;
		mdmNextPattern = 0;
	}
}

std::vector < std::vector <int> >* Population::nextMDMPattern()
{
	if (mdmPatterns.empty())
		return NULL;
	
	std::vector < std::vector <int> >* pattern = &(mdmPatterns[mdmNextPattern]);
	mdmNextPattern = (mdmNextPattern + 1) % mdmPatterns.size();

	return pattern;
}

void Population::restart()
{
	mdmEliteNonUpdatingRestarts++;
	
	if (params.verbose) std::cout << "----- RESET: CREATING A NEW POPULATION -----" << std::endl;
	for (Individual * indiv : feasibleSubpop) delete indiv ;
	for (Individual * indiv : infeasibleSubpop) delete indiv;
	feasibleSubpop.clear();
	infeasibleSubpop.clear();
	bestSolutionRestart = Individual(params);
	generatePopulation();
}

void Population::managePenalties()
{
	// Setting some bounds [0.1,100000] to the penalty values for safety
	double fractionFeasibleLoad = (double)std::count(listFeasibilityLoad.begin(), listFeasibilityLoad.end(), true) / (double)listFeasibilityLoad.size();
	if (fractionFeasibleLoad < params.ap.targetFeasible - 0.05 && params.penaltyCapacity < 100000.) params.penaltyCapacity = std::min<double>(params.penaltyCapacity * 1.2,100000.);
	else if (fractionFeasibleLoad > params.ap.targetFeasible + 0.05 && params.penaltyCapacity > 0.1) params.penaltyCapacity = std::max<double>(params.penaltyCapacity * 0.85, 0.1);

	// Setting some bounds [0.1,100000] to the penalty values for safety
	double fractionFeasibleDuration = (double)std::count(listFeasibilityDuration.begin(), listFeasibilityDuration.end(), true) / (double)listFeasibilityDuration.size();
	if (fractionFeasibleDuration < params.ap.targetFeasible - 0.05 && params.penaltyDuration < 100000.)	params.penaltyDuration = std::min<double>(params.penaltyDuration * 1.2,100000.);
	else if (fractionFeasibleDuration > params.ap.targetFeasible + 0.05 && params.penaltyDuration > 0.1) params.penaltyDuration = std::max<double>(params.penaltyDuration * 0.85, 0.1);

	// Update the evaluations
	for (int i = 0; i < (int)infeasibleSubpop.size(); i++)
		infeasibleSubpop[i]->eval.penalizedCost = infeasibleSubpop[i]->eval.distance
		+ params.penaltyCapacity * infeasibleSubpop[i]->eval.capacityExcess
		+ params.penaltyDuration * infeasibleSubpop[i]->eval.durationExcess;

	// If needed, reorder the individuals in the infeasible subpopulation since the penalty values have changed (simple bubble sort for the sake of simplicity)
	for (int i = 0; i < (int)infeasibleSubpop.size(); i++)
	{
		for (int j = 0; j < (int)infeasibleSubpop.size() - i - 1; j++)
		{
			if (infeasibleSubpop[j]->eval.penalizedCost > infeasibleSubpop[j + 1]->eval.penalizedCost + MY_EPSILON)
			{
				Individual * indiv = infeasibleSubpop[j];
				infeasibleSubpop[j] = infeasibleSubpop[j + 1];
				infeasibleSubpop[j + 1] = indiv;
			}
		}
	}
}

const Individual & Population::getBinaryTournament ()
{
	// Picking two individuals with uniform distribution over the union of the feasible and infeasible subpopulations
	std::uniform_int_distribution<> distr(0, feasibleSubpop.size() + infeasibleSubpop.size() - 1);
	int place1 = distr(params.ran);
	int place2 = distr(params.ran);
	Individual * indiv1 = (place1 >= (int)feasibleSubpop.size()) ? infeasibleSubpop[place1 - feasibleSubpop.size()] : feasibleSubpop[place1];
	Individual * indiv2 = (place2 >= (int)feasibleSubpop.size()) ? infeasibleSubpop[place2 - feasibleSubpop.size()] : feasibleSubpop[place2];
	
	// Keeping the best of the two in terms of biased fitness
	updateBiasedFitnesses(feasibleSubpop);
	updateBiasedFitnesses(infeasibleSubpop);
	if (indiv1->biasedFitness < indiv2->biasedFitness) return *indiv1 ;
	else return *indiv2 ;		
}

const Individual * Population::getBestFeasible ()
{
	if (!feasibleSubpop.empty()) return feasibleSubpop[0] ;
	else return NULL ;
}

const Individual * Population::getBestInfeasible ()
{
	if (!infeasibleSubpop.empty()) return infeasibleSubpop[0] ;
	else return NULL ;
}

const Individual * Population::getBestFound()
{
	if (bestSolutionOverall.eval.penalizedCost < 1.e29) return &bestSolutionOverall;
	else return NULL;
}

void Population::printState(int nbIter, int nbIterNoImprovement)
{
	if (params.verbose)
	{
		std::printf("It %6d %6d | T(s) %.2f", nbIter, nbIterNoImprovement, (double)(clock()-params.startTime)/(double)CLOCKS_PER_SEC);

		if (getBestFeasible() != NULL) std::printf(" | Feas %zu %.2f %.2f", feasibleSubpop.size(), getBestFeasible()->eval.penalizedCost, getAverageCost(feasibleSubpop));
		else std::printf(" | NO-FEASIBLE");

		if (getBestInfeasible() != NULL) std::printf(" | Inf %zu %.2f %.2f", infeasibleSubpop.size(), getBestInfeasible()->eval.penalizedCost, getAverageCost(infeasibleSubpop));
		else std::printf(" | NO-INFEASIBLE");

		std::printf(" | Div %.2f %.2f", getDiversity(feasibleSubpop), getDiversity(infeasibleSubpop));
		std::printf(" | Feas %.2f %.2f", (double)std::count(listFeasibilityLoad.begin(), listFeasibilityLoad.end(), true) / (double)listFeasibilityLoad.size(), (double)std::count(listFeasibilityDuration.begin(), listFeasibilityDuration.end(), true) / (double)listFeasibilityDuration.size());
		std::printf(" | Pen %.2f %.2f", params.penaltyCapacity, params.penaltyDuration);
		std::cout << std::endl;
	}
}

double Population::brokenPairsDistance(const Individual & indiv1, const Individual & indiv2)
{
	int differences = 0;
	for (int j = 1; j <= params.nbClients; j++)
	{
		if (indiv1.successors[j] != indiv2.successors[j] && indiv1.successors[j] != indiv2.predecessors[j]) differences++;
		if (indiv1.predecessors[j] == 0 && indiv2.predecessors[j] != 0 && indiv2.successors[j] != 0) differences++;
	}
	return (double)differences / (double)params.nbClients;
}

double Population::averageBrokenPairsDistanceClosest(const Individual & indiv, int nbClosest)
{
	double result = 0.;
	int maxSize = std::min<int>(nbClosest, indiv.indivsPerProximity.size());
	auto it = indiv.indivsPerProximity.begin();
	for (int i = 0; i < maxSize; i++)
	{
		result += it->first;
		++it;
	}
	return result / (double)maxSize;
}

double Population::getDiversity(const SubPopulation & pop)
{
	double average = 0.;
	int size = std::min<int>(params.ap.mu, pop.size()); // Only monitoring the "mu" better solutions to avoid too much noise in the measurements
	for (int i = 0; i < size; i++) average += averageBrokenPairsDistanceClosest(*pop[i],size);
	if (size > 0) return average / (double)size;
	else return -1.0;
}

double Population::getAverageCost(const SubPopulation & pop)
{
	double average = 0.;
	int size = std::min<int>(params.ap.mu, pop.size()); // Only monitoring the "mu" better solutions to avoid too much noise in the measurements
	for (int i = 0; i < size; i++) average += pop[i]->eval.penalizedCost;
	if (size > 0) return average / (double)size;
	else return -1.0;
}

void Population::exportSearchProgress(std::string fileName, std::string instanceName)
{
	std::ofstream myfile(fileName);
	for (std::pair<clock_t, double> state : searchProgress)
		myfile << instanceName << ";" << params.ap.seed << ";" << state.second << ";" << (double)state.first / (double)CLOCKS_PER_SEC << std::endl;
}

void Population::exportCVRPLibFormat(const Individual & indiv, std::string fileName)
{
	std::ofstream myfile(fileName);
	if (myfile.is_open())
	{
		for (int k = 0; k < (int)indiv.chromR.size(); k++)
		{
			if (!indiv.chromR[k].empty())
			{
				myfile << "Route #" << k + 1 << ":"; // Route IDs start at 1 in the file format
				for (int i : indiv.chromR[k]) myfile << " " << i;
				myfile << std::endl;
			}
		}
		myfile << "Cost " << indiv.eval.penalizedCost << std::endl;
	}
	else std::cout << "----- IMPOSSIBLE TO OPEN: " << fileName << std::endl;
}

// Comparator for ordering individuals by penalized cost
bool CompareIndividuals(Individual indiv1, Individual indiv2)
{
	return indiv1.eval.penalizedCost < indiv2.eval.penalizedCost - MY_EPSILON;
}

Population::Population(Params & params, Split & split, LocalSearch & localSearch) : params(params), split(split), localSearch(localSearch), bestSolutionRestart(params), bestSolutionOverall(params), mdmElite(CompareIndividuals)
{
	listFeasibilityLoad = std::list<bool>(100, true);
	listFeasibilityDuration = std::list<bool>(100, true);

	mdmEliteUpdated = false;
	mdmEliteNonUpdatingRestarts = 0;
}

Population::~Population()
{
	for (int i = 0; i < (int)feasibleSubpop.size(); i++) delete feasibleSubpop[i];
	for (int i = 0; i < (int)infeasibleSubpop.size(); i++) delete infeasibleSubpop[i];
}
