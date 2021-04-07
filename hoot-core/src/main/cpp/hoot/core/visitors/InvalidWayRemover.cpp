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
 * @copyright Copyright (C) 2021 Maxar (http://www.maxar.com/)
 */

#include "InvalidWayRemover.h"

// hoot
#include <hoot/core/criterion/EmptyWayCriterion.h>
#include <hoot/core/criterion/LinearCriterion.h>
#include <hoot/core/criterion/ZeroLengthWayCriterion.h>
#include <hoot/core/ops/RemoveWayByEid.h>
#include <hoot/core/util/Factory.h>

namespace hoot
{

HOOT_FACTORY_REGISTER(ElementVisitor, InvalidWayRemover)

void InvalidWayRemover::visit(const ElementPtr& e)
{
  if (!e)
  {
    return;
  }
  else if (_conflateInfoCache &&
           !_conflateInfoCache->elementCanBeConflatedByActiveMatcher(e, className()))
  {
    LOG_TRACE(
      "Skipping processing of " << e->getElementId() << " as it cannot be conflated by any " <<
      "actively configured conflate matcher.");
    return;
  }

  if (EmptyWayCriterion().isSatisfied(e) || ZeroLengthWayCriterion().isSatisfied(e))
  {
    RemoveWayByEid::removeWayFully(_map->shared_from_this(), e->getId());
  }
}

QStringList InvalidWayRemover::getCriteria() const
{
  return QStringList(LinearCriterion::className());
}

}