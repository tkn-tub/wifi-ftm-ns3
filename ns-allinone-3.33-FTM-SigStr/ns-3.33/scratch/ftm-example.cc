/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * Based on the "wifi-simple-infra.cc" example.
 * Modified by Christos Laskos.
 * 2022
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/ftm-header.h"
#include "ns3/mgt-headers.h"
#include "ns3/ftm-error-model.h"
#include "ns3/pointer.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FtmExample");

void SessionOver (FtmSession session)
{
  NS_LOG_UNCOND ("RTT: " << session.GetMeanRTT ());
//  std::cout << "RTT: " << session.GetMeanRTT () << std::endl;
  std::cout << "Mean Signal Strength: " << session.GetMeanSignalStrength () << std::endl;
  std::cout << "RTT list size: " << session.GetIndividualRTT().size() << std::endl;
}

Ptr<WirelessFtmErrorModel::FtmMap> map;

static void GenerateTraffic (Ptr<WifiNetDevice> ap, Ptr<WifiNetDevice> sta, Address recvAddr)
{
  Ptr<RegularWifiMac> sta_mac = sta->GetMac()->GetObject<RegularWifiMac>();

  Mac48Address to = Mac48Address::ConvertFrom (recvAddr);

  Ptr<FtmSession> session = sta_mac->NewFtmSession(to);
  if (session == 0)
    {
      NS_FATAL_ERROR ("ftm not enabled");
    }

  //create the wired error model
//  Ptr<WiredFtmErrorModel> wired_error = CreateObject<WiredFtmErrorModel> ();
//  wired_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);

  //create wireless error model
  //map has to be created prior
//  Ptr<WirelessFtmErrorModel> wireless_error = CreateObject<WirelessFtmErrorModel> ();
//  wireless_error->SetFtmMap(map);
//  wireless_error->SetNode(sta->GetNode());
//  wireless_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);

  //create wireless signal strength error model
  //map has to be created prior
  Ptr<WirelessSigStrFtmErrorModel> wireless_sig_str_error = CreateObject<WirelessSigStrFtmErrorModel> ();
  wireless_sig_str_error->SetFtmMap(map);
  wireless_sig_str_error->SetNode(sta->GetNode());
  wireless_sig_str_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);

  //using wired error model in this case
  session->SetFtmErrorModel(wireless_sig_str_error);

  //create the parameter for this session and set them
  FtmParams ftm_params;
  ftm_params.SetStatusIndication(FtmParams::RESERVED);
  ftm_params.SetStatusIndicationValue(0);
  ftm_params.SetNumberOfBurstsExponent(7); //128 bursts
  ftm_params.SetBurstDuration(6); //4 ms burst duration, this needs to be larger due to long processing delay until transmission

  ftm_params.SetMinDeltaFtm(1); //100 us between frames
  ftm_params.SetPartialTsfNoPref(true);
  ftm_params.SetAsap(true);
  ftm_params.SetFtmsPerBurst(2);

  ftm_params.SetBurstPeriod(1); //100 ms between burst periods
  session->SetFtmParams(ftm_params);

  session->SetSessionOverCallback(MakeCallback(&SessionOver));
  session->SessionBegin();
}

int main (int argc, char *argv[])
{
  //double rss = -80;  // -dBm

  //enable FTM through attribute system
  Config::SetDefault ("ns3::RegularWifiMac::FTM_Enabled", BooleanValue(true));

  NodeContainer c;
  c.Create (2);

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211n_2_4GHZ);

  YansWifiPhyHelper wifiPhy;
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  //wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  wifiChannel.AddPropagationLoss ("ns3::ThreeGppIndoorOfficePropagationLossModel");
//  wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");

  // Setup the rest of the mac
  Ssid ssid = Ssid ("wifi-default");
  // setup sta.
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid));
//  wifiMac.SetType ("ns3::StaWifiMac");
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, c.Get (0));
  NetDeviceContainer devices = staDevice;
  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, c.Get (1));
  devices.Add (apDevice);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (5, 0, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install(c.Get (0));

  Ptr<ListPositionAllocator> positionAlloc2 = CreateObject<ListPositionAllocator> ();
  positionAlloc2->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc2);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c.Get(1));

  Ptr<NetDevice> ap = apDevice.Get(0);
  Ptr<NetDevice> sta = staDevice.Get(0);
  Address recvAddr = ap->GetAddress();

  //convert net device to wifi net device
  Ptr<WifiNetDevice> wifi_ap = ap->GetObject<WifiNetDevice>();
  Ptr<WifiNetDevice> wifi_sta = sta->GetObject<WifiNetDevice>();
  //enable FTM through the MAC object
//  Ptr<RegularWifiMac> ap_mac = wifi_ap->GetMac()->GetObject<RegularWifiMac>();
//  Ptr<RegularWifiMac> sta_mac = wifi_sta->GetMac()->GetObject<RegularWifiMac>();
//  ap_mac->EnableFtm();
//  sta_mac->EnableFtm();

  //load FTM map for usage
  map = CreateObject<WirelessFtmErrorModel::FtmMap> ();
  map->LoadMap ("src/wifi/ftm_map/dim_16.map");
  //set FTM map through attribute system
//  Config::SetDefault ("ns3::WirelessFtmErrorModel::FtmMap", PointerValue (map));

  // Tracing
  wifiPhy.EnablePcap ("ftm-example", devices);

  Simulator::ScheduleNow (&GenerateTraffic, wifi_ap, wifi_sta, recvAddr);

  //set the default FTM params through the attribute system
//  FtmParams ftm_params;
//  ftm_params.SetNumberOfBurstsExponent(1); //2 bursts
//  ftm_params.SetBurstDuration(7); //8 ms burst duration, this needs to be larger due to long processing delay until transmission
//
//  ftm_params.SetMinDeltaFtm(10); //1000 us between frames
//  ftm_params.SetPartialTsfNoPref(true);
//  ftm_params.SetAsap(true);
//  ftm_params.SetFtmsPerBurst(2);
//  ftm_params.SetBurstPeriod(10); //1000 ms between burst periods
//  Ptr<FtmParamsHolder> holder = CreateObject<FtmParamsHolder>();
//  holder->SetFtmParams(ftm_params);
//  Config::SetDefault("ns3::FtmSession::DefaultFtmParams", PointerValue(holder));

  //set time resolution to pico seconds for the time stamps, as default is in nano seconds. IMPORTANT
  Time::SetResolution(Time::PS);

  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
