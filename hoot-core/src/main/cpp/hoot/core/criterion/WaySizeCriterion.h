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
 * This will properly maintain the copyright information. DigitalGlobe
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2021 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef WAY_SIZE_CRITERION_H
#define WAY_SIZE_CRITERION_H

// hoot
#include <hoot/core/criterion/ElementCriterion.h>
#include <hoot/core/util/NumericComparisonType.h>

namespace hoot
{

class WaySizeCriterion : public ElementCriterion
{
public:

  static QString className() { return "hoot::WaySizeCriterion"; }

  WaySizeCriterion() = default;
  WaySizeCriterion(const int comparisonSize, const NumericComparisonType& numericComparisonType);
  virtual ~WaySizeCriterion() = default;

  /**
   * @see ElementCriterion
   */
  virtual bool isSatisfied(const ConstElementPtr& e) const override;

  virtual ElementCriterionPtr clone() override
  { return ElementCriterionPtr(new WaySizeCriterion()); }

  virtual QString getDescription() const override
  { return "Identifies that meet a size threshold"; }

  virtual QString getName() const override { return className(); }

  virtual QString getClassName() const override { return className(); }

private:

  int _comparisonSize;
  NumericComparisonType _numericComparisonType;
};

}

#endif // WAY_SIZE_CRITERION_H