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

#include "ftm-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FtmParams);

TypeId
FtmParams::GetTypeId (void)
{
  static TypeId tid = TypeId ("FtmParams")
    .SetParent<Header> ()
    .AddConstructor<FtmParams> ()
  ;
  return tid;
}

TypeId
FtmParams::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


FtmParams::FtmParams ()
{
  m_status_indication = RESERVED;
  m_status_indication_value = 0;
  m_number_of_bursts_exponent = 0;
  m_burst_duration = 0;
  m_min_delta_ftm = 0;
  m_partial_tsf_timer = 0;
  m_partial_tsf_no_pref = false;
  m_asap_capable = false;
  m_asap = false;
  m_ftms_per_burst = 0;
  m_format_and_bandwidth = 0;
  m_burst_period = 0;
}

FtmParams::~FtmParams ()
{

}

uint32_t
FtmParams::GetSerializedSize (void) const
{
  return 11;
}
void
FtmParams::Serialize (Buffer::Iterator start) const
{
  start.WriteU8(m_element_id);
  start.WriteU8(m_tag_length);
  uint8_t tmp = (m_status_indication & 0x03) | ((m_status_indication_value & 0x1F) << 2);
  start.WriteU8(tmp);
  start.WriteU8((m_number_of_bursts_exponent & 0x0F) | (m_burst_duration << 4));
  start.WriteU8(m_min_delta_ftm);
  start.WriteHtonU16(m_partial_tsf_timer); //weird in wireshark
  tmp = (m_partial_tsf_no_pref & 0x01) | ((m_asap_capable << 1) & 0x02) | ((m_asap << 2) & 0x04);
  tmp |= ((m_ftms_per_burst << 3) & 0xF8);
//  std::cout << (int) tmp << " " << (int) ((m_ftms_per_burst << 3) & 0xF8) << std::endl;
  start.WriteU8(tmp);
  start.WriteU8(m_format_and_bandwidth << 2);
  start.WriteHtonU16(m_burst_period);
}

uint32_t
FtmParams::Deserialize (Buffer::Iterator start)
{
  uint8_t element_id = start.ReadU8();
  NS_ASSERT_MSG(element_id == m_element_id, "Received unsupported Tag in FTM Packet");
  uint8_t length = start.ReadU8();
  NS_ASSERT(length == m_tag_length);
  uint8_t tmp = start.ReadU8();
  m_status_indication = IntToStatusIndication (tmp & 0x03);
  m_status_indication_value = (tmp >> 2) & 0x1F;
  tmp = start.ReadU8();
  m_number_of_bursts_exponent = tmp & 0x0F;
  m_burst_duration = tmp >> 4;
  m_min_delta_ftm = start.ReadU8();
  m_partial_tsf_timer = start.ReadNtohU16(); //TODO weird in wireshark
  tmp = start.ReadU8();
  m_partial_tsf_no_pref = tmp & 0x01 ;
  m_asap_capable = (tmp >> 1) & 0x01;
  m_asap = (tmp >> 2) & 0x01;
  m_ftms_per_burst = (tmp >> 3) & 0x1F;
  m_format_and_bandwidth = start.ReadU8() >> 2;
  m_burst_period = start.ReadNtohU16();
  return 11; // the number of bytes consumed.
}

void
FtmParams::Print (std::ostream &os) const
{
  os << "element_id=" << (int) m_element_id
      << ", length=" << (int) m_tag_length
      << ", status_indication=" << (int) m_status_indication
      << ", status_indication_value=" << (int) m_status_indication_value
      << ", number_of_bursts_exponent=" << (int) m_number_of_bursts_exponent
      << ", burst_duration=" << (int) m_burst_duration
      << ", min_delta_ftm=" << (int) m_min_delta_ftm
      << ", partial_tsf_timer=" << (int) m_partial_tsf_timer
      << ", partial_tsf_no_pref=" << m_partial_tsf_no_pref
      << ", asap_capable=" << m_asap_capable
      << ", asap=" << m_asap
      << ", ftms_per_burst=" << (int) m_ftms_per_burst
      << ", format_and_bandwidth=" << (int) m_format_and_bandwidth
      << ", burst_period=" << (int) m_burst_period;
}

void
FtmParams::SetStatusIndication (StatusIndication status)
{
  m_status_indication = status;
}

FtmParams::StatusIndication
FtmParams::GetStatusIndication (void)
{
  return m_status_indication;
}

void
FtmParams::SetStatusIndicationValue (uint8_t value)
{
  m_status_indication_value = value;
}

uint8_t
FtmParams::GetStatusIndicationValue (void)
{
  return m_status_indication_value;
}

void
FtmParams::SetNumberOfBurstsExponent (uint8_t burst_exponent)
{
  m_number_of_bursts_exponent = burst_exponent;
}

uint8_t
FtmParams::GetNumberOfBurstsExponent (void)
{
  return m_number_of_bursts_exponent;
}

void
FtmParams::SetBurstDuration (uint8_t burst_duration)
{
  m_burst_duration = burst_duration;
}

uint8_t
FtmParams::GetBurstDuration (void)
{
  return m_burst_duration;
}

void
FtmParams::SetMinDeltaFtm (uint8_t min_delta_ftm)
{
  m_min_delta_ftm = min_delta_ftm;
}
uint8_t
FtmParams::GetMinDeltaFtm (void)
{
  return m_min_delta_ftm;
}

void
FtmParams::SetPartialTsfTimer (uint16_t partial_tsf_timer)
{
  m_partial_tsf_timer = partial_tsf_timer;
}

uint16_t
FtmParams::GetPartialTsfTimer (void)
{
  return m_partial_tsf_timer;
}

void
FtmParams::SetPartialTsfNoPref (bool tsf_no_pref)
{
  m_partial_tsf_no_pref = tsf_no_pref;
}

bool
FtmParams::GetPartialTsfNoPref (void)
{
  return m_partial_tsf_no_pref;
}

void
FtmParams::SetAsapCapable (bool asap_capable)
{
  m_asap_capable = asap_capable;
}

bool
FtmParams::GetAsapCapable (void)
{
  return m_asap_capable;
}

void
FtmParams::SetAsap (bool asap)
{
  m_asap = asap;
}

bool
FtmParams::GetAsap (void)
{
  return m_asap;
}

void
FtmParams::SetFtmsPerBurst (uint8_t ftms_per_burst)
{
  m_ftms_per_burst = ftms_per_burst;
}

uint8_t
FtmParams::GetFtmsPerBurst (void)
{
  return m_ftms_per_burst;
}

void
FtmParams::SetFormatAndBandwidth (uint8_t format_bandwidth)
{
  m_format_and_bandwidth = format_bandwidth;
}

uint8_t
FtmParams::GetFormatAndBandwidth (void)
{
  return m_format_and_bandwidth;
}

void
FtmParams::SetBurstPeriod (uint16_t burst_period)
{
  m_burst_period = burst_period;
}

uint16_t
FtmParams::GetBurstPeriod (void)
{
  return m_burst_period;
}

FtmParams::StatusIndication
FtmParams::IntToStatusIndication (uint8_t status)
{
  switch (status)
  {
    case 0:
      return RESERVED;
    case 1:
      return SUCCESSFUL;
    case 2:
      return REQUEST_INCAPABLE;
    case 3:
      return REQUEST_FAILED;
  }
  return RESERVED; //used to keep compiler happy, should never be reached as status is only 2 bits
}

// returns the burst duration in micro seconds
uint32_t
FtmParams::DecodeBurstDuration (void)
{
  if (m_burst_duration < 2 || m_burst_duration > 11)
    {
      return 0;
    }
  return 125 * (1 << (m_burst_duration - 1)); //125 * (2 ^ (m_burst_duration - 1))
}

NS_OBJECT_ENSURE_REGISTERED (FtmParamsHolder);

TypeId
FtmParamsHolder::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FtmParamsHolder")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<FtmParamsHolder>()
    ;
  return tid;
}

FtmParamsHolder::FtmParamsHolder ()
{

}

FtmParamsHolder::~FtmParamsHolder ()
{

}

void
FtmParamsHolder::SetFtmParams (FtmParams params)
{
  m_ftm_params = params;
}

FtmParams
FtmParamsHolder::GetFtmParams ()
{
  return m_ftm_params;
}


/*
 * Ftm Request Header class used for creating request frames
 */

NS_OBJECT_ENSURE_REGISTERED (FtmRequestHeader);

TypeId
FtmRequestHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("FtmRequestHeader")
    .SetParent<Header> ()
    .AddConstructor<FtmRequestHeader> ()
  ;
  return tid;
}

TypeId
FtmRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


FtmRequestHeader::FtmRequestHeader ()
{
  m_trigger = 0;
  m_ftm_params_set = false;
}

FtmRequestHeader::~FtmRequestHeader ()
{

}

uint32_t
FtmRequestHeader::GetSerializedSize (void) const
{
  if (m_ftm_params_set)
    {
      return 1 + m_ftm_params.GetSerializedSize();
    }
  else
    {
      return 1;
    }
}

void
FtmRequestHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU8(m_trigger);
  if (m_ftm_params_set)
    {
      m_ftm_params.Serialize(start);
    }
}
uint32_t
FtmRequestHeader::Deserialize (Buffer::Iterator start)
{
  m_trigger = start.ReadU8();

  if (start.GetRemainingSize() >= m_ftm_params.GetSerializedSize()
      && start.PeekU8() == 206)
    {
      m_ftm_params.Deserialize(start);
      m_ftm_params_set = true;
      return 1 + m_ftm_params.GetSerializedSize();
    }
  return 1; // the number of bytes consumed.
}

void
FtmRequestHeader::Print (std::ostream &os) const
{
  os << "trigger=" << m_trigger;
  if (m_ftm_params_set)
    {
      os << ", FTM_PARAMS: ";
      m_ftm_params.Print(os);
    }
}

void
FtmRequestHeader::SetTrigger (uint8_t trigger)
{
  m_trigger = trigger;
}

uint8_t
FtmRequestHeader::GetTrigger (void)
{
  return m_trigger;
}

void
FtmRequestHeader::SetFtmParams (FtmParams ftm_params)
{
  m_ftm_params = ftm_params;
  m_ftm_params_set = true;
}


FtmParams
FtmRequestHeader::GetFtmParams (void)
{
  if (m_ftm_params_set)
    {
      return m_ftm_params;
    }
  return FtmParams();
}

bool
FtmRequestHeader::GetFtmParamsSet (void)
{
  return m_ftm_params_set;
}

/*
 * Ftm Response Header class used for creating ftm responses
 */

NS_OBJECT_ENSURE_REGISTERED (FtmResponseHeader);

TypeId
FtmResponseHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("FtmResponseHeader")
    .SetParent<Header> ()
    .AddConstructor<FtmResponseHeader> ()
  ;
  return tid;
}

TypeId
FtmResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


FtmResponseHeader::FtmResponseHeader ()
{
  m_dialog_token = 0;
  m_follow_up_dialog_token = 0;
  m_tod = 0;
  m_toa = 0;
  m_tod_error = 0;
  m_toa_error = 0;

  union { //source: https://stackoverflow.com/a/1001373
    uint32_t i;
    char c[4];
  } bint = {0x01020304};
  m_big_endian = bint.c[0] == 1;

  m_ftm_params_set = false;
}

FtmResponseHeader::~FtmResponseHeader ()
{

}

uint32_t
FtmResponseHeader::GetSerializedSize (void) const
{
  if (m_ftm_params_set)
    {
      return 18 + m_ftm_params.GetSerializedSize();
    }
  else
    {
      return 18;
    }
}

/*
 * for now the timestamps are in 64bit size, but are only 48bit in the ftm protocol
 */
void
FtmResponseHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU8(m_dialog_token);
  start.WriteU8(m_follow_up_dialog_token);

  if(m_big_endian) {
      for(int i = 0; i < 6; i++) {
          start.WriteU8((m_tod >> (40 - 8 * i)) &0xFF);
      }
      for(int i = 0; i < 6; i++) {
          start.WriteU8((m_toa >> (40 - 8 * i)) &0xFF);
      }
  }
  else {
      for(int i = 0; i < 6; i++) {
          start.WriteU8((m_tod >> (8 * i)) &0xFF);
      }
      for(int i = 0; i < 6; i++) {
          start.WriteU8((m_toa >> (8 * i)) &0xFF);
      }
  }

  start.WriteHtonU16(m_tod_error);
  start.WriteHtonU16(m_toa_error);

  if (m_ftm_params_set)
    {
      m_ftm_params.Serialize(start);
    }
}

uint32_t
FtmResponseHeader::Deserialize (Buffer::Iterator start)
{
  m_dialog_token = start.ReadU8();
  m_follow_up_dialog_token = start.ReadU8();
  uint8_t tod_buffer[6];
  start.Read (tod_buffer, 6);
  uint8_t toa_buffer[6];
  start.Read (toa_buffer, 6);
  m_tod = 0;
  m_toa = 0;

  if(m_big_endian) {
      int i;
      for(i = 0; i < 5; i++) {
          m_tod |= tod_buffer[i];
          m_tod <<= 8;

          m_toa |= toa_buffer[i];
          m_toa <<= 8;
      }
      m_tod |= tod_buffer[i];
      m_toa |= toa_buffer[i];
  }
  else {
      int i;
      for(i = 5; i > 0; i--) {
          m_tod |= tod_buffer[i];
          m_tod <<= 8;

          m_toa |= toa_buffer[i];
          m_toa <<= 8;
      }
      m_tod |= tod_buffer[i];
      m_toa |= toa_buffer[i];
  }

  m_tod_error = start.ReadNtohU16();
  m_toa_error = start.ReadNtohU16();

  if (start.GetRemainingSize() >= m_ftm_params.GetSerializedSize()
      && start.PeekU8() == 206)
    {

      m_ftm_params.Deserialize(start);
      m_ftm_params_set = true;
      return 18 + m_ftm_params.GetSerializedSize();
    }

  return 18; // the number of bytes consumed.
}

void
FtmResponseHeader::Print (std::ostream &os) const
{
  os << "DialogToken=" << m_dialog_token
      << ", FollowUpDialogToken=" << m_follow_up_dialog_token
      << ", TOD=" << m_tod << ", TOA=" << m_toa
      << ", TOD_Error=" << m_tod_error
      << ", TOA_Error=" << m_toa_error;
}

void
FtmResponseHeader::SetDialogToken (uint8_t dialog_token)
{
  m_dialog_token = dialog_token;
}

uint8_t
FtmResponseHeader::GetDialogToken (void)
{
  return m_dialog_token;
}

void
FtmResponseHeader::SetFollowUpDialogToken (uint8_t follow_up_dialog_token)
{
  m_follow_up_dialog_token = follow_up_dialog_token;
}

uint8_t
FtmResponseHeader::GetFollowUpDialogToken (void)
{
  return m_follow_up_dialog_token;
}

void
FtmResponseHeader::SetTimeOfDeparture (uint64_t tod)
{
  m_tod = tod;
}

uint64_t
FtmResponseHeader::GetTimeOfDeparture (void)
{
  return m_tod;
}

void
FtmResponseHeader::SetTimeOfArrival (uint64_t toa)
{
  m_toa = toa;
}

uint64_t
FtmResponseHeader::GetTimeOfArrival (void)
{
  return m_toa;
}

void
FtmResponseHeader::SetTimeOfDepartureError (uint16_t tod_error)
{
  m_tod_error = tod_error;
}

uint16_t
FtmResponseHeader::GetTimeOfDepartureError (void)
{
  return m_tod_error;
}

void
FtmResponseHeader::SetTimeOfArrivalError (uint16_t toa_error)
{
  m_toa_error = toa_error;
}

uint16_t
FtmResponseHeader::GetTimeOfArrivalError (void)
{
  return m_toa_error;
}

void
FtmResponseHeader::SetFtmParams (FtmParams ftm_params)
{
  m_ftm_params = ftm_params;
  m_ftm_params_set = true;
}


FtmParams
FtmResponseHeader::GetFtmParams (void)
{
  if (m_ftm_params_set)
    {
      return m_ftm_params;
    }
  return FtmParams();
}

bool
FtmResponseHeader::GetFtmParamsSet (void)
{
  return m_ftm_params_set;
}


NS_OBJECT_ENSURE_REGISTERED (TsfSyncInfo);

TsfSyncInfo::TsfSyncInfo ()
{
  tsf_sync_info = 0;
}

TsfSyncInfo::~TsfSyncInfo ()
{

}

TypeId
TsfSyncInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("TsfSyncInfo")
    .SetParent<Header> ()
    .AddConstructor<TsfSyncInfo> ()
  ;
  return tid;
}

TypeId
TsfSyncInfo::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
TsfSyncInfo::GetSerializedSize () const
{
  return 7;
}

void
TsfSyncInfo::Serialize (Buffer::Iterator start) const
{
  start.WriteU8 (element_id);
  start.WriteU8 (length);
  start.WriteU8 (element_id_extension);
  start.WriteHtonU32 (tsf_sync_info);
}

uint32_t
TsfSyncInfo::Deserialize (Buffer::Iterator start)
{
  uint8_t tmp = start.ReadU8 ();
  NS_ASSERT (tmp == element_id);
  tmp = start.ReadU8 ();
  NS_ASSERT (tmp == length);
  tmp = start.ReadU8 ();
  NS_ASSERT (tmp == element_id_extension);
  tsf_sync_info = start.ReadNtohU32 ();
  return GetSerializedSize ();
}

void
TsfSyncInfo::Print (std::ostream &os) const
{
  os << "TSF sync info=" << tsf_sync_info;
}

void
TsfSyncInfo::SetTsfSyncInfo (uint32_t tsf_sync)
{
  tsf_sync_info = tsf_sync;
}

uint32_t
TsfSyncInfo::GetTsfSyncInfo (void) const
{
  return tsf_sync_info;
}

} /* namespace ns3 */
