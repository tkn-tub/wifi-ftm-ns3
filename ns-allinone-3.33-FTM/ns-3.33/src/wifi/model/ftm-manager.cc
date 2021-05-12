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

#include "ftm-manager.h"
#include "ns3/core-module.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FtmManager");

NS_OBJECT_ENSURE_REGISTERED (FtmManager);

TypeId
FtmManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FtmManager")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<FtmManager>()
    ;
  return tid;
}

FtmManager::FtmManager ()
{
  received_packets = 0;
  awaiting_ack = false;
  sent_packets = 0;
  sending_ack = false;
}

FtmManager::FtmManager (Ptr<WifiPhy> phy, Ptr<Txop> txop)
{
  received_packets = 0;
  awaiting_ack = false;
  sent_packets = 0;
  sending_ack = false;
  phy->TraceConnectWithoutContext("PhyTxBegin", MakeCallback(&FtmManager::PhyTxBegin, this));
  phy->TraceConnectWithoutContext("PhyRxBegin", MakeCallback(&FtmManager::PhyRxBegin, this));
  m_txop = txop;

  m_preamble_detection_duration = phy->GetPreambleDetectionDuration();
}

FtmManager::~FtmManager ()
{
  TraceDisconnectWithoutContext("PhyTxBegin", MakeCallback(&FtmManager::PhyTxBegin, this));
  TraceDisconnectWithoutContext("PhyRxBegin", MakeCallback(&FtmManager::PhyRxBegin, this));
  sessions.clear();
  m_blocked_partners.clear();
  m_txop = 0;
}

void
FtmManager::PhyTxBegin(Ptr<const Packet> packet, double num)
{

  Time now = Simulator::Now();
  int64_t pico_sec = now.GetPicoSeconds();
  pico_sec &= 0x0000FFFFFFFFFFFF;
  sent_packets++;
  Ptr<Packet> copy = packet->Copy();
  WifiMacHeader hdr;
  copy->RemoveHeader(hdr);
  if(hdr.IsMgt() && hdr.IsAction()) {
      WifiActionHeader action_hdr;
      copy->RemoveHeader(action_hdr);
      if(action_hdr.GetCategory() == WifiActionHeader::PUBLIC_ACTION) {
          WifiActionHeader::ActionValue action = action_hdr.GetAction();
          if (action.publicAction == WifiActionHeader::FTM_RESPONSE)
            {
              Ptr<FtmSession> session = FindSession (hdr.GetAddr1());
              if (session != 0)
                {
                  FtmResponseHeader ftm_resp_hdr;
                  copy->RemoveHeader(ftm_resp_hdr);
                  session->SetT1(ftm_resp_hdr.GetDialogToken(), pico_sec);

                  received_packets = 0;
                  awaiting_ack = true;

                  PacketInPieces pieces;
                  pieces.mac_hdr = hdr;
                  pieces.action_hdr = action_hdr;
                  pieces.ftm_res_hdr = ftm_resp_hdr;
                  m_current_tx_packet = pieces;
                }
            }
      }
  }
  else if(hdr.IsAck()) {
      if(sending_ack && sent_packets == 1) {
          if(m_ack_to == hdr.GetAddr1()) {
              sending_ack = false;
              Ptr<FtmSession> session = FindSession (m_current_rx_packet.mac_hdr.GetAddr2());
              if (session != 0)
                {
                  session->SetT3(m_current_rx_packet.ftm_res_hdr.GetDialogToken(), pico_sec);
                }
          }
      }
  }
}

void
FtmManager::PhyRxBegin(Ptr<const Packet> packet, RxPowerWattPerChannelBand rxPowersW)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now();
  int64_t pico_sec = now.GetPicoSeconds();
  pico_sec &= 0x0000FFFFFFFFFFFF;
  Ptr<Packet> copy = packet->Copy();
  received_packets++;
  WifiMacHeader hdr;
  copy->RemoveHeader(hdr);
  if(hdr.GetAddr1() == m_mac_address){
      if(hdr.IsMgt() && hdr.IsAction()) {
          WifiActionHeader action_hdr;
          copy->RemoveHeader(action_hdr);
          if(action_hdr.GetCategory() == WifiActionHeader::PUBLIC_ACTION) {
              Mac48Address partner = hdr.GetAddr2();
              if(action_hdr.GetAction().publicAction == WifiActionHeader::FTM_RESPONSE) {
                  sending_ack = true;
                  sent_packets = 0;
                  m_ack_to = partner;

                  FtmResponseHeader ftm_res_hdr;
                  copy->RemoveHeader(ftm_res_hdr);

                  Ptr<FtmSession> session = FindSession(partner);
                  if (session != 0 && ftm_res_hdr.GetDialogToken() != 0)
                    {
                      session->SetT2(ftm_res_hdr.GetDialogToken(), pico_sec);
                      PacketInPieces pieces;
                      pieces.mac_hdr = hdr;
                      pieces.action_hdr = action_hdr;
                      pieces.ftm_res_hdr = ftm_res_hdr;
                      m_current_rx_packet = pieces;
                    }
              }
          }
      }
      if(hdr.IsAck()) {
          if(awaiting_ack && received_packets == 1) {
              awaiting_ack = false;
              Ptr<FtmSession> session = FindSession (m_current_tx_packet.mac_hdr.GetAddr1());
              if (session != 0)
                {
                  session->SetT4(m_current_tx_packet.ftm_res_hdr.GetDialogToken(), pico_sec);
                }
          }
          else if(awaiting_ack && received_packets > 1) { //this needs to be checked also for non ack, cause if ack never arrives but other packet, its still an error
              awaiting_ack = false;
          }
      }
  }
}

void
FtmManager::SetMacAddress(Mac48Address addr)
{
  m_mac_address = addr;
}

Ptr<FtmSession>
FtmManager::CreateNewSession (Mac48Address partner, FtmSession::SessionType type)
{
  if(FindSession(partner) == 0 && !CheckSessionBlocked (partner) && partner != m_mac_address)
    {
      Ptr<FtmSession> new_session = CreateObject<FtmSession> ();
      new_session->InitSession(partner, type, MakeCallback(&FtmManager::SendPacket, this));
      new_session->SetSessionOverCallbackManager(MakeCallback(&FtmManager::SessionOver, this));
      new_session->SetBlockSessionCallback(MakeCallback(&FtmManager::BlockSession, this));
      new_session->SetOverrideCallback(MakeCallback(&FtmManager::OverrideSession, this));
      new_session->SetPreambleDetectionDuration(m_preamble_detection_duration);
      sessions.insert({partner, new_session});
      return new_session;
    }
  return 0;
}

void
FtmManager::SendPacket (Ptr<Packet> packet, WifiMacHeader hdr)
{
  hdr.SetType(WIFI_MAC_MGT_ACTION);
  hdr.SetAddr2(m_mac_address);
  hdr.SetAddr3(m_mac_address);
  hdr.SetDsNotTo();
  hdr.SetDsNotFrom();
//  hdr.SetNoRetry();
  m_txop->Queue(packet, hdr);
}

Ptr<FtmSession>
FtmManager::FindSession (Mac48Address addr)
{
  auto search = sessions.find(addr);
  if (search != sessions.end())
    {
      return search->second;
    }
  return 0;
}

void
FtmManager::SessionOver (Mac48Address addr)
{
  sessions.erase (addr);
}

void
FtmManager::ReceivedFtmRequest (Mac48Address partner, FtmRequestHeader ftm_req)
{
  Ptr<FtmSession> session = FindSession (partner);
  if (session == 0)
    {
      session = CreateNewSession(partner, FtmSession::FTM_RESPONDER);
    }
  session->ProcessFtmRequest(ftm_req);
}

void
FtmManager::ReceivedFtmResponse (Mac48Address partner, FtmResponseHeader ftm_res)
{
  Ptr<FtmSession> session = FindSession (partner);
  if (session != 0)
    {
      session->ProcessFtmResponse(ftm_res);
    }
}


void
FtmManager::BlockSession (Mac48Address partner, Time duration)
{
  m_blocked_partners.push_back (partner);
  Simulator::Schedule(duration, &FtmManager::UnblockSession, this, partner);
}

void
FtmManager::UnblockSession (Mac48Address partner)
{
  m_blocked_partners.remove (partner);
}

bool
FtmManager::CheckSessionBlocked (Mac48Address partner)
{
  for (Mac48Address addr : m_blocked_partners)
    {
      if (addr == partner)
        {
          return true;
        }
    }
  return false;
}

void
FtmManager::OverrideSession (Mac48Address partner, FtmRequestHeader ftm_req)
{
  std::cout << "override" << std::endl;
  Ptr<FtmSession> session = CreateNewSession (partner, FtmSession::FTM_RESPONDER);
  session->ProcessFtmRequest (ftm_req);
}

}
