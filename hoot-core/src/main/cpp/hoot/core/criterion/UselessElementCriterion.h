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
 * @copyright Copyright (C) 2017-2023 Maxar (http://www.maxar.com/)
 */
#ifndef USELESSELEMENTCRITERION_H
#define USELESSELEMENTCRITERION_H

// hoot
#include <hoot/core/criterion/ElementCriterion.h>
#include <hoot/core/elements/ConstOsmMapConsumer.h>
#include <hoot/core/elements/OsmMap.h>

namespace hoot
{

/**
 * A criterion for detecting elements with no parents or children.
 *
 * For nodes: return true if the node has no parent ways or relations
 * For ways: return true if the way has no child nodes or parent relations
 * For relations: return true if the relation has no parent relations or members
 */
class UselessElementCriterion : public ElementCriterion, public ConstOsmMapConsumerBase
{
public:

  static QString className() { return "UselessElementCriterion"; }

  UselessElementCriterion() = default;
  UselessElementCriterion(ConstOsmMapPtr map) : ConstOsmMapConsumerBase(map) { }
  ~UselessElementCriterion() override = default;

  bool isSatisfied(const ConstElementPtr& e) const override;
  ElementCriterionPtr clone() override { return std::make_shared<UselessElementCriterion>(); }

  QString getDescription() const override { return "Identifies elements that have no use"; }
  QString getName() const override { return className(); }
  QString getClassName() const override { return className(); }
  QString toString() const override { return className(); }

};

}

#endif // USELESSELEMENTCRITERION_H
