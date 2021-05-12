/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (C) 2021 Christos Laskos
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FTM_HEADER_H_
#define FTM_HEADER_H_

#include "ns3/header.h"
#include "ns3/object.h"

namespace ns3 {

/**
 * \brief Class for the FTM parameters.
 * \ingroup FTM
 *
 * This Class implements the FTM parameter header.
 */
class FtmParams : public Header
{
public:
  FtmParams ();
  virtual ~FtmParams ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  /**
   * Status Indication types.
   */
  enum StatusIndication : uint8_t
  {
    RESERVED = 0,
    SUCCESSFUL = 1,
    REQUEST_INCAPABLE = 2,
    REQUEST_FAILED = 3
  };

  /**
   * Set the StatusIndication field in the FTM parameters header.
   *
   * \param status the StatusIndication field value
   */
  void SetStatusIndication (StatusIndication status);

  /**
   * Return the StatusIndication field.
   *
   * \return the StatusIndication field
   */
  StatusIndication GetStatusIndication (void);

  /**
   * Set the status indication value field.
   *
   * \param value the status indication value
   */
  void SetStatusIndicationValue (uint8_t value);

  /**
   * Returns the status indication value field.
   *
   * \return the status indication value
   */
  uint8_t GetStatusIndicationValue (void);

  /**
   * Set the number of bursts exponent field.
   *
   * \param burst_exponent the burst exponent
   */
  void SetNumberOfBurstsExponent (uint8_t burst_exponent);

  /**
   * Returns the number of bursts exponent field.
   *
   * \return the number of bursts exponent
   */
  uint8_t GetNumberOfBurstsExponent (void);

  /**
   * Set the burst duration field.
   *
   * \param burst_duration the burst duration
   */
  void SetBurstDuration (uint8_t burst_duration);

  /**
   * Returns the burst duration field.
   *
   * \return the burst duration
   */
  uint8_t GetBurstDuration (void);

  /**
   * Set the min delta FTM field.
   *
   * \param min_delta_ftm the min delta FTM
   */
  void SetMinDeltaFtm (uint8_t min_delta_ftm);

  /**
   * Returns the min delta FTM field.
   *
   * \return the min delta FTM
   */
  uint8_t GetMinDeltaFtm (void);

  /**
   * Set the partial TSF timer field.
   *
   * \param partial_tsf_timer the partial TSF timer
   */
  void SetPartialTsfTimer (uint16_t partial_tsf_timer);

  /**
   * Returns the partial TSF timer field.
   *
   * \return the partial TSF timer
   */
  uint16_t GetPartialTsfTimer (void);

  /**
   * Set the partial TSF no pref field.
   *
   * \param tsf_no_pref the TSF no pref
   */
  void SetPartialTsfNoPref (bool tsf_no_pref);

  /**
   * Returns the partial TSF no pref field.
   *
   * \return the partial TSF no pref
   */
  bool GetPartialTsfNoPref (void);

  /**
   * Set the ASAP capable field.
   *
   * \param asap_capable the ASAP capable
   */
  void SetAsapCapable (bool asap_capable);

  /**
   * Returns the ASAP capable field.
   *
   * \return the ASAP capable
   */
  bool GetAsapCapable (void);

  /**
   * Set the ASAP field.
   *
   * \param asap the ASAP
   */
  void SetAsap (bool asap);

  /**
   * Returns the ASAP field.
   *
   * \return the ASAP
   */
  bool GetAsap (void);

  /**
   * Set the FTMs per burst field.
   *
   * \param ftms_per_burst the FTMs per burst
   */
  void SetFtmsPerBurst (uint8_t ftms_per_burst);

  /**
   * Returns the FTMs per burst field.
   *
   * \return the FTMs per burst
   */
  uint8_t GetFtmsPerBurst (void);

  /**
   * Set the format and bandwidth field. This is not used in the implementation.
   *
   * \param format_bandwidth the format and bandwidth
   */
  void SetFormatAndBandwidth (uint8_t format_bandwidth);

  /**
   * Returns the format and bandwidth field. This not used in the implementation.
   *
   * \return the format and bandwidth
   */
  uint8_t GetFormatAndBandwidth (void);

  /**
   * Set the burst period field.
   *
   * \param burst_period the burst period
   */
  void SetBurstPeriod (uint16_t burst_period);

  /**
   * Returns the burst duration field.
   *
   * \return the burst duration
   */
  uint16_t GetBurstPeriod (void);

  /**
   * Returns the burst duration in micro seconds. Used to convert the header value into a Time value.
   * Details for the conversation can be found in table 9-257 in the 802.11-2016 standard.
   *
   * \return the decoded burst duration
   */
  uint32_t DecodeBurstDuration (void);

private:
  /**
   * Convert the integer value of the status indication to the StatusIndication enum.
   *
   * \param status int representation of the status indication
   *
   * \return the status indication as the enum
   */
  StatusIndication IntToStatusIndication (uint8_t status);

  static const uint8_t m_element_id = 206; //!< Element ID for the header
  static const uint8_t m_tag_length = 9; //!< Tag length for the header

  StatusIndication m_status_indication;
  uint8_t m_status_indication_value;
  uint8_t m_number_of_bursts_exponent;
  uint8_t m_burst_duration;
  uint8_t m_min_delta_ftm;
  uint16_t m_partial_tsf_timer;
  bool m_partial_tsf_no_pref;
  bool m_asap_capable;
  bool m_asap;
  uint8_t m_ftms_per_burst;
  uint8_t m_format_and_bandwidth;
  uint16_t m_burst_period;
};

/**
 * \brief holder class for an FtmParams header
 * \ingroup FTM
 *
 * This class only purpose is to be used in the attribute system to set the default FTM parameters
 * for the FtmSession class. This was created because creating an attribute value from a header did not work.
 */
class FtmParamsHolder : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  FtmParamsHolder ();
   virtual ~FtmParamsHolder ();

   /**
    * Set the FTM parameters.
    *
    * \param params the FtmParams to set
    */
   void SetFtmParams (FtmParams params);

   /**
    * Get the FTM parameters.
    *
    * \return the FtmParams
    */
   FtmParams GetFtmParams (void);

private:
   FtmParams m_ftm_params; //!< FtmParams
};

/**
 * \brief Class for the FTM request header.
 * \ingroup FTM
 *
 * Implementation of the FTM request header.
 */
class FtmRequestHeader : public Header
{
public:
  FtmRequestHeader ();
  virtual
  ~FtmRequestHeader ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  /**
   * Set the Trigger field.
   *
   * \param trigger the trigger
   */
  void SetTrigger (uint8_t trigger);

  /**
   * Returns the trigger field.
   *
   * \return the trigger
   */
  uint8_t GetTrigger (void);

  /**
   * Set the FTM parameters.
   *
   * \param ftm_params the FtmParams
   */
  void SetFtmParams (FtmParams ftm_params);

  /**
   * Returns the FTM parameters, if set.
   *
   * \return the FtmParams
   */
  FtmParams GetFtmParams (void);

  /**
   * Returns true if the FTM parameters have been set.
   *
   * \return if FtmParams have been set
   */
  bool GetFtmParamsSet (void);

private:
  uint8_t m_trigger;
  FtmParams m_ftm_params;
  bool m_ftm_params_set;
};

/**
 * \brief Class for the FTM response header.
 * \ingroup FTM
 *
 * Implementation of the FTM response header.
 */
class FtmResponseHeader : public Header
{
public:
  FtmResponseHeader ();
  virtual ~FtmResponseHeader ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  /**
   * Set the dialog token.
   *
   * \param dialog_token the dialog token
   */
  void SetDialogToken (uint8_t dialog_token);

  /**
   * Returns the dialog token.
   *
   * \return the dialog token
   */
  uint8_t GetDialogToken (void);

  /**
   * Set the follow up dialog token.
   *
   * \param follow_up_dialog_token the follow up dialog token
   */
  void SetFollowUpDialogToken (uint8_t follow_up_dialog_token);

  /**
   * Returns the follow up dialog token.
   *
   * \return the follow up dialog token
   */
  uint8_t GetFollowUpDialogToken (void);

  /**
   * Set the time of departure.
   *
   * \param tod the time of departure
   */
  void SetTimeOfDeparture (uint64_t tod);

  /**
   * Returns the time of departure.
   *
   * \return the time of departure
   */
  uint64_t GetTimeOfDeparture (void);

  /**
   * Set the time of arrival.
   *
   * \param toa the time of arrival
   */
  void SetTimeOfArrival (uint64_t toa);

  /**
   * Returns the time of arrival.
   *
   * \return the time of arrival
   */
  uint64_t GetTimeOfArrival (void);

  /**
   * Set the time of departure error.
   *
   * \param rod_error the time of departure error
   */
  void SetTimeOfDepartureError (uint16_t tod_error);

  /**
   * Returns the time of departure error.
   *
   * \return the time of departure error
   */
  uint16_t GetTimeOfDepartureError (void);

  /**
   * Set the time of arrival error.
   *
   * \param toa_error the time of arrival error
   */
  void SetTimeOfArrivalError (uint16_t toa_error);

  /**
   * Returns the time of arrival error.
   *
   * \return the time of arrival error
   */
  uint16_t GetTimeOfArrivalError (void);

  /**
   * Set the FTM parameters.
   *
   * \param ftm_params the FtmParams
   */
  void SetFtmParams (FtmParams ftm_params);

  /**
   * Returns the FTM parameters.
   *
   * \return the FtmParams
   */
  FtmParams GetFtmParams (void);

  /**
   * Returns true if the FTM parameters have been set.
   *
   * \return if FtmParams have been set
   */
  bool GetFtmParamsSet (void);

private:
  uint8_t m_dialog_token;
  uint8_t m_follow_up_dialog_token;
  uint64_t m_tod;
  uint64_t m_toa;
  uint16_t m_tod_error;
  uint16_t m_toa_error;

  FtmParams m_ftm_params;
  bool m_ftm_params_set;

  bool m_big_endian; //!< If this system is big endian. Used for conversion of the 6 byte time stamps.
};

/**
 * \brief Class for the TSF sync info header.
 * \ingroup FTM
 *
 * Implementation of the TSF sync info header.
 */
class TsfSyncInfo : public Header
{
public:
  TsfSyncInfo ();
  virtual ~TsfSyncInfo ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  /**
   * Sets the TSF sync info.
   * \param tsf_sync the value for the TSF sync info
   */
  void SetTsfSyncInfo (uint32_t tsf_sync);

  /**
   * Returns the TSF sync info.
   * \return the TSF sync info
   */
  uint32_t GetTsfSyncInfo () const;

private:
  const uint8_t element_id = 255;
  const uint8_t length = 5;
  const uint8_t element_id_extension = 9;
  uint32_t tsf_sync_info;
};




} /* namespace ns3 */

#endif /* FTM_HEADER_H_ */
