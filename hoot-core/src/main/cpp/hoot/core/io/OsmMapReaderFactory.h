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
#ifndef OSMMAPREADERFACTORY_H
#define OSMMAPREADERFACTORY_H

// hoot
#include <hoot/core/elements/Status.h>

namespace hoot
{

class OsmMap;
class OsmMapReader;

/**
 * A factory for constructing readers based on the URL
 */
class OsmMapReaderFactory
{

public:

  static std::shared_ptr<OsmMapReader> createReader(const QString& url, bool useFileId = true, Status defaultStatus = Status::Invalid, bool cropOnReadIfBounded = true);
  // Note the url as the last param here...was getting runtime overlap between these two where
  // bools were being passed as status ints and vice versa. May need to do some more refactoring
  // here to make things cleaner.
  static std::shared_ptr<OsmMapReader> createReader(bool useFileId, bool useFileStatus, const QString& url, bool cropOnReadIfBounded = true);

  /**
   * Returns true if a partial reader is available for the given URL.
   */
  static bool supportsPartialReading(const QString& url);

  static void read(const std::shared_ptr<OsmMap>& map, const QString& url, bool useFileId = true, Status defaultStatus = Status::Invalid,
                   bool cropOnReadIfBounded = true, bool suppressLogging = false);
  // See note for createReader.
  static void read(const std::shared_ptr<OsmMap>& map, bool useFileId, bool useFileStatus, const QString& url,
                   bool cropOnReadIfBounded = true, bool suppressLogging = false);

  static QString getReaderName(const QString& url);

private:

  static void _read(const std::shared_ptr<OsmMap>& map, const std::shared_ptr<OsmMapReader>& reader, const QString& url, bool suppressLogging);

  static std::shared_ptr<OsmMapReader> _createReader(const QString& url);
};

}

#endif // OSMMAPREADERFACTORY_H
