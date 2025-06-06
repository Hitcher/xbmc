/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "pvr/PVRCachedImage.h"
#include "threads/CriticalSection.h"
#include "utils/ISerializable.h"

#include <memory>
#include <string>
#include <vector>

namespace EDL
{
struct Edit;
}

struct EPG_TAG;

namespace PVR
{
class CPVREpgChannelData;
class CPVREpgDatabase;

class CPVREpgInfoTag final : public ISerializable,
                             public std::enable_shared_from_this<CPVREpgInfoTag>
{
  friend class CPVREpgDatabase;

public:
  static const std::string IMAGE_OWNER_PATTERN;

  /*!
   * @brief Create a new EPG infotag.
   * @param data The tag's data.
   * @param iClientId The client id.
   * @param channelData The channel data.
   * @param iEpgId The id of the EPG this tag belongs to.
   */
  CPVREpgInfoTag(const EPG_TAG& data,
                 int iClientId,
                 const std::shared_ptr<CPVREpgChannelData>& channelData,
                 int iEpgID);

  /*!
   * @brief Create a new EPG infotag.
   * @param channelData The channel data.
   * @param iEpgId The id of the EPG this tag belongs to.
   * @param start The start time of the event
   * @param end The end time of the event
   * @param bIsGapTagTrue if this is a "gap" tag, false if this is a real EPG event
   */
  CPVREpgInfoTag(const std::shared_ptr<CPVREpgChannelData>& channelData,
                 int iEpgID,
                 const CDateTime& start,
                 const CDateTime& end,
                 bool bIsGapTag);

  /*!
   * @brief Set data for the channel linked to this EPG infotag.
   * @param data The channel data.
   */
  void SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data);

  bool operator==(const CPVREpgInfoTag& right) const;

  // ISerializable implementation
  void Serialize(CVariant& value) const override;

  /*!
   * @brief Get the identifier of the client that serves this event.
   * @return The identifier.
   */
  int ClientID() const;

  /*!
   * @brief Check if this event is currently active.
   * @return True if it's active, false otherwise.
   */
  bool IsActive() const;

  /*!
   * @brief Check if this event is in the past.
   * @return True when this event has already passed, false otherwise.
   */
  bool WasActive() const;

  /*!
   * @brief Check if this event is in the future.
   * @return True when this event is an upcoming event, false otherwise.
   */
  bool IsUpcoming() const;

  /*!
   * @brief Get the progress of this tag in percent.
   * @return The current progress of this tag.
   */
  double ProgressPercentage() const;

  /*!
   * @brief Get the progress of this tag in seconds.
   * @return The current progress of this tag in seconds.
   */
  unsigned int Progress() const;

  /*!
   * @brief Get EPG ID of this tag.
   * @return The epg ID.
   */
  int EpgID() const;

  /*!
   * @brief Sets the EPG id for this event.
   * @param iEpgID The EPG id.
   */
  void SetEpgID(int iEpgID);

  /*!
   * @brief Change the unique broadcast ID of this event.
   * @param iUniqueBroadcastId The new unique broadcast ID.
   */
  void SetUniqueBroadcastID(unsigned int iUniqueBroadcastID);

  /*!
   * @brief Get the unique broadcast ID.
   * @return The unique broadcast ID.
   */
  unsigned int UniqueBroadcastID() const;

  /*!
   * @brief Get the event's database ID.
   * @return The database ID.
   */
  int DatabaseID() const;

  /*!
   * @brief Get the unique ID of the channel associated with this event.
   * @return The unique channel ID.
   */
  int UniqueChannelID() const;

  /*!
   * @brief Get the path for the icon of the channel associated with this event.
   * @return The channel icon path.
   */
  std::string ChannelIconPath() const;

  /*!
   * @brief Get the event's start time.
   * @return The start time in UTC.
   */
  CDateTime StartAsUTC() const;

  /*!
   * @brief Get the event's start time.
   * @return The start time as local time.
   */
  CDateTime StartAsLocalTime() const;

  /*!
   * @brief Get the event's end time.
   * @return The end time in UTC.
   */
  CDateTime EndAsUTC() const;

  /*!
   * @brief Get the event's end time.
   * @return The end time as local time.
   */
  CDateTime EndAsLocalTime() const;

  /*!
   * @brief Change the event's end time.
   * @param end The new end time.
   */
  void SetEndFromUTC(const CDateTime& end);

  /*!
   * @brief Get the duration of this event in seconds.
   * @return The duration.
   */
  unsigned int GetDuration() const;

  /*!
   * @brief Get the title of this event.
   * @return The title.
   */
  const std::string& Title() const { return m_strTitle; }

  /*!
  * @brief Get the title extra information of this event.
  * @return The title extra info.
  */
  const std::string& TitleExtraInfo() const { return m_titleExtraInfo; }

  /*!
   * @brief Get the plot outline of this event.
   * @return The plot outline.
   */
  const std::string& PlotOutline() const { return m_strPlotOutline; }

  /*!
   * @brief Get the plot of this event.
   * @return The plot.
   */
  const std::string& Plot() const { return m_strPlot; }

