/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. Maxar
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2015, 2016, 2017, 2018 Maxar (http://www.maxar.com/)
 */

#include "DanglerRemover.h"

// Boost
using namespace boost;

// GEOS
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/LineString.h>
#include <geos/geom/Point.h>
#include <geos/operation/distance/DistanceOp.h>
using namespace geos::geom;
using namespace geos::operation::distance;

// Hoot
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/algorithms/DirectionFinder.h>
#include <hoot/core/algorithms/subline-matching/MaximalNearestSubline.h>
#include <hoot/core/conflate/NodeToWayMap.h>
#include <hoot/core/conflate/WorkingMap.h>
#include <hoot/core/criterion/UnknownCriterion.h>
#include <hoot/core/elements/Node.h>
#include <hoot/core/elements/Way.h>
#include <hoot/core/index/OsmMapIndex.h>
#include <hoot/core/manipulators/DanglerRemoverManipulation.h>
#include <hoot/core/util/Log.h>
#include <hoot/core/visitors/FindWaysVisitor.h>
using namespace hoot::elements;

// Qt
#include <qdebug.h>

// Standard
#include <iostream>
using namespace std;

// Tgs
#include <tgs/StreamUtils.h>
using namespace Tgs;

namespace hoot
{

DanglerRemover::DanglerRemover(Meters errorPlus)
{
  _errorPlus = errorPlus;
}

const vector< boost::shared_ptr<Manipulation> >& DanglerRemover::findAllManipulations(
        ConstOsmMapPtr map)
{
  LOG_INFO("Finding all dangle remover manipulations...");

  // Find all Unknown ways
  UnknownCriterion unknownCrit;
  vector<long> unknown = FindWaysVisitor::findWays(map, &unknownCrit);

  // return the result
  return findWayManipulations(map, unknown);
}

const vector< boost::shared_ptr<Manipulation> >& DanglerRemover::findWayManipulations(
        ConstOsmMapPtr map, const vector<long>& wids)
{
  _result.clear();
  _map = map;

  size_t i;
  for (i = 0; i < wids.size(); i++)
  {
    if (i >= 100 && i % 100 == 0)
    {
      PROGRESS_INFO("  finding manipulations: " << i << " / " << wids.size() << "        ");
    }
    if (_map->containsWay(wids[i]))
    {
      // evaluate the way to see if it has a matching candidate and add it to _result.
      _findMatches(wids[i]);
    }
  }

  if (i >= 100)
  {
    LOG_INFO("  finding manipulations: " << wids.size() << " / " << wids.size() << "        ");
  }

  return _result;
}

void DanglerRemover::_findMatches(long baseWayId)
{
  boost::shared_ptr<DanglerRemoverManipulation> result(new DanglerRemoverManipulation(baseWayId, _map, _errorPlus));

  if (result->getScoreEstimate() > 0)
  {
    _result.push_back(result);
  }
}

}
