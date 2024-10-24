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
 * @copyright Copyright (C) 2015-2023 Maxar (http://www.maxar.com/)
 */

#include "MapCropper.h"

// GEOS
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/MultiLineString.h>
#include <geos/geom/Point.h>
#include <geos/geom/Polygon.h>
#include <geos/io/WKBReader.h>
#include <geos/io/WKBWriter.h>
#include <geos/util/GEOSException.h>

// Hoot
#include <hoot/core/algorithms/FindNodesInWayFactory.h>
#include <hoot/core/elements/MapProjector.h>
#include <hoot/core/elements/NodeToWayMap.h>
#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/elements/Way.h>
#include <hoot/core/elements/WayUtils.h>
#include <hoot/core/geometry/ElementToGeometryConverter.h>
#include <hoot/core/geometry/GeometryUtils.h>
#include <hoot/core/index/OsmMapIndex.h>
#include <hoot/core/io/OsmMapWriterFactory.h>
#include <hoot/core/ops/RemoveEmptyRelationsOp.h>
#include <hoot/core/ops/RemoveNodeByEid.h>
#include <hoot/core/ops/RemoveWayByEid.h>
#include <hoot/core/ops/SuperfluousNodeRemover.h>
#include <hoot/core/ops/SuperfluousWayRemover.h>
#include <hoot/core/schema/OsmSchema.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/util/Factory.h>
#include <hoot/core/util/StringUtils.h>
#include <hoot/core/util/Validate.h>
#include <hoot/core/visitors/RemoveMissingElementsVisitor.h>

// Standard
#include <limits>

// TGS
#include <tgs/StreamUtils.h>

using namespace geos::geom;
using namespace std;
using namespace Tgs;

namespace hoot
{

int MapCropper::logWarnCount = 0;

HOOT_FACTORY_REGISTER(OsmMapOperation, MapCropper)

MapCropper::MapCropper()
  : _invert(false),
    _keepEntireFeaturesCrossingBounds(false),
    _keepOnlyFeaturesInsideBounds(false),
    _removeSuperfluousFeatures(true),
    _removeMissingElements(true),
    _statusUpdateInterval(1000),
    _numWaysInBounds(0),
    _numWaysOutOfBounds(0),
    _numWaysCrossingThreshold(0),
    _numCrossingWaysKept(0),
    _numCrossingWaysRemoved(0),
    _numNodesRemoved(0),
    _logWarningsForMissingElements(true),
    _removeFromParentRelation(true)
{
}

QString MapCropper::getInitStatusMessage() const
{
  QString msg = "Cropping map at bounds: ";
  if (_bounds)
  {
    QString boundsStr;
    std::shared_ptr<geos::geom::Polygon> polyBounds = std::dynamic_pointer_cast<geos::geom::Polygon>(_bounds);
    if (polyBounds) // This is easier to read.
      boundsStr = GeometryUtils::polygonToString(polyBounds);
    else
      boundsStr = QString::fromStdString(_bounds->toString());
    msg += "..." + boundsStr.right(ConfigOptions().getProgressVarPrintLengthMax() * 2);
  }
  msg += "...";
  return msg;
}

void MapCropper::setInvert(bool invert)
{
  _invert = invert;
  // Haven't seen evidence that these options make sense when we're doing inverted cropping, so
  // let's leave them turned off if inversion is selected.
  if (!_invert)
  {
    _keepOnlyFeaturesInsideBounds = false;
    _keepEntireFeaturesCrossingBounds = false;
  }
}

void MapCropper::setKeepEntireFeaturesCrossingBounds(bool keep)
{
  // this option is ignored when set to invert
  if (_invert)
    _keepEntireFeaturesCrossingBounds = false;
  // this option is incomptible with the option to keep only features inside the bounds
  else if (keep && _keepOnlyFeaturesInsideBounds)
    throw IllegalArgumentException("Incompatible crop options: _keepOnlyFeaturesInsideBounds and _keepEntireFeaturesCrossingBounds cannot both be enabled.");
  else
    _keepEntireFeaturesCrossingBounds = keep;
}

void MapCropper::setKeepOnlyFeaturesInsideBounds(bool keep)
{
  // this option is ignored when set to invert
  if (_invert)
    _keepOnlyFeaturesInsideBounds = false;
  // this option is incomptible with the option to keep features crossing the bounds
  else if (keep && _keepEntireFeaturesCrossingBounds)
    throw IllegalArgumentException("Incompatible crop options: _keepOnlyFeaturesInsideBounds and _keepEntireFeaturesCrossingBounds cannot both be enabled.");
  else
    _keepOnlyFeaturesInsideBounds = keep;
}

void MapCropper::setConfiguration(const Settings& conf)
{
  ConfigOptions confOpts = ConfigOptions(conf);

  setBounds(Boundable::loadCropBounds(confOpts));

  // invert must be set before the two options following it
  setInvert(confOpts.getCropInvert());
  setKeepEntireFeaturesCrossingBounds(confOpts.getCropKeepEntireFeaturesCrossingBounds());
  setKeepOnlyFeaturesInsideBounds(confOpts.getCropKeepOnlyFeaturesInsideBounds());

  setLogWarningsForMissingElements(confOpts.getLogWarningsForMissingElements());

  setRemoveFromParentRelation(confOpts.getCropRemoveFeaturesFromRelations());

  _statusUpdateInterval = confOpts.getTaskStatusUpdateInterval();
}

void MapCropper::apply(OsmMapPtr& map)
{
  LOG_DEBUG("Cropping ways...");
  LOG_VARD(map->size());

  if (!_bounds)
    throw IllegalArgumentException("No bounds set on MapCropper.");

  _numProcessed = 0;
  _numAffected = 0;
  _numWaysInBounds = 0;
  _numWaysOutOfBounds = 0;
  _numWaysCrossingThreshold = 0;
  _numCrossingWaysKept = 0;
  _numCrossingWaysRemoved = 0;
  _numNodesRemoved = 0;
  _explicitlyIncludedWayIds.clear();
  ElementToGeometryConverter elementConverter(map, _logWarningsForMissingElements);

  LOG_VARD(_invert);
  LOG_VARD(_keepEntireFeaturesCrossingBounds);
  LOG_VARD(_keepOnlyFeaturesInsideBounds);
  LOG_VARD(_bounds->toString());
  LOG_VARD(_inclusionCrit.get());

  //  First iteration finds the elements to delete and crop
  vector<long> waysToRemove;
  vector<long> waysToRemoveFully;
  vector<long> waysToCrop;
  // go through all the ways
  long wayCtr = 0;
  const WayMap& ways = map->getWays();
  for (auto it = ways.begin(); it != ways.end(); ++it)
  {
    const std::shared_ptr<Way>& w = it->second;
    LOG_VART(w.get());
    LOG_TRACE("Checking " << w->getElementId() << " for cropping...");
    LOG_VART(w->getNodeIds());
    LOG_VART(w);

    if (_inclusionCrit)
    {
      LOG_VART(_inclusionCrit->isSatisfied(w));
    }
    if (_inclusionCrit && _inclusionCrit->isSatisfied(w))
    {
      // keep the way; We don't need to do a geometry check, since it was explicitly included.
      LOG_TRACE("Keeping explicitly included way: " << w->getElementId() << "...");
      _explicitlyIncludedWayIds.insert(w->getId());
      _numWaysInBounds++;
      _numProcessed++;
      wayCtr++;
      continue;
    }

    std::shared_ptr<LineString> ls = elementConverter.convertToLineString(w);
    if (!ls.get())
    {
      if (_logWarningsForMissingElements)
      {
        if (logWarnCount < Log::getWarnMessageLimit())
        {
          LOG_WARN("Couldn't convert " << w->getElementId() << " to line string. Keeping way...");
          LOG_VARD(w);
        }
        else if (logWarnCount == Log::getWarnMessageLimit())
        {
          LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
        }
        logWarnCount++;
      }

      _numProcessed++;
      wayCtr++;
      continue;
    }
    const Envelope& wayEnv = *(ls->getEnvelopeInternal());
    LOG_VART(wayEnv);

    // It seems very unnecessary to check against both the way's linestring geometry and its
    // envelope here, however, this is how this was originally written after the option to check
    // against a geometry was added (the class originally only checked against envelopes). Several
    // test failures occur if you just try to check one or the other (checking against the
    // linestring geometry seems to make more sense...but maybe not...). Checking both could
    // contribute to crop performance issues. Opened #4359 to look further into it.
    if (_isWhollyOutside(wayEnv) || _isWhollyOutside(*ls))
    {
      // remove the way
      LOG_TRACE("Dropping wholly outside way: " << w->getElementId() << "...");
      //  Removal is based on the parent setting, either remove it fully or leave it in the relation
      if (_removeFromParentRelation)
        waysToRemoveFully.emplace_back(w->getId());
      else
        waysToRemove.emplace_back(w->getId());
      _numWaysOutOfBounds++;
      _numAffected++;
    }
    // For whatever reason, the inside check against an envelope only causes no problems, but
    // checking against just the geometry yields test failures.
    else if (_isWhollyInside(wayEnv))
    {
      // keep the way
      LOG_TRACE("Keeping wholly inside way: " << w->getElementId() << "...");
      _numWaysInBounds++;
    }
    else if (_keepOnlyFeaturesInsideBounds)
    {
      // Way isn't wholly inside and the configuration requires it to be, so remove the way.
      LOG_TRACE("Dropping due to _keepOnlyFeaturesInsideBounds=true: " << w->getElementId() << "...");
      waysToRemoveFully.emplace_back(w->getId());
      _numWaysOutOfBounds++;
      _numAffected++;
    }
    else if (!_keepEntireFeaturesCrossingBounds)
    {
      // Way crosses the boundary and we're not configured to keep ways that cross the bounds, so
      // do an expensive operation to decide how much to keep, if any.
      LOG_TRACE("Cropping due to _keepEntireFeaturesCrossingBounds=false: " << w->getElementId() << "...");
      waysToCrop.emplace_back(w->getId());
      _numWaysCrossingThreshold++;
    }
    else
    {
      // keep the way
      LOG_TRACE("Keeping way: " << w->getElementId() << "...");
      _numWaysInBounds++;
    }

    wayCtr++;
    _numProcessed++;
    if (wayCtr % _statusUpdateInterval == 0)
    {
      PROGRESS_INFO(
        "Cropped " << StringUtils::formatLargeNumber(wayCtr) << " of " <<
        StringUtils::formatLargeNumber(ways.size()) << " ways.");
    }
  }

  //  Bulk remove ways from map and relations too
  map->bulkRemoveWays(waysToRemoveFully, true);

  //  Bulk remove ways from map only
  map->bulkRemoveWays(waysToRemove, false);

  //  Iterate the ways that cross the bounds and crop
  for (auto id : waysToCrop)
    _cropWay(map, id);

  LOG_VARD(map->size());
  OsmMapWriterFactory::writeDebugMap(map, className(), "after-way-removal");

  std::shared_ptr<NodeToWayMap> n2w = map->getIndex().getNodeToWayMap();

  LOG_DEBUG("Removing nodes...");

  // go through all the nodes
  long nodeCtr = 0;
  long nodesRemoved = 0;
  //  Make a copy of nodes so the original can be modified
  const NodeMap nodes = map->getNodes();
  for (auto it = nodes.begin(); it != nodes.end(); ++it)
  {
    NodePtr node = it->second;
    LOG_TRACE("Checking " << node->getElementId() << " for cropping...");
    LOG_VART(node);

    bool nodeInside = false;

    LOG_VART(_explicitlyIncludedWayIds.size());
    if (!_explicitlyIncludedWayIds.empty())
    {
      LOG_VART(WayUtils::nodeContainedByAnyWay(node->getId(), _explicitlyIncludedWayIds, map));
    }
    if (!_inclusionCrit && _explicitlyIncludedWayIds.empty() &&
        WayUtils::nodeContainedByAnyWay(node->getId(), _explicitlyIncludedWayIds, map))
    {
      LOG_TRACE("Skipping delete for: " << node->getElementId() << " belonging to explicitly included way(s)...");
    }
    else
    {
      const Coordinate& c = it->second->toCoordinate();
      std::shared_ptr<Point> p(GeometryFactory::getDefaultInstance()->createPoint(c));
      LOG_VART(c.toString());
      if (_invert == false)
      {
        nodeInside = _bounds->covers(p.get());
        LOG_TRACE("Node inside check: non-inverted crop and the envelope covers the element=" << nodeInside);
      }
      else
      {
        nodeInside = !_bounds->covers(p.get());
        LOG_TRACE("Node inside check: inverted crop and the envelope covers the element=" << !nodeInside);
      }
      LOG_VART(nodeInside);

      // If the node is outside and the node is within the limiting bounds, and the node is not part of a way
      if (!nodeInside && n2w->find(it->first) == n2w->end())
      {
        // remove the node.
        LOG_TRACE("Removing node with coords: " << it->second->getX() << " : " << it->second->getY());
        RemoveNodeByEid::removeNodeNoCheck(map, it->second->getId());
        nodesRemoved++;
        _numAffected++;
      }
    }

    nodeCtr++;
    _numProcessed++;
    if (nodeCtr % _statusUpdateInterval == 0)
    {
      PROGRESS_INFO(
        "Cropped " << StringUtils::formatLargeNumber(nodeCtr) << " of " <<
        StringUtils::formatLargeNumber(nodes.size()) << " nodes.");
    }
  }
  LOG_VARD(map->size());
  OsmMapWriterFactory::writeDebugMap(map, className(), "after-node-removal");

  // Remove dangling features here now, which used to be done in CropCmd only.
  long numSuperfluousWaysRemoved = 0;
  long numSuperfluousNodesRemoved = 0;
  if (_removeSuperfluousFeatures)
  {
    numSuperfluousWaysRemoved = SuperfluousWayRemover::removeWays(map);
    OsmMapWriterFactory::writeDebugMap(map, className(), "after-superfluous-way-removal");
    numSuperfluousNodesRemoved = SuperfluousNodeRemover::removeNodes(map);
    OsmMapWriterFactory::writeDebugMap(map, className(), "after-superfluous-node-removal");
  }

  // Most of the time we want to remove missing refs in order for the output to be clean. In some
  // workflows like cut and replace where the input relations may not have been fully hydrated,
  // however, we need to keep them around to prevent the resulting changeset from being too heavy
  // handed.
  if (_removeMissingElements)
  {
    // This will handle removing refs in relation members we've cropped out.
    LOG_VARD(map->size());
    RemoveMissingElementsVisitor missingElementsRemover;
    LOG_INFO("\t" << missingElementsRemover.getInitStatusMessage());
    map->visitRw(missingElementsRemover);
    LOG_DEBUG("\t" << missingElementsRemover.getCompletedStatusMessage());
    LOG_VARD(map->size());
    OsmMapWriterFactory::writeDebugMap(map, className(), "after-missing-elements-removal");

    // This will remove any relations that were already empty or became empty after the previous
    // step.
    LOG_VARD(map->size());
    RemoveEmptyRelationsOp emptyRelationRemover;
    LOG_INFO("\t" << emptyRelationRemover.getInitStatusMessage());
    emptyRelationRemover.apply(map);
    LOG_DEBUG("\t" << emptyRelationRemover.getCompletedStatusMessage());
    OsmMapWriterFactory::writeDebugMap(map, className(), "after-empty-relations-removal");
  }

  LOG_VARD(_numAffected);
  LOG_VARD(map->size());
  LOG_VARD(StringUtils::formatLargeNumber(_numWaysInBounds));
  LOG_VARD(StringUtils::formatLargeNumber(_numWaysOutOfBounds));
  LOG_VARD(StringUtils::formatLargeNumber(_numWaysCrossingThreshold));
  LOG_VARD(StringUtils::formatLargeNumber(_numCrossingWaysKept));
  LOG_VARD(StringUtils::formatLargeNumber(_numCrossingWaysRemoved));
  LOG_VARD(StringUtils::formatLargeNumber(_numNodesRemoved));
  LOG_VARD(numSuperfluousWaysRemoved);
  LOG_VARD(numSuperfluousNodesRemoved);
}

void MapCropper::_cropWay(const OsmMapPtr& map, long wid)
{
  LOG_TRACE("Cropping way crossing bounds: " << wid << "...");

  std::shared_ptr<Way> way = map->getWay(wid);
  std::shared_ptr<Geometry> fg = ElementToGeometryConverter(map, _logWarningsForMissingElements).convertToGeometry(way);
  LOG_VART(GeometryUtils::geometryTypeIdToString(fg));
  if (!fg || fg->isEmpty())
    return;

  // perform the intersection with the geometry
  std::shared_ptr<Geometry> g;
  try
  {
    if (_invert)
      g = fg->difference(_bounds.get());
    else
      g = fg->intersection(_bounds.get());
  }
  catch (const geos::util::GEOSException&)
  {
    // try cleaning up the geometry and try again.
    fg.reset(GeometryUtils::validateGeometry(fg.get()));
    if (_invert)
      g = fg->difference(_bounds.get());
    else
      g = fg->intersection(_bounds.get());
  }
  LOG_VART(GeometryUtils::geometryTypeIdToString(g));

  std::shared_ptr<FindNodesInWayFactory> nodeFactory = std::make_shared<FindNodesInWayFactory>(way);
  GeometryToElementConverter gc(map);
  gc.setNodeFactory(nodeFactory);
  ElementPtr e = gc.convertGeometryToElement(g.get(), way->getStatus(), way->getCircularError());
  LOG_VART(e.get());
  if (!e)
  {
    LOG_TRACE(way->getElementId() << " converted geometry can't be converted to an element. Skipping cropping...");
    return;
  }

  // If the cropped version of the way ends up being cropped down to a single node, throw it out.
  if (e->getElementType() == ElementType::Node)
  {
    LOG_TRACE(way->getElementId() << " converted geometry is a single node. Skipping cropping...");
    return;
  }

  if (e == nullptr)
  {
    LOG_TRACE("Removing way during crop check: " << way->getElementId() << "...");
    RemoveWayByEid::removeWayFully(map, way->getId());
    _numCrossingWaysRemoved++;
    _numAffected++;
  }
  else
  {
    LOG_TRACE("Replacing way during crop check: " << way->getElementId() << " with element: " << e->getElementId() << "...");

    if (e->getElementType() == ElementType::Way)
    {
      //  Update the current way with the cropped node IDs only
      WayPtr newWay = std::dynamic_pointer_cast<Way>(e);
      way->setNodes(newWay->getNodeIds());
      //  In some instances, the new element has already been added to the map, remove it here
      if (map->containsWay(e->getId()))
        map->bulkRemoveWays({e->getId()}, false);
    }
    else if (e->getElementType() == ElementType::Relation)
    {
      //  Find the way with the most nodes in the relation to retain the ID
      long eid = 0;
      size_t max_nodes = 0;
      //  When cropping a way that turns into a relation, one of the ways should retain the original ID
      RelationPtr newRelation = std::dynamic_pointer_cast<Relation>(e);
      const vector<RelationData::Entry>& members = newRelation->getMembers();
      for (const auto& element : members)
      {
        if (element.getElementId().getType() == ElementType::Way)
        {
          WayPtr memberWay = map->getWay(element.getElementId().getId());
          memberWay->setPid(way->getId());
          // Retain the way tags here and not on the multilinestring relation
          memberWay->setTags(way->getTags());
          if (memberWay->getNodeCount() > max_nodes)
          {
            eid = memberWay->getId();
            max_nodes = memberWay->getNodeCount();
          }
        }
      }
      WayPtr oldWay = std::dynamic_pointer_cast<Way>(way->clone());
      //  Replace the way with the relation element
      map->replace(way, e);
      //  Replace the new way in the relation with the modified way
      if (eid != 0)
      {
        //  Update the old way and replace the old one with the new one just created in the geometry
        WayPtr newWay = map->getWay(eid);
        oldWay->setNodes(newWay->getNodeIds());
        map->replace(newWay, oldWay);
      }
    }
    _numCrossingWaysKept++;
  }
}

bool MapCropper::_isWhollyInside(const Envelope& e) const
{
  bool result = false;;
  if (_invert)
  {
    result = !_bounds->getEnvelopeInternal()->intersects(e);
    LOG_TRACE("Wholly inside way check: inverted crop and the envelope intersects with the element=" << !result);
  }
  else
  {
    // If it isn't inverted, we need to do an expensive check.
    result = _bounds->getEnvelopeInternal()->covers(e);
    LOG_TRACE("Wholly inside way check: non-inverted crop and the envelope covers the element=" << result);
  }
  LOG_TRACE("Wholly inside way check result: " << result);
  return result;
}

bool MapCropper::_isWhollyOutside(const Envelope& e) const
{
  bool result = false;
  if (!_invert)
  {
    LOG_VART(_bounds->toString());
    LOG_VART(_bounds->getEnvelopeInternal()->toString());
    result = !_bounds->getEnvelopeInternal()->intersects(e);
    LOG_TRACE("Wholly outside way check: non-inverted crop and the envelope intersects with the element=" << !result);
  }
  else
  {
    result = _bounds->getEnvelopeInternal()->covers(e);
    LOG_TRACE("Wholly outside way check: inverted crop and the envelope covers the element=" << result);
  }
  LOG_TRACE("Wholly outside way check result: " << result);
  return result;
}

bool MapCropper::_isWhollyOutside(const Geometry& e) const
{
  bool result = false;
  if (!_invert)
  {
    result = !_bounds->intersects(&e);
    LOG_TRACE("Wholly outside way check: non-inverted crop and the envelope intersects with the element=" << !result);
  }
  else
  {
    result = _bounds->covers(&e);
    LOG_TRACE("Wholly outside way check: inverted crop and the envelope covers the element=" << result);
  }
  LOG_TRACE("Wholly outside way check result: " << result);
  return result;
}

}
