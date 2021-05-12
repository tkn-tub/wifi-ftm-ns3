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

#include "ftm-session.h"
#include "ns3/ftm-header.h"
#include "ns3/core-module.h"
#include "ns3/mgt-headers.h"
#include "ns3/wifi-mac-header.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FtmSession");

NS_OBJECT_ENSURE_REGISTERED (FtmSession);

TypeId
FtmSession::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FtmSession")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<FtmSession> ()
    .AddAttribute ("FtmErrorModel",
                   "The FtmErrorModel to be used for the FtmSession.",
                   PointerValue (),
                   MakePointerAccessor (&FtmSession::SetFtmErrorModel),
                   MakePointerChecker<FtmErrorModel> ())
    .AddAttribute ("DefaultFtmParams",
                   "The default FTM parameters which will be used on the initiator, if no other "
                   "parameters are specified.",
                   PointerValue (),
                   MakePointerAccessor (&FtmSession::SetDefaultFtmParamsHolder),
                   MakePointerChecker<FtmParamsHolder> ())
  ;
  return tid;
}

FtmSession::FtmSession ()
{
  m_session_type = FTM_UNINITIALIZED;
  m_session_over_callback_set = false;
  m_dialog_token_overflow = false;
  m_session_active = false;
  m_ftm_error_model = CreateObject<FtmErrorModel> ();
  m_live_rtt_enabled = false;
  CreateDefaultFtmParams ();

  send_packet = MakeNullCallback <void, Ptr<Packet>, WifiMacHeader> ();
  session_over_ftm_manager_callback = MakeNullCallback<void, Mac48Address> ();
  session_over_callback = MakeNullCallback<void, FtmSession> ();
  block_session = MakeNullCallback<void, Mac48Address, Time> ();
  live_rtt = MakeNullCallback<void, int64_t> ();
  session_override = MakeNullCallback<void, Mac48Address, FtmRequestHeader> ();
}

FtmSession::~FtmSession ()
{
  m_ftm_error_model = 0;
  m_rtt_list.clear();
  m_ftm_dialogs.clear();
  m_current_dialog = 0;


  send_packet = MakeNullCallback <void, Ptr<Packet>, WifiMacHeader> ();
  session_over_ftm_manager_callback = MakeNullCallback<void, Mac48Address> ();
  session_over_callback = MakeNullCallback<void, FtmSession> ();
  block_session = MakeNullCallback<void, Mac48Address, Time> ();
  live_rtt = MakeNullCallback<void, int64_t> ();
  session_override = MakeNullCallback<void, Mac48Address, FtmRequestHeader> ();
}

void
FtmSession::InitSession (Mac48Address partner_addr, SessionType type, Callback <void, Ptr<Packet>, WifiMacHeader> callback)
{
  m_partner_addr = partner_addr;
  m_session_type = type;
  send_packet = callback;
  if (type == FTM_INITIATOR)
    {
      UseDefaultFtmParams ();
    }
}

void
FtmSession::SetPreambleDetectionDuration (Time duration)
{
  m_preamble_detection_duration = duration.GetPicoSeconds();
}

void
FtmSession::SetFtmParams (FtmParams ftm_params)
{
  m_ftm_params = ftm_params;
}

FtmParams
FtmSession::GetFtmParams (void)
{
  return m_ftm_params;
}

void
FtmSession::ProcessFtmRequest (FtmRequestHeader ftm_req)
{
  if (ftm_req.GetTrigger() == 1)
    {
      if(ftm_req.GetFtmParamsSet() && !m_session_active)
        {
          SetFtmParams(ftm_req.GetFtmParams());
          if (ValidateFtmParams()) //if parameters valid then session gets accepted
            {
              m_session_active = true;
              m_ftm_params.SetStatusIndication(FtmParams::SUCCESSFUL);
              m_ftm_params.SetAsapCapable(true);

              m_current_dialog_token = 1;
              m_previous_dialog_token = 0;
              m_number_of_bursts_remaining = 1 << m_ftm_params.GetNumberOfBurstsExponent(); // 2 ^ Number of Bursts
              m_ftms_per_burst_remaining = m_ftm_params.GetFtmsPerBurst();
              SessionBegin();
            }
          else
            {
              DenySession ();
            }
        }
      else if (ftm_req.GetFtmParamsSet() && m_session_active)
        {
          EndSession ();
          session_override (m_partner_addr, ftm_req);
        }
      else
        {
          TriggerReceived ();
        }

    }
  else if (ftm_req.GetTrigger() == 0)
    {
      EndSession ();
    }
}

