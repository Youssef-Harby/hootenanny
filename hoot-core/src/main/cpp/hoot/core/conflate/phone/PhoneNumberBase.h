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
 * @copyright Copyright (C) 2023 Maxar (http://www.maxar.com/)
 */

#ifndef PHONE_NUMBER_BASE_H
#define PHONE_NUMBER_BASE_H

#include <mutex>

namespace hoot
{

/**
 * Phone number base object with mutex to keep libphonenumber calls reentrant
 */
class PhoneNumberBase
{
protected:

  PhoneNumberBase() = default;
  ~PhoneNumberBase() = default;
  /** Shared mutex used for calls to libphonenumber */
  std::mutex& getPhoneNumberMutex() const
  {
    static std::mutex m;
    return m;
  }
};

}

#endif // PHONE_NUMBER_BASE_H