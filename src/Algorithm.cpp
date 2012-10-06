/*
Copyright 2012, Bernhard Bliem
WWW: <http://dbai.tuwien.ac.at/research/project/dynasp/dflat/>.

This file is part of D-FLAT.

D-FLAT is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

D-FLAT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with D-FLAT.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <gringo/streams.h>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#ifdef WITH_NODE_TIMER
#include <boost/timer/timer.hpp>
#endif

#include "Algorithm.h"
#include "ClaspInputReader.h"
#include "ClaspCallbackGeneral.h"
#include "ClaspCallbackNP.h"

using sharp::Table;

Algorithm::Algorithm(sharp::Problem& problem, const sharp::PlanFactory& planFactory, const std::string& instanceFacts, sharp::NormalizationType normalizationType, bool ignoreOptimization, unsigned int level)
	: AbstractHTDAlgorithm(&problem, planFactory), problem(problem), instanceFacts(instanceFacts), normalizationType(normalizationType), ignoreOptimization(ignoreOptimization), level(level)
#ifdef PROGRESS_REPORT
	  , nodesProcessed(0)
#endif
{
}

Table* Algorithm::computeTable(const sharp::ExtendedHypertree& node, const std::vector<Table*>& childTables)
{
	assert((node.getChildren()->size() == 0 && childTables.size() == 1) || node.getChildren()->size() == childTables.size());

	Table* newTable = new Table;

	// There might be a child table without rows, consider as a child, e.g., a join node without matches.
	foreach(Table* r, childTables)
		if(r->empty())
			return newTable;

	// Input: Original problem instance
	std::stringstream* instance = new std::stringstream;
	*instance << instanceFacts;

	// Input: Bag contents
	std::stringstream* bag = new std::stringstream;
	declareBag(*bag, node);
#ifdef PRINT_BAG_INPUT
	std::cout << std::endl << "Bag input:" << std::endl << bag->str() << std::endl;
#endif

	// Input: Child rows
	std::stringstream* childTableInput = new std::stringstream;
	declareChildTables(*childTableInput, node, childTables);
#ifdef PRINT_CHILD_TABLES_INPUT
	std::cout << std::endl << "Child tables input:" << std::endl << childTableInput->str() << std::endl;
#endif

	// Put these inputs together
	Streams inputStreams;
	inputStreams.addFile(getUserProgram(node), false); // Second parameter: "relative" here means relative to the file added previously, which does not exist yet
	// Remember: "Streams" deletes the appended streams -_-
	inputStreams.appendStream(Streams::StreamPtr(instance), "<instance>");
	inputStreams.appendStream(Streams::StreamPtr(bag), "<bag>");
	inputStreams.appendStream(Streams::StreamPtr(childTableInput), "<child_rows>");

	// Call the ASP solver
	std::auto_ptr<GringoOutputProcessor> outputProcessor = newGringoOutputProcessor();
	ClaspInputReader inputReader(inputStreams, *outputProcessor);
	std::auto_ptr<Clasp::ClaspFacade::Callback> cb = newClaspCallback(*newTable, *outputProcessor, childTables.size(), node.getVertices());
	Clasp::ClaspConfig config;
	config.enumerate.numModels = 0;
	clasp.solve(inputReader, config, cb.get());

	return newTable;
}

Table* Algorithm::evaluateNode(const sharp::ExtendedHypertree* node)
{
	std::vector<Table*> childTables;
	childTables.reserve(node->getChildren()->size());
#ifdef PROGRESS_REPORT
	unsigned numChildRows = 0;
#endif

	foreach(const sharp::Hypertree* child, *node->getChildren()) {
		Table* childTable = evaluateNode(dynamic_cast<const sharp::ExtendedHypertree*>(child));
		childTables.push_back(childTable);
#ifdef PROGRESS_REPORT
		numChildRows += childTable->size();
		++nodesProcessed;
#endif
	}

#ifdef PROGRESS_REPORT
	printProgressLine(node, numChildRows);
#endif
#ifdef WITH_NODE_TIMER
	boost::timer::auto_cpu_timer timer(" %ts\n");
#endif

	Table* newTable = computeTable(*node, childTables);

	foreach(Table* childTable, childTables)
		delete childTable;

#ifdef PRINT_COMPUTED_ROWS
	std::cout << std::endl << "Resulting rows:" << std::endl;
	for(Table::const_iterator it = newTable->begin(); it != newTable->end(); ++it)
		dynamic_cast<Row*>(it->first)->print(std::cout);
	std::cout << std::endl;
#endif

	return newTable;
}

sharp::Plan* Algorithm::selectPlan(Table* table, const sharp::ExtendedHypertree* root)
{
#ifdef PROGRESS_REPORT
	std::cout << '\r' << std::setw(66) << std::left << "Done." << std::endl; // Clear/end progress line
#endif

	if(table->empty())
		return 0;

	Table::const_iterator it = table->begin();
	sharp::Plan* result = it->second;

	for(++it; it != table->end(); ++it)
		result = planFactory.unify(result, it->second);

	return result;
}

sharp::ExtendedHypertree* Algorithm::prepareHypertreeDecomposition(sharp::ExtendedHypertree* root)
{
	// FIXME: This is a workaround until SHARP lets us insert an empty root in the normalization
	sharp::ExtendedHypertree* newRoot = new sharp::ExtendedHypertree(sharp::VertexSet());
	newRoot->insChild(root);

	return newRoot->normalize(normalizationType);
}

std::auto_ptr<Clasp::ClaspFacade::Callback> Algorithm::newClaspCallback(Table& newTable, const GringoOutputProcessor& gringoOutput, unsigned int numChildNodes, const sharp::VertexSet& currentVertices) const
{
	if(level == 0)
		return std::auto_ptr<Clasp::ClaspFacade::Callback>(new ClaspCallbackNP(*this, newTable, gringoOutput, numChildNodes));
	else
		return std::auto_ptr<Clasp::ClaspFacade::Callback>(new ClaspCallbackGeneral(*this, newTable, gringoOutput, numChildNodes, level));
}

std::auto_ptr<GringoOutputProcessor> Algorithm::newGringoOutputProcessor() const
{
	return std::auto_ptr<GringoOutputProcessor>(new GringoOutputProcessor(ignoreOptimization));
}

#ifdef PROGRESS_REPORT
void Algorithm::printProgressLine(const sharp::ExtendedHypertree* node, size_t numChildRows) {
	std::cout << '\r' << "Processing node ";
	std::cout << std::setw(4) << std::left << (nodesProcessed+1) << " [";
	if(normalizationType == sharp::NoNormalization)
		std::cout << node->getChildren()->size();
	else {
		switch(node->getType()) {
			case sharp::Leaf:
				std::cout << 'L';
				break;
			case sharp::Permutation:
				std::cout << 'E';
				break;
			case sharp::Branch:
				std::cout << 'J';
				break;
			case sharp::Introduction:
				std::cout << 'I';
				break;
			case sharp::Removal:
				std::cout << 'R';
				break;
			default:
				assert(false);
		}
	}

	// FIXME: Number of removed/introduced vertices is always 0 when there is more than one child
	std::cout << "] " << std::setw(2) << std::right << node->getVertices().size() << " elements ["
		<< std::setw(2) << node->getRemovedVertices().size() << "R"
		<< std::setw(2) << node->getIntroducedVertices().size() << "I] "
		<< std::setw(7) << numChildRows << " child rows"
		<< std::flush;
}
#endif