void
FtmSession::ProcessFtmResponse (FtmResponseHeader ftm_res)
{
  if (ftm_res.GetFtmParamsSet())
    {
      m_ftm_params = ftm_res.GetFtmParams ();
      FtmParams::StatusIndication status = m_ftm_params.GetStatusIndication();
      if (status == FtmParams::SUCCESSFUL)
        {
          m_number_of_bursts_remaining = 1 << m_ftm_params.GetNumberOfBurstsExponent(); // 2 ^ Number of Bursts
          m_next_burst_period = MilliSeconds(m_ftm_params.GetBurstPeriod() * 100);

          if(!m_ftm_params.GetAsap())
            {
              m_ftm_dialogs.clear();
              Time burst_begin = MilliSeconds (m_ftm_params.GetPartialTsfTimer());
              Simulator::Schedule(burst_begin, &FtmSession::StartNextBurst, this);
            }
          else
            {
              m_number_of_bursts_remaining--;
              Simulator::Schedule(m_next_burst_period, &FtmSession::StartNextBurst, this);
            }
        }
      else if (status == FtmParams::REQUEST_FAILED)
        {
          NS_LOG_ERROR ("FTM Request Failed!");
          if (m_ftm_params.GetStatusIndicationValue () != 0)
            {
              Time timeout = Seconds (m_ftm_params.GetStatusIndicationValue());
              block_session (m_partner_addr, timeout);
            }
          EndSession ();
          return;
        }
      else
        {
          NS_LOG_ERROR ("FTM Request Incapable!");
          EndSession ();
          return;
        }
    }

  if (ftm_res.GetDialogToken() != 0)
    {
      Ptr<FtmDialog> dialog = FindDialog (ftm_res.GetDialogToken());
      if(dialog == 0)
        {
          dialog = CreateNewDialog(ftm_res.GetDialogToken());
          m_ftm_dialogs.insert({ftm_res.GetDialogToken(), dialog});
        }
    }

  if (ftm_res.GetFollowUpDialogToken() != 0)
    {
      Ptr<FtmDialog> follow_up_dialog = FindDialog(ftm_res.GetFollowUpDialogToken());
      if(follow_up_dialog != 0 && follow_up_dialog->t1 == 0 && follow_up_dialog->t4 == 0)
        {
          follow_up_dialog->t1 = ftm_res.GetTimeOfDeparture();
          follow_up_dialog->t4 = ftm_res.GetTimeOfArrival();
          CalculateRTT (follow_up_dialog);
        }
    }

  if (ftm_res.GetDialogToken() == 0)
    {
      EndSession ();
    }
}

