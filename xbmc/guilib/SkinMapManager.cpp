/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinMapManager.h"

#include "utils/log.h"

#include <tinyxml.h>

using namespace KODI::GUILIB;

void CSkinMapManager::Clear()
{
  m_maps.clear();
  m_refs.clear();
}

void CSkinMapManager::LoadMaps(const TiXmlElement* node)
{
  if (!node)
    return;

  const TiXmlElement* mapElement = node->FirstChildElement("map");
  while (mapElement)
  {
    const char* name = mapElement->Attribute("name");
    if (name && *name)
    {
      const char* ref = mapElement->Attribute("ref");
      if (ref && *ref)
      {
        // reference to another map — store the alias
        CLog::LogF(LOGDEBUG, "Skin map '{}' references map '{}'", name, ref);
        m_refs.insert_or_assign(name, ref);
      }
      else
      {
        std::unordered_map<std::string, std::string> entries;

        const TiXmlElement* entry = mapElement->FirstChildElement("entry");
        while (entry)
        {
          const char* key = entry->Attribute("key");
          const TiXmlNode* textNode = entry->FirstChild();
          if (key && *key && textNode && textNode->Type() == TiXmlNode::TINYXML_TEXT)
            entries.try_emplace(key, textNode->ValueStr());

          entry = entry->NextSiblingElement("entry");
        }

        if (!entries.empty())
        {
          CLog::LogF(LOGDEBUG, "Loaded skin map '{}' with {} entries", name, entries.size());
          m_maps.insert_or_assign(name, std::move(entries));
        }
        else
        {
          CLog::LogF(LOGWARNING, "Skin map '{}' has no valid entries, skipping", name);
        }
      }
    }

    mapElement = mapElement->NextSiblingElement("map");
  }
}

std::string CSkinMapManager::Lookup(const std::string& mapName, const std::string& key) const
{
  // resolve ref alias if present
  const std::string* resolvedName = &mapName;
  const auto refIt = m_refs.find(mapName);
  if (refIt != m_refs.end())
    resolvedName = &refIt->second;

  const auto mapIt = m_maps.find(*resolvedName);
  if (mapIt == m_maps.end())
    return key; // no map defined — return raw value unchanged

  const auto entryIt = mapIt->second.find(key);
  if (entryIt == mapIt->second.end())
    return key; // key not in map — return raw value unchanged

  return entryIt->second;
}

bool CSkinMapManager::HasMap(const std::string& mapName) const
{
  if (m_refs.count(mapName) > 0)
    return true;
  return m_maps.count(mapName) > 0;
}