  /*!
   * @brief Get the original title of this event.
   * @return The original title.
   */
  const std::string& OriginalTitle() const { return m_strOriginalTitle; }

  /*!
   * @brief Get the cast of this event.
   * @return The cast.
   */
  const std::vector<std::string>& Cast() const { return m_cast; }

  /*!
   * @brief Get the director(s) of this event.
   * @return The director(s).
   */
  const std::vector<std::string>& Directors() const { return m_directors; }

  /*!
   * @brief Get the writer(s) of this event.
   * @return The writer(s).
   */
  const std::vector<std::string>& Writers() const { return m_writers; }

  /*!
   * @brief Get the cast members of this event as formatted string.
   * @param separator The separator for the different cast members, default value if empty.
   * @return The cast label.
   */
  std::string GetCastLabel(const std::string& separator) const;

  /*!
   * @brief Get the director(s) of this event as formatted string.
   * @param separator The separator for the different directors, default value if empty.
   * @return The directors label.
   */
  std::string GetDirectorsLabel(const std::string& separator) const;

  /*!
   * @brief Get the writer(s) of this event as formatted string.
   * @param separator The separator for the different writers, default value if empty.
   * @return The writers label.
   */
  std::string GetWritersLabel(const std::string& separator) const;

  /*!
   * @brief Get the genre(s) of this event as formatted string.
   * @param separator The separator for the different genres, default value if empty.
   * @return The genres label.
   */
  std::string GetGenresLabel(const std::string& separator) const;

  /*!
   * @brief Get the year of this event.
   * @return The year.
   */
  int Year() const;

  /*!
   * @brief Get the imdbnumber of this event.
   * @return The imdbnumber.
   */
  const std::string& IMDBNumber() const { return m_strIMDBNumber; }

  /*!
   * @brief Get the genre type ID of this event.
   * @return The genre type ID.
   */
  int GenreType() const;

  /*!
   * @brief Get the genre subtype ID of this event.
   * @return The genre subtype ID.
   */
  int GenreSubType() const;

  /*!
   * @brief Get the genre description of this event.
   * @return The genre.
   */
  const std::string& GenreDescription() const { return m_strGenreDescription; }

  /*!
   * @brief Get the genre as human readable string.
   * @return The genre.
   */
  const std::vector<std::string> Genre() const;

  /*!
   * @brief Get the first air date of this event.
   * @return The first air date.
   */
  CDateTime FirstAired() const;

  /*!
   * @brief Get the parental rating of this event.
   * @return The parental rating.
   */
  unsigned int ParentalRating() const;

  /*!
   * @brief Get the parental rating code of this event.
   * @return The parental rating code.
   */
  const std::string& ParentalRatingCode() const { return m_parentalRatingCode; }

  /*!
   * @brief Get the parental rating icon path of this event.
   * @return Path to the parental rating icon.
   */
  const std::string& ParentalRatingIcon() const { return m_parentalRatingIcon.GetLocalImage(); }

  /*!
   * @brief Get the parental rating icon path of this event as given by the client.
   * @return The path to the icon
   */
  std::string ClientParentalRatingIconPath() const { return m_parentalRatingIcon.GetClientImage(); }

  /*!
   * @brief Get the parental rating source of this event.
   * @return The parental rating source.
   */
  const std::string& ParentalRatingSource() const { return m_parentalRatingSource; }
  /*!
   * @brief Get the star rating of this event.
   * @return The star rating.
   */
  int StarRating() const;

  /*!
   * @brief The series number of this event.
   * @return The series number.
   */
  int SeriesNumber() const;

  /*!
   * @brief The series link for this event.
   * @return The series link or empty string, if not available.
   */
  const std::string& SeriesLink() const { return m_strSeriesLink; }

  /*!
   * @brief The episode number of this event.
   * @return The episode number.
   */
  int EpisodeNumber() const;

  /*!
   * @brief The episode part number of this event.
   * @return The episode part number.
   */
  int EpisodePart() const;

  /*!
   * @brief The episode name of this event.
   * @return The episode name.
   */
  const std::string& EpisodeName() const { return m_strEpisodeName; }

  /*!
   * @brief Get the path to the icon for this event used by Kodi.
   * @return The path to the icon
   */
  std::string IconPath() const;

  /*!
   * @brief Get the path to the icon for this event as given by the client.
   * @return The path to the icon
   */
  std::string ClientIconPath() const;

  /*!
   * @brief The path to this event.
   * @return The path.
   */
  std::string Path() const;

  /*!
   * @brief Check if this event can be recorded.
   * @return True if it can be recorded, false otherwise.
   */
  bool IsRecordable() const;

  /*!
   * @brief Check if this event can be played.
   * @return True if it can be played, false otherwise.
   */
  bool IsPlayable() const;

  /*!
   * @brief Write query to persist this tag in the query queue of the given database.
   * @param database The database.
   * @return True on success, false otherwise.
   */
  bool QueuePersistQuery(const std::shared_ptr<CPVREpgDatabase>& database) const;