bool
FtmSession::ValidateFtmParams (void)
{
  if (m_ftm_params.GetStatusIndication () != FtmParams::RESERVED)
    {
      NS_LOG_ERROR ("FTM session denied! Status indication is not set to reserved");
      return false;
    }
  if (m_ftm_params.GetNumberOfBurstsExponent () == 15)
    {
      m_ftm_params.SetNumberOfBurstsExponent (m_default_ftm_params.GetNumberOfBurstsExponent ());
    }
  uint8_t burst_duration = m_ftm_params.GetBurstDuration ();
  if (burst_duration <= 1 || (burst_duration >= 12 && burst_duration <= 14))
    {
      NS_LOG_ERROR ("FTM session denied! Burst duration set to a reserved value. Should be in range [2, 11] "
          "or 15 if no preference.");
      return false;
    }
  if (burst_duration == 15)
    {
      m_ftm_params.SetBurstDuration (m_default_ftm_params.GetBurstDuration ());
    }
  if (m_ftm_params.GetMinDeltaFtm () == 0)
    {
      m_ftm_params.SetMinDeltaFtm (m_default_ftm_params.GetMinDeltaFtm ());
    }
  //TSF field does not need to be checked, because in the way we use it, its value is always valid
  //ASAP capable field is reserved in initial FTM request, so if true, parameters are not valid
  if (m_ftm_params.GetAsapCapable ())
    {
      NS_LOG_ERROR ("FTM session denied! ASAP capable field has been set. It is reserved in the FTM request.");
      return false;
    }
  //ASAP and TSF no pref do not need to be checked here
  if (m_ftm_params.GetFtmsPerBurst () == 0)
    {
      m_ftm_params.SetFtmsPerBurst (m_default_ftm_params.GetFtmsPerBurst ());
    }
  //Format and bandwidth is not used, so we do not need to check it

  //if number of bursts exponent is 0, then we only have one burst and the burst period field is reserved
  //so if burst period is set to something, the parameters are invalid
  if (m_ftm_params.GetNumberOfBurstsExponent () == 0 && m_ftm_params.GetBurstPeriod () != 0)
    {
      NS_LOG_ERROR ("FTM session denied! This is caused by the number of bursts exponent being 0 and "
          "the burst period not being 0. Both of them need to be 0 in this case.");
      return false;
    }
  if (m_ftm_params.GetBurstPeriod () == 0 && m_ftm_params.GetNumberOfBurstsExponent () > 0)
    {
      m_ftm_params.SetBurstPeriod (m_default_ftm_params.GetBurstPeriod ());
    }
  //validate if burst duration is enough to fit all packets with min delta ftm spacing
  //if it is not enough, we cant accept this session
  unsigned int min_required_burst_duration = (m_ftm_params.GetFtmsPerBurst() + 1) * m_ftm_params.GetMinDeltaFtm() * 100;
  if (min_required_burst_duration > m_ftm_params.DecodeBurstDuration())
    {
      NS_LOG_ERROR ("FTM session denied! This may be caused by a too small burst duration, too many FTMs per burst "
          "for the specified burst duration or a too large min delta FTM for the specified burst duration and "
          "FTMs per burst.");
      return false;
    }
  if (m_ftm_params.GetFtmsPerBurst() == 1 && m_ftm_params.GetNumberOfBurstsExponent() == 0)
    {
      NS_LOG_ERROR ("FTM session denied! Can not do anything useful with 1 FTM per burst and 1 burst.");
      return false;
    }
  return true;
}

/*
 * default ftm params for initiator of session
 */
void
FtmSession::CreateDefaultFtmParams (void)
{
  FtmParams ftm_params;
  ftm_params.SetStatusIndication(FtmParams::RESERVED);
  ftm_params.SetStatusIndicationValue(0);
  ftm_params.SetNumberOfBurstsExponent(1); //2 bursts
  ftm_params.SetBurstDuration(6); //4ms burst duration, this needs to be larger due to long processing delay until transmission

  ftm_params.SetMinDeltaFtm(4); //400us between frames
  ftm_params.SetPartialTsfTimer(0);
  ftm_params.SetPartialTsfNoPref(true);
  ftm_params.SetAsapCapable(false);
  ftm_params.SetAsap(true);
  ftm_params.SetFtmsPerBurst(2);

  ftm_params.SetFormatAndBandwidth(0);
  ftm_params.SetBurstPeriod(2); //200ms between burst periods

  m_default_ftm_params = ftm_params;
}

void
FtmSession::SetDefaultFtmParams (FtmParams params)
{
  m_default_ftm_params = params;
}

void
FtmSession::SetDefaultFtmParamsHolder (Ptr<FtmParamsHolder> params)
{
  SetDefaultFtmParams (params->GetFtmParams ());
}

void
FtmSession::UseDefaultFtmParams (void)
{
  m_ftm_params = m_default_ftm_params;
}

void
FtmSession::SessionBegin (void)
{
  Ptr<Packet> packet = Create<Packet>();

  WifiActionHeader hdr;
  WifiActionHeader::ActionValue action;

  if (m_session_type == FTM_INITIATOR)
    {
      FtmRequestHeader ftm_req_hdr;
      ftm_req_hdr.SetTrigger(1);

      ftm_req_hdr.SetFtmParams(m_ftm_params);

      packet->AddHeader(ftm_req_hdr);

      action.publicAction = WifiActionHeader::FTM_REQUEST;
      hdr.SetAction(WifiActionHeader::PUBLIC_ACTION, action);
      packet->AddHeader(hdr);
    }
  else if (m_session_type == FTM_RESPONDER)
    {
      m_next_burst_period = MilliSeconds(m_ftm_params.GetBurstPeriod() * 100);
      m_next_ftm_packet = MicroSeconds(m_ftm_params.GetMinDeltaFtm() * 100);

      TsfSyncInfo tsf_sync;
      packet->AddHeader (tsf_sync);

      FtmResponseHeader ftm_res_hdr;
      ftm_res_hdr.SetDialogToken(m_current_dialog_token);

      if (m_ftm_params.GetAsap())
        {
          uint8_t dialog_token = ftm_res_hdr.GetDialogToken();
          Ptr<FtmDialog> new_dialog = CreateNewDialog (dialog_token);
          m_current_dialog = new_dialog;
          m_ftm_dialogs.insert({dialog_token, new_dialog});

          m_current_burst_end = Simulator::Now() + MicroSeconds(m_ftm_params.DecodeBurstDuration());

          m_number_of_bursts_remaining--;
          m_ftms_per_burst_remaining--;

          Simulator::Schedule(m_next_ftm_packet, &FtmSession::SendNextFtmPacket, this);
          if (m_number_of_bursts_remaining > 0)
            {
              Simulator::Schedule(m_next_burst_period, &FtmSession::StartNextBurst, this);
            }
          m_ftm_params.SetPartialTsfNoPref (false);
        }
      else
        {
          if (m_ftm_params.GetPartialTsfNoPref ())
            {
              m_ftm_params.SetPartialTsfNoPref (false);
              m_ftm_params.SetPartialTsfTimer (500);
              Simulator::Schedule(MilliSeconds (500), &FtmSession::StartNextBurst, this);
            }
          else
            {
              Simulator::Schedule(MilliSeconds (m_ftm_params.GetPartialTsfTimer()), &FtmSession::StartNextBurst, this);
            }
        }
      ftm_res_hdr.SetFtmParams(m_ftm_params);
      packet->AddHeader(ftm_res_hdr);

      action.publicAction = WifiActionHeader::FTM_RESPONSE;
      hdr.SetAction(WifiActionHeader::PUBLIC_ACTION, action);
      packet->AddHeader(hdr);
    }
  else
    {
      NS_FATAL_ERROR ("Session has not been initialized!");
    }

  WifiMacHeader mac_hdr;
  mac_hdr.SetAddr1(m_partner_addr);

  send_packet(packet, mac_hdr);
}

