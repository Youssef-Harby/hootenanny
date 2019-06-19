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
 * @copyright Copyright (C) 2019 DigitalGlobe (http://www.digitalglobe.com/)
 */

#ifndef IDSWAPOP_H
#define IDSWAPOP_H

#include <hoot/core/elements/OsmMap.h>
#include <hoot/core/info/OperationStatusInfo.h>
#include <hoot/core/ops/OsmMapOperation.h>

namespace hoot
{

class IdSwapOp : public OsmMapOperation, public OperationStatusInfo
{
public:
  /**
   * @brief className - Get classname string
   * @return - "hoot::IdSwapOp"
   */
  static std::string className() { return "hoot::IdSwapOp"; }

  /**
   * @brief IdSwapOp - Default constructor
   */
  IdSwapOp();

  /**
   * @brief apply - Apply the IdSwap op
   * @param pMap - Target map
   */
  virtual void apply(std::shared_ptr<OsmMap>& map) override;

  virtual QString getDescription() const override
  { return "Swap IDs for ID preservation in Attribute Conflation"; }

  virtual QString getInitStatusMessage() const override
  { return "Swapping IDs..."; }

  virtual QString getCompletedStatusMessage() const override
  { return "Swapped " + QString::number(_numAffected) + " IDs."; }
};

}

#endif  //  IDSWAPOP_H