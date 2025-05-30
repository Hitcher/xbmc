/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRProviders.h"

#include "ServiceBroker.h"
#include "pvr/PVRCachedImages.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/providers/PVRProvider.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <vector>

using namespace PVR;

bool CPVRProvidersContainer::UpdateFromClient(const std::shared_ptr<CPVRProvider>& provider)
{
  std::unique_lock lock(m_critSection);
  const std::shared_ptr<CPVRProvider> providerToUpdate =
      GetByClient(provider->GetClientId(), provider->GetUniqueId());
  if (providerToUpdate)
  {
    return providerToUpdate->UpdateEntry(provider, ProviderUpdateMode::BY_CLIENT);
  }
  else
  {
    m_iLastId++;
    provider->SetDatabaseId(m_iLastId);
    InsertEntry(provider, ProviderUpdateMode::BY_CLIENT);
  }

  return true;
}

std::shared_ptr<CPVRProvider> CPVRProvidersContainer::GetByClient(int iClientId,
                                                                  int iUniqueId) const
{
  std::unique_lock lock(m_critSection);
  const auto it = std::ranges::find_if(
      m_providers, [iClientId, iUniqueId](const auto& provider)
      { return provider->GetClientId() == iClientId && provider->GetUniqueId() == iUniqueId; });
  return it != m_providers.cend() ? (*it) : std::shared_ptr<CPVRProvider>();
}

void CPVRProvidersContainer::InsertEntry(const std::shared_ptr<CPVRProvider>& newProvider,
                                         ProviderUpdateMode updateMode)
{
  bool found = false;
  for (const auto& provider : m_providers)
  {
    if (provider->GetClientId() == newProvider->GetClientId() &&
        provider->GetUniqueId() == newProvider->GetUniqueId())
    {
      found = true;
      provider->UpdateEntry(newProvider, updateMode);
    }
  }

  if (!found)
  {
    m_providers.emplace_back(newProvider);
  }
}

std::vector<std::shared_ptr<CPVRProvider>> CPVRProvidersContainer::GetProvidersList() const
{
  std::unique_lock lock(m_critSection);
  return m_providers;
}

std::vector<std::shared_ptr<CPVRProvider>> CPVRProviders::GetProviders() const
{
  std::vector<std::shared_ptr<CPVRProvider>> providers;
  std::unique_lock lock(m_critSection);

  //! @todo optimize; get rid of iteration.
  providers.reserve(m_providers.size());
  for (const auto& provider : m_providers)
  {
    if (provider->IsClientProvider())
    {
      // Ignore non installed / disabled client providers
      const std::shared_ptr<const CPVRClient> client{
          CServiceBroker::GetPVRManager().GetClient(provider->GetClientId())};
      if (!client)
        continue;
    }
    providers.emplace_back(provider);
  }
  return providers;
}

std::size_t CPVRProviders::GetNumProviders() const
{
  std::unique_lock lock(m_critSection);
  return GetProviders().size();
}

bool CPVRProviders::Update(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  return LoadFromDatabase(clients) && UpdateFromClients(clients);
}

void CPVRProviders::Unload()
{
  // remove all tags
  std::unique_lock lock(m_critSection);
  m_providers.clear();
}

bool CPVRProviders::LoadFromDatabase(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  const std::shared_ptr<const CPVRDatabase> database =
      CServiceBroker::GetPVRManager().GetTVDatabase();
  if (database)
  {
    m_iLastId = database->GetMaxProviderId();

    CPVRProviders providers;
    database->Get(providers, clients);

    for (const auto& provider : providers.GetProvidersList())
      CheckAndAddEntry(provider, ProviderUpdateMode::BY_DATABASE);
  }
  return true;
}

bool CPVRProviders::UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  {
    std::unique_lock lock(m_critSection);
    if (m_bIsUpdating)
      return false;
    m_bIsUpdating = true;
  }

  // Default client providers are the add-ons themselves, we retrieve both enabled
  // and disabled add-ons as we don't want them removed from the DB
  CPVRProviders newAddonProviderList;
  std::vector<int> disabledClients;
  std::vector<CVariant> clientProviderInfos =
      CServiceBroker::GetPVRManager().Clients()->GetClientProviderInfos();
  CLog::LogFC(LOGDEBUG, LOGPVR, "Adding default providers, found {} PVR add-ons",
              clientProviderInfos.size());
  for (const auto& clientInfo : clientProviderInfos)
  {
    auto addonProvider = std::make_shared<CPVRProvider>(
        clientInfo["clientid"].asInteger32(), clientInfo["fullname"].asString(),
        clientInfo["icon"].asString(), clientInfo["thumb"].asString());

    newAddonProviderList.CheckAndAddEntry(addonProvider, ProviderUpdateMode::BY_CLIENT);

    if (!clientInfo["enabled"].asBoolean())
      disabledClients.emplace_back(clientInfo["clientid"].asInteger32());
  }
  UpdateDefaultEntries(newAddonProviderList);

  // Client providers are retrieved from the clients
  CLog::LogFC(LOGDEBUG, LOGPVR, "Updating providers");
  CPVRProvidersContainer newProviderList;
  std::vector<int> failedClients;
  CServiceBroker::GetPVRManager().Clients()->GetProviders(clients, &newProviderList, failedClients);
  return UpdateClientEntries(newProviderList, failedClients, disabledClients);
}

bool CPVRProviders::UpdateDefaultEntries(const CPVRProvidersContainer& newProviders)
{
  bool bChanged = false;

  std::unique_lock lock(m_critSection);

  // go through the provider list and check for updated or new providers
  const auto newProviderList = newProviders.GetProvidersList();
  bChanged = std::accumulate(
      newProviderList.cbegin(), newProviderList.cend(), false,
      [this](bool changed, const auto& newProvider) {
        return (CheckAndPersistEntry(newProvider, ProviderUpdateMode::BY_CLIENT) != nullptr)
                   ? true
                   : changed;
      });

  // check for deleted providers
  for (auto it = m_providers.cbegin(); it != m_providers.cend();)
  {
    const std::shared_ptr<const CPVRProvider> provider = *it;
    if (!newProviders.GetByClient(provider->GetClientId(), provider->GetUniqueId()))
    {
      // provider was not found
      bool bIgnoreProvider = false;

      // ignore add-on any providers that are no PVR Client addon providers
      if (!provider->IsClientProvider())
        bIgnoreProvider = true;

      if (bIgnoreProvider)
      {
        ++it;
        continue;
      }

      CLog::LogFC(LOGDEBUG, LOGPVR, "Deleted provider {} on client {}", provider->GetUniqueId(),
                  provider->GetClientId());

      (*it)->DeleteFromDatabase();
      it = m_providers.erase(it);

      bChanged |= true;
    }
    else
    {
      ++it;
    }
  }

  return bChanged;
}

bool CPVRProviders::UpdateClientEntries(const CPVRProvidersContainer& newProviders,
                                        const std::vector<int>& failedClients,
                                        const std::vector<int>& disabledClients)
{
  std::unique_lock lock(m_critSection);

  // go through the provider list and check for updated or new providers
  for (const auto& newProvider : newProviders.GetProvidersList())
  {
    CheckAndPersistEntry(newProvider, ProviderUpdateMode::BY_CLIENT);
  }

  // check for deleted providers
  for (auto it = m_providers.begin(); it != m_providers.end();)
  {
    const std::shared_ptr<const CPVRProvider> provider = *it;
    if (!newProviders.GetByClient(provider->GetClientId(), provider->GetUniqueId()))
    {
      const bool bIgnoreProvider =
          (provider->IsClientProvider() || // ignore add-on providers as they are a special case
           std::ranges::any_of(failedClients, [&provider](const auto& failedClient)
                               { return failedClient == provider->GetClientId(); }) ||
           std::ranges::any_of(disabledClients, [&provider](const auto& disabledClient)
                               { return disabledClient == provider->GetClientId(); }));
      if (bIgnoreProvider)
      {
        ++it;
        continue;
      }

      CLog::LogFC(LOGDEBUG, LOGPVR, "Deleted provider {} on client {}", provider->GetUniqueId(),
                  provider->GetClientId());

      (*it)->DeleteFromDatabase();
      it = m_providers.erase(it);
    }
    else
    {
      ++it;
    }
  }

  m_bIsUpdating = false;

  return true;
}

std::shared_ptr<CPVRProvider> CPVRProviders::CheckAndAddEntry(
    const std::shared_ptr<CPVRProvider>& newProvider, ProviderUpdateMode updateMode)
{
  bool bChanged = false;

  std::unique_lock lock(m_critSection);
  std::shared_ptr<CPVRProvider> provider =
      GetByClient(newProvider->GetClientId(), newProvider->GetUniqueId());
  if (provider)
  {
    bChanged = provider->UpdateEntry(newProvider, updateMode);
  }
  else
  {
    // We don't set an id as this came from the DB so it has one already
    InsertEntry(newProvider, updateMode);

    if (newProvider->GetDatabaseId() > m_iLastId)
      m_iLastId = newProvider->GetDatabaseId();

    provider = newProvider;
    bChanged = true;
  }

  if (bChanged)
    return provider;

  return {};
}

std::shared_ptr<CPVRProvider> CPVRProviders::CheckAndPersistEntry(
    const std::shared_ptr<CPVRProvider>& newProvider, ProviderUpdateMode updateMode)
{
  bool bChanged = false;

  std::unique_lock lock(m_critSection);
  std::shared_ptr<CPVRProvider> provider =
      GetByClient(newProvider->GetClientId(), newProvider->GetUniqueId());
  if (provider)
  {
    bChanged = provider->UpdateEntry(newProvider, updateMode);

    if (bChanged)
      provider->Persist(true);

    CLog::LogFC(LOGDEBUG, LOGPVR, "Updated provider {} on client {}", newProvider->GetUniqueId(),
                newProvider->GetClientId());
  }
  else
  {
    m_iLastId++;
    newProvider->SetDatabaseId(m_iLastId);
    InsertEntry(newProvider, updateMode);

    newProvider->Persist();

    CLog::LogFC(LOGDEBUG, LOGPVR, "Added provider {} on client {}", newProvider->GetUniqueId(),
                newProvider->GetClientId());

    provider = newProvider;
    bChanged = true;
  }

  if (bChanged)
    return provider;

  return {};
}

bool CPVRProviders::PersistUserChanges(
    const std::vector<std::shared_ptr<CPVRProvider>>& providers) const
{
  for (const auto& provider : providers)
  {
    provider->Persist(true);

    CLog::LogFC(LOGDEBUG, LOGPVR, "Updated provider {} on client {}", provider->GetUniqueId(),
                provider->GetClientId());
  }

  return true;
}

std::shared_ptr<CPVRProvider> CPVRProviders::GetById(int iProviderId) const
{
  std::unique_lock lock(m_critSection);
  const auto it = std::ranges::find_if(m_providers, [iProviderId](const auto& provider)
                                       { return provider->GetDatabaseId() == iProviderId; });
  return it != m_providers.cend() ? (*it) : std::shared_ptr<CPVRProvider>();
}

void CPVRProviders::RemoveEntry(const std::shared_ptr<CPVRProvider>& provider)
{
  std::unique_lock lock(m_critSection);

  m_providers.erase(
      std::remove_if(m_providers.begin(), m_providers.end(),
                     [&provider](const std::shared_ptr<const CPVRProvider>& providerToRemove) {
                       return provider->GetClientId() == providerToRemove->GetClientId() &&
                              provider->GetUniqueId() == providerToRemove->GetUniqueId();
                     }),
      m_providers.end());
}

int CPVRProviders::CleanupCachedImages()
{
  std::vector<std::string> urlsToCheck;
  {
    std::unique_lock lock(m_critSection);

    for (const auto& provider : m_providers)
    {
      urlsToCheck.emplace_back(provider->GetClientIconPath());
      urlsToCheck.emplace_back(provider->GetClientThumbPath());
    }
  }

  static const std::vector<PVRImagePattern> urlPatterns = {{CPVRProvider::IMAGE_OWNER_PATTERN, ""}};
  return CPVRCachedImages::Cleanup(urlPatterns, urlsToCheck);
}