void
FtmSession::SendNextFtmPacket (void)
{
  if(m_ftms_per_burst_remaining > 0 && Simulator::Now() < m_current_burst_end)
    {
      if (!CheckTimestampSet ())
        {
          Simulator::Schedule(m_next_ftm_packet / 10, &FtmSession::SendNextFtmPacket, this);
          return;
        }
      bool add_tsf_sync = false;
      if (m_ftms_per_burst_remaining == m_ftm_params.GetFtmsPerBurst())
        {
          add_tsf_sync = true;
        }
      m_ftms_per_burst_remaining--;

      m_previous_dialog_token = m_current_dialog_token;
      m_current_dialog_token++;
      if (m_current_dialog_token == 0)
        {
          m_current_dialog_token = 1;
          m_dialog_token_overflow = true;
        }
      if (m_dialog_token_overflow)
        {
          DeleteDialog (m_current_dialog_token);
        }

      Ptr<FtmDialog> new_dialog = CreateNewDialog(m_current_dialog_token);
      m_current_dialog = new_dialog;
      m_ftm_dialogs.insert({m_current_dialog_token, new_dialog});

      Ptr<FtmDialog> previous_dialog = FindDialog (m_previous_dialog_token);
      FtmResponseHeader ftm_res_hdr;
      ftm_res_hdr.SetDialogToken(m_current_dialog_token);
      if(previous_dialog != 0)
        {
          ftm_res_hdr.SetFollowUpDialogToken(m_previous_dialog_token);
          ftm_res_hdr.SetTimeOfDeparture(previous_dialog->t1);
          ftm_res_hdr.SetTimeOfArrival(previous_dialog->t4);
        }
      if (m_ftms_per_burst_remaining <= 0 && m_number_of_bursts_remaining <= 0)
        {
          ftm_res_hdr.SetDialogToken(0);
        }
      Ptr<Packet> packet = Create<Packet> ();
      if (add_tsf_sync)
        {
          TsfSyncInfo tsf_sync;
          packet->AddHeader (tsf_sync);
        }
      packet->AddHeader(ftm_res_hdr);

      WifiActionHeader hdr;
      WifiActionHeader::ActionValue action;
      action.publicAction = WifiActionHeader::FTM_RESPONSE;
      hdr.SetAction(WifiActionHeader::PUBLIC_ACTION, action);
      packet->AddHeader(hdr);

      WifiMacHeader mac_hdr;
      mac_hdr.SetAddr1(m_partner_addr);
      send_packet(packet, mac_hdr);

      Simulator::Schedule(m_next_ftm_packet, &FtmSession::SendNextFtmPacket, this);
    }
  else if (m_ftms_per_burst_remaining <= 0 && m_number_of_bursts_remaining <= 0)
    {
      EndSession ();
    }

  /*
   * Case for when session is over its time limit but we still have remaining FTMs per burst.
   * We send the last frame to end the session, this will happen when the burst period is over.
   * We overdraw the time constraints. This is unavoidable due to queuing delays.
   * To make the most out of the session we make sure the ending frame also has some useful
   * information for the initiator, rather than just ending the session without new information
   * to maybe not overdraw the time constraint as much. Not perfect but at least useful.
   */
  else if(Simulator::Now() >= m_current_burst_end && m_number_of_bursts_remaining <= 0
          && m_ftms_per_burst_remaining > 0)
    {
      if (!CheckTimestampSet ())
        {
          Simulator::Schedule(m_next_ftm_packet / 10, &FtmSession::SendNextFtmPacket, this);
          return;
        }
      /*
       * Our previous dialog is the last successfully received dialog. As there wont be a next dialog,
       * we do not have to create a new one and advance the dialog token. This is the last dialog of the session.
       */
      Ptr<FtmDialog> previous_dialog = FindDialog (m_current_dialog_token);
      FtmResponseHeader ftm_res_hdr;
      ftm_res_hdr.SetDialogToken(0);
      ftm_res_hdr.SetFollowUpDialogToken(m_current_dialog_token);
      ftm_res_hdr.SetTimeOfDeparture(previous_dialog->t1);
      ftm_res_hdr.SetTimeOfArrival(previous_dialog->t4);

      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader(ftm_res_hdr);

      WifiActionHeader hdr;
      WifiActionHeader::ActionValue action;
      action.publicAction = WifiActionHeader::FTM_RESPONSE;
      hdr.SetAction(WifiActionHeader::PUBLIC_ACTION, action);
      packet->AddHeader(hdr);

      WifiMacHeader mac_hdr;
      mac_hdr.SetAddr1(m_partner_addr);
      send_packet(packet, mac_hdr);

      EndSession ();
    }
}

