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
 * @copyright Copyright (C) 2020, 2021 Maxar (http://www.maxar.com/)
 */
#ifndef RIVER_WAY_NODE_CRITERION_H
#define RIVER_WAY_NODE_CRITERION_H

// hoot
#include <hoot/core/criterion/WayNodeCriterion.h>

namespace hoot
{

class RiverWayNodeCriterion : public WayNodeCriterion
{
public:

  static QString className() { return "hoot::RiverWayNodeCriterion"; }

  RiverWayNodeCriterion();
  RiverWayNodeCriterion(ConstOsmMapPtr map);
  ~RiverWayNodeCriterion() = default;

  ElementCriterionPtr clone() override
  { return ElementCriterionPtr(new RiverWayNodeCriterion(_map)); }

  QString getDescription() const override
  { return "Identifies nodes belonging to linear bodies of water"; }
  QString getName() const override { return className(); }
  QString getClassName() const override { return className(); }
  QString toString() const override { return className(); }
};

}

#endif // RIVER_WAY_NODE_CRITERION_H