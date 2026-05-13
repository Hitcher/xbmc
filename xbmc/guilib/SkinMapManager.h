/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

class TiXmlElement;

namespace KODI::GUILIB
{

/*!
 \brief Manages skin-defined label maps loaded from Maps.xml (or per-infolabel XML files).

 Skin authors can define a <map> element in their Includes.xml (or a separate Maps.xml) with a
 unique name, containing <entry key="rawValue">Display Value</entry> children.  At runtime,
 $MAP[infolabel] resolves the infolabel to its current string value and looks it up in the
 default map named after the infolabel (e.g. "ListItem.AudioCodec"), while
 $MAP[MapName,infolabel] uses a named map, allowing multiple mappings for the same infolabel.

 Example XML (inside <includes> or a separate file referenced by <include file="Maps.xml"/>):
 \code{.xml}
 <map name="ListItem.AudioCodec">
   <entry key="ac3">Dolby Digital</entry>
   <entry key="eac3">Dolby Digital+</entry>
   <entry key="dtshd_ma">DTS-HD MA</entry>
   <entry key="flac">FLAC</entry>
 </map>

 <map name="AudioCodecShort">
   <entry key="ac3">AC3</entry>
   <entry key="eac3">DD+</entry>
   <entry key="dtshd_ma">DTS-MA</entry>
   <entry key="flac">FLAC</entry>
 </map>
 \endcode
*/
class CSkinMapManager
{
public:
  CSkinMapManager() = default;
  ~CSkinMapManager() = default;

  /*! \brief Clear all loaded maps */
  void Clear();

  /*! \brief Load all <map> elements from the given XML node (root of an includes file) */
  void LoadMaps(const TiXmlElement* node);

  /*! \brief Look up a value in the named map.
   \param mapName  the map to search (either an infolabel name or a skin-defined map name)
   \param key      the raw infolabel value to look up
   \return the mapped display string, or \p key unchanged if no mapping is found
  */
  std::string Lookup(const std::string& mapName, const std::string& key) const;

  /*! \brief Returns true if a map with the given name has been loaded */
  bool HasMap(const std::string& mapName) const;

private:
  // outer key = map name, inner key = raw infolabel value, value = display string
  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::string>>
      m_maps;
};

} // namespace KODI::GUILIB