void
FtmSession::StartNextBurst (void)
{
  if (m_number_of_bursts_remaining > 0)
    {
      m_number_of_bursts_remaining--;
      m_ftms_per_burst_remaining = m_ftm_params.GetFtmsPerBurst();
      m_current_burst_end = Simulator::Now() + MicroSeconds(m_ftm_params.DecodeBurstDuration());

      if (m_session_type == FTM_INITIATOR)
        {
          SendTrigger();
        }
      if (m_number_of_bursts_remaining > 0)
        {
          Simulator::Schedule(m_next_burst_period, &FtmSession::StartNextBurst, this);
        }
    }
}

void
FtmSession::SetSessionOverCallback (Callback<void, FtmSession> callback)
{
  m_session_over_callback_set = true;
  session_over_callback = callback;
}

void
FtmSession::SetSessionOverCallbackManager (Callback<void, Mac48Address> callback)
{
  session_over_ftm_manager_callback = callback;
}

void
FtmSession::SetBlockSessionCallback (Callback<void, Mac48Address, Time> callback)
{
  block_session = callback;
}

void
FtmSession::SetT1 (uint8_t dialog_token, uint64_t timestamp)
{
  Ptr<FtmDialog> dialog = FindDialog (dialog_token);
  if(dialog != 0)
    {
      dialog->t1 = timestamp;
    }
}

void
FtmSession::SetT2 (uint8_t dialog_token, uint64_t timestamp)
{
  Ptr<FtmDialog> dialog = FindDialog (dialog_token);
  if(dialog == 0)
    {
      dialog = CreateNewDialog(dialog_token);
      m_ftm_dialogs.insert({dialog_token, dialog});
    }
  dialog->t2 = timestamp;
}

void
FtmSession::SetT3 (uint8_t dialog_token, uint64_t timestamp)
{
  Ptr<FtmDialog> dialog = FindDialog (dialog_token);
  if(dialog != 0)
    {
      dialog->t3 = timestamp;
    }
}

void
FtmSession::SetT4 (uint8_t dialog_token, uint64_t timestamp)
{
  Ptr<FtmDialog> dialog = FindDialog (dialog_token);
  if(dialog != 0)
    {
      dialog->t4 = timestamp;
    }
}

Ptr<FtmSession::FtmDialog>
FtmSession::FindDialog (uint8_t dialog_token)
{
  auto search = m_ftm_dialogs.find (dialog_token);
  if (search != m_ftm_dialogs.end())
    {
      return search->second;
    }
  return 0;
}

void
FtmSession::DeleteDialog (uint8_t dialog_token)
{
  m_ftm_dialogs.erase (dialog_token);
}

Ptr<FtmSession::FtmDialog>
FtmSession::CreateNewDialog (uint8_t dialog_token)
{
  Ptr<FtmDialog> new_dialog = Create<FtmDialog> ();
  new_dialog->dialog_token = dialog_token;
  new_dialog->t1 = 0;
  new_dialog->t2 = 0;
  new_dialog->t3 = 0;
  new_dialog->t4 = 0;
  return new_dialog;
}

std::map<uint8_t, Ptr<FtmSession::FtmDialog>>
FtmSession::GetFtmDialogs (void)
{
  return m_ftm_dialogs;
}

int64_t
FtmSession::GetMeanRTT (void)
{
  if (m_rtt_list.size () == 0)
    {
      return 0;
    }
  int64_t avg_rtt = 0;
  for(int64_t curr : m_rtt_list)
    {
      avg_rtt += curr;
    }
  avg_rtt /= m_rtt_list.size ();
  return avg_rtt;
}

std::list<int64_t>
FtmSession::GetIndividualRTT (void)
{
  return m_rtt_list;
}

