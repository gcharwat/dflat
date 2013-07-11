/*
Copyright 2012-2013, Bernhard Bliem
WWW: <http://dbai.tuwien.ac.at/research/project/dflat/>.

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

#include <sstream>
#include <cassert>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "Row.h"
#include "Problem.h"

Row::Row(const Row::Items& topLevelItems, const ExtensionPointerTuple& predecessor)
	: tree(topLevelItems), currentCost(0), cost(0), index(0)
{
	// Default counting
	mpz_class product = 1;
	foreach(const Row* r, predecessor)
		product *= r->getCount();
	count = product;

	if(!predecessor.empty())
		extensionPointers.push_back(predecessor);
}

Row::Row(const Row::Tree& tree, const ExtensionPointerTuple& predecessor)
	: tree(tree), currentCost(0), cost(0), index(0)
{
	// Default counting
	mpz_class product = 1;
	foreach(const Row* r, predecessor)
		product *= r->getCount();
	count = product;

	if(!predecessor.empty())
		extensionPointers.push_back(predecessor);
}

bool Row::Tree::operator==(const Tree& rhs) const
{
	return items == rhs.items && children == rhs.children;
}

bool Row::Tree::operator<(const Tree& rhs) const
{
	return items < rhs.items || (items == rhs.items && children < rhs.children);
}

bool Row::operator<(const sharp::Row& rhs) const
{
	return tree < dynamic_cast<const Row&>(rhs).tree;
}

bool Row::operator==(const sharp::Row& rhs) const
{
	return tree == dynamic_cast<const Row&>(rhs).tree;
}

void Row::unify(sharp::Row& other)
{
	Row& o = dynamic_cast<Row&>(other);
	assert(currentCost == o.currentCost);
	assert(getItems() == o.getItems());

	if(o.cost < cost) {
		count = o.count;
		cost = o.cost;
		// XXX: I think swapping the trees is unnecessary since they must be equal when rows are joined?
		tree.items.swap(o.tree.items);
		tree.children.swap(o.tree.children);
		extensionPointers.swap(o.extensionPointers);
	}
	else if(o.cost == cost) {
		count += o.count;
		extensionPointers.insert(extensionPointers.end(), o.extensionPointers.begin(), o.extensionPointers.end());
	}
}

bool Row::matches(const Row& other) const
{
	return tree == other.tree;
}

Row* Row::join(const Row& other) const
{
	// Since according to matches() the trees must coincide, we suppose equal currentCost
	assert(currentCost == other.currentCost);
	assert(cost >= currentCost);
	assert(other.cost >= other.currentCost);

	Row* t = new Row(*this);
	// currentCost is contained in both this->cost and other.cost, so subtract it once
	t->cost = (this->cost - currentCost) + other.cost;
	t->count = this->count * other.count;
	t->extensionPointers.clear();
	ExtensionPointerTuple predecessors(2);
	predecessors[0] = this;
	predecessors[1] = &other;
	t->extensionPointers.push_back(predecessors);
	return t;
}

void Row::declare(std::ostream& out, unsigned childNumber) const
{
	std::ostringstream rowName;
	rowName << 'r' << childNumber << '_' << index;
	out << "childRow(" << rowName.str() << ',' << childNumber << ")." << std::endl;
	out << "childCount(" << rowName.str() << ',' << count << ")." << std::endl;
	out << "childCost(" << rowName.str() << ',' << cost << ")." << std::endl;
	tree.declare(out, rowName.str());
}

#ifdef PRINT_COMPUTED_ROWS
void Row::Tree::print(std::ostream& out, const std::string& indent) const {
	out << indent;
	foreach(const std::string& value, items)
		out << value << ' ';
	out << std::endl;

	foreach(const Tree& child, children)
		child.print(out, indent + "  ");
}

void Row::print(std::ostream& str) const
{
	str << "[Row " << index << ']' << std::endl;
	tree.print(str);
	str << "Count: " << count << std::endl;
	str << "Cost: " << cost << std::endl;
	str << "Extends:";
	foreach(const ExtensionPointerTuple& tup, extensionPointers) {
		str << ' ';
		char openingChar = '(';
		foreach(const Row* r, tup) {
			str << openingChar << r->getIndex();
			openingChar = ',';
		}
		str << ")";
	}
	str << std::endl << std::endl;
}
#endif

void Row::Tree::declare(std::ostream& out, const std::string& thisName) const
{
	foreach(const std::string& value, items)
		out << "childItem(" << thisName << ',' << value << ")." << std::endl;
	unsigned int i = 0;
	foreach(const Tree& child, children) {
		std::ostringstream childName;
		childName << thisName << '_' << i;
		// Declare this subsidiary item set
		out << "sub(" << thisName << ',' << childName.str() << ")." << std::endl;
		// Print its children recursively
		child.declare(out, childName.str());
		++i;
	}
}