  /*!
   * @brief Update the information in this tag with the info in the given tag.
   * @param tag The new info.
   * @param bUpdateBroadcastId If set to false, the tag BroadcastId (locally unique) will not be
   * checked/updated
   * @return True if something changed, false otherwise.
   */
  bool Update(const CPVREpgInfoTag& tag, bool bUpdateBroadcastId = true);

  /*!
   * @brief Retrieve the edit decision list (EDL) of an EPG tag.
   * @return The edit decision list (empty on error)
   */
  std::vector<EDL::Edit> GetEdl() const;

  /*!
   * @brief Check whether this tag has any series attributes.
   * @return True if this tag has any series attributes, false otherwise
   */
  bool IsSeries() const;

  /*!
   * @brief Check whether this tag is associated with a radion or TV channel.
   * @return True if this tag is associated with a radio channel, false otherwise.
   */
  bool IsRadio() const;

  /*!
   * @brief Check whether this event is parental locked.
   * @return True if whether this event is parental locked, false otherwise.
   */
  bool IsParentalLocked() const;

  /*!
   * @brief Check whether this event is a real event or a gap in the EPG timeline.
   * @return True if this event is a gap, false otherwise.
   */
  bool IsGapTag() const;

  /*!
   * @brief Check whether this tag will be flagged as new.
   * @return True if this tag will be flagged as new, false otherwise
   */
  bool IsNew() const;

  /*!
   * @brief Check whether this tag will be flagged as a premiere.
   * @return True if this tag will be flagged as a premiere, false otherwise
   */
  bool IsPremiere() const;

  /*!
   * @brief Check whether this tag will be flagged as a finale.
   * @return True if this tag will be flagged as a finale, false otherwise
   */
  bool IsFinale() const;

  /*!
   * @brief Check whether this tag will be flagged as live.
   * @return True if this tag will be flagged as live, false otherwise
   */
  bool IsLive() const;

  /*!
   * @brief Return the flags (EPG_TAG_FLAG_*) of this event as a bitfield.
   * @return the flags.
   */
  unsigned int Flags() const { return m_iFlags; }

  /*!
   * @brief Split the given string into tokens. Interprets occurrences of EPG_STRING_TOKEN_SEPARATOR
   * in the string as separator.
   * @param str The string to tokenize.
   * @return the tokens.
   */
  static std::vector<std::string> Tokenize(const std::string& str);

  /*!
   * @brief Combine the given strings to a single string. Inserts EPG_STRING_TOKEN_SEPARATOR as
   * separator.
   * @param tokens The tokens.
   * @return the combined string.
   */
  static std::string DeTokenize(const std::vector<std::string>& tokens);

private:
  CPVREpgInfoTag(int iEpgID,
                 const std::string& iconPath,
                 const std::string& parentalRatingIconPath);

  CPVREpgInfoTag() = delete;
  CPVREpgInfoTag(const CPVREpgInfoTag& tag) = delete;
  CPVREpgInfoTag& operator=(const CPVREpgInfoTag& other) = delete;

  /*!
     * @brief Get current time, taking timeshifting into account.
     * @return The playing time.
     */
  CDateTime GetCurrentPlayingTime() const;

  int m_iDatabaseID = -1; /*!< database ID */
  int m_iGenreType = 0; /*!< genre type */
  int m_iGenreSubType = 0; /*!< genre subtype */
  std::string m_strGenreDescription; /*!< genre description */
  unsigned int m_parentalRating = 0; /*!< parental rating */
  std::string m_parentalRatingCode; /*!< Parental rating code */
  CPVRCachedImage m_parentalRatingIcon; /*!< parental rating icon path */
  std::string m_parentalRatingSource; /*!< parental rating source */
  int m_iStarRating = 0; /*!< star rating */
  int m_iSeriesNumber = -1; /*!< series number */
  int m_iEpisodeNumber = -1; /*!< episode number */
  int m_iEpisodePart = -1; /*!< episode part number */
  unsigned int m_iUniqueBroadcastID = 0; /*!< unique broadcast ID */
  std::string m_strTitle; /*!< title */
  std::string m_titleExtraInfo; /*!< title extra info */
  std::string m_strPlotOutline; /*!< plot outline */
  std::string m_strPlot; /*!< plot */
  std::string m_strOriginalTitle; /*!< original title */
  std::vector<std::string> m_cast; /*!< cast */
  std::vector<std::string> m_directors; /*!< director(s) */
  std::vector<std::string> m_writers; /*!< writer(s) */
  int m_iYear = 0; /*!< year */
  std::string m_strIMDBNumber; /*!< imdb number */
  mutable std::vector<std::string> m_genre; /*!< genre */
  std::string m_strEpisodeName; /*!< episode name */
  CPVRCachedImage m_iconPath; /*!< the path to the icon */
  CDateTime m_startTime; /*!< event start time */
  CDateTime m_endTime; /*!< event end time */
  CDateTime m_firstAired; /*!< first airdate */
  unsigned int m_iFlags = 0; /*!< the flags applicable to this EPG entry */
  std::string m_strSeriesLink; /*!< series link */
  bool m_bIsGapTag = false;

  mutable CCriticalSection m_critSection;
  std::shared_ptr<CPVREpgChannelData> m_channelData;
  int m_iEpgID = -1;
};
} // namespace PVR