void
FtmSession::CalculateRTT (Ptr<FtmDialog> dialog)
{
  int64_t rtt = 0;
  int64_t diff_t4_t1;
  int64_t diff_t3_t2;
  if (dialog->t4 < dialog->t1) //time stamp overflow
    {
      diff_t4_t1 = (0xFFFFFFFFFFFF - dialog->t1) + dialog->t4;
    }
  else
    {
      diff_t4_t1 = dialog->t4 - dialog->t1;
    }
  if (dialog->t3 < dialog->t2) //time stamp overflow
    {
      diff_t3_t2 = (0xFFFFFFFFFFFF - dialog->t2) + dialog->t3;
    }
  else
    {
      diff_t3_t2 = dialog->t3 - dialog->t2;
    }
  rtt = diff_t4_t1 - diff_t3_t2;

  //need to remove the duration twice, because we received twice per dialog
  //and otherwise readings are 8us off
  rtt -= 2 * m_preamble_detection_duration;

  //add the error given by the current error model, by default error model is disabled
  rtt += m_ftm_error_model->GetFtmError();

  m_rtt_list.push_back (rtt);

  if (m_live_rtt_enabled)
    {
      live_rtt (rtt);
    }
}

bool
FtmSession::CheckTimestampSet (void)
{
  if (m_current_dialog == 0)
    {
      return true;
    }
  if (m_current_dialog->t1 == 0 || m_current_dialog->t4 == 0)
    {
      return false;
    }
  return true;
}

void
FtmSession::TriggerReceived (void)
{
  if (m_session_active && Simulator::Now() < m_current_burst_end)
    {
      SendNextFtmPacket ();
    }
//  else if (!m_session_active)
//    {
//      //ignore packet
//    }
}

void
FtmSession::EndSession (void)
{
  m_session_active = false;
  session_over_ftm_manager_callback (m_partner_addr);
//  if (m_session_type == FTM_RESPONDER) std::cout << test_value << std::endl;
//  if (m_session_over_callback_set && m_session_type == FTM_INITIATOR) //to fix break from session_override
  if (m_session_over_callback_set)
    {
      session_over_callback (*this);
    }
}

void
FtmSession::DenySession (void)
{
  Ptr<Packet> packet = Create<Packet> ();
  FtmResponseHeader ftm_res;
  FtmParams ftm_params;
  ftm_params.SetStatusIndication(FtmParams::REQUEST_INCAPABLE);
  ftm_res.SetFtmParams(ftm_params);
  packet->AddHeader(ftm_res);

  WifiActionHeader action_hdr;
  WifiActionHeader::ActionValue action;
  action.publicAction = WifiActionHeader::FTM_RESPONSE;
  action_hdr.SetAction(WifiActionHeader::PUBLIC_ACTION, action);
  packet->AddHeader(action_hdr);

  WifiMacHeader mac_hdr;
  mac_hdr.SetAddr1(m_partner_addr);
  send_packet (packet, mac_hdr);
  EndSession ();
}

void
FtmSession::SendTrigger (void)
{
  Ptr<Packet> packet = Create<Packet> ();
  FtmRequestHeader ftm_req;
  ftm_req.SetTrigger(1);
  packet->AddHeader(ftm_req);

  WifiActionHeader action_hdr;
  WifiActionHeader::ActionValue action;
  action.publicAction = WifiActionHeader::FTM_REQUEST;
  action_hdr.SetAction(WifiActionHeader::PUBLIC_ACTION, action);
  packet->AddHeader(action_hdr);

  WifiMacHeader mac_hdr;
  mac_hdr.SetAddr1(m_partner_addr);
  send_packet (packet, mac_hdr);
}

void
FtmSession::SetFtmErrorModel (Ptr<FtmErrorModel> error_model)
{
  m_ftm_error_model = error_model;
}

void
FtmSession::EnableLiveRTTFeedback (Callback<void, int64_t> callback)
{
  m_live_rtt_enabled = true;
  live_rtt = callback;
}

void
FtmSession::DisableLiveRTTFeedback (void)
{
  m_live_rtt_enabled = false;
}

void
FtmSession::SetOverrideCallback (Callback<void, Mac48Address, FtmRequestHeader> callback)
{
  session_override = callback;
}

} /* namespace ns3 */
