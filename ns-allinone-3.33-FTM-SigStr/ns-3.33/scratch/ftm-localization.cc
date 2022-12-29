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

NS_LOG_COMPONENT_DEFINE ("FtmLocalization");

Ptr<WirelessFtmErrorModel::FtmMap> map;

int selected_error_mode = 0; //0: wired, 1: wireless, 2: wireless sig_str, 3: wireless_sig_str with fading
std::string file_name = "ftm_localization/tmp.txt";

std::mt19937 gen;
std::uniform_int_distribution<> dist;

uint8_t curr_position_num = 0;
const uint8_t total_positions = 10;
double x_positions[10] = {};
double y_positions[10] = {};

std::list<std::tuple<int64_t, double, double, double>> measurements; //saving RTT, sig str, x pos, y pos

void SessionOver (FtmSession session)
{
  std::list<int64_t> rtts = session.GetIndividualRTT();
  std::list<double> sig_strs = session.GetIndividualSignalStrength();

  while (!rtts.empty())
    {
      std::tuple<int64_t, double, double, double> tmp_data_point =
          std::make_tuple(rtts.front(), sig_strs.front(), x_positions[curr_position_num-1], y_positions[curr_position_num-1]);
      measurements.push_back(tmp_data_point);

      rtts.pop_front();
      sig_strs.pop_front();
    }
}

void ChangePosition (Ptr<Node> sta) {
  Ptr<MobilityModel> mobility = sta->GetObject<MobilityModel>();
  Vector position = mobility->GetPosition();

  double x = (double) dist(gen) / 100; //convert cm to m
  double y = (double) dist(gen) / 100; //convert cm to m

  double distance_ap = sqrt(pow(x, 2) + pow(y, 2)); //calc distance to AP at 0,0

  bool too_close = false;
  //calc distance between current new position and all previous ones, if less then 1, take a new one
  for(int i = 0; i < curr_position_num; i++)
    {
      double distance_point = sqrt(pow(x - x_positions[i], 2) + pow(y - y_positions[i], 2));
      if (distance_point < 1)
        {
          too_close = true;
          break;
        }
    }

  if(distance_ap < 1 || too_close)
    {
      ChangePosition (sta);
      return;
    }

  x_positions[curr_position_num] = x;
  y_positions[curr_position_num] = y;

  position.x = x;
  position.y = y;

  curr_position_num++;

  mobility->SetPosition(position);
}

static void GenerateTraffic (Ptr<WifiNetDevice> ap, Ptr<WifiNetDevice> sta, Address recvAddr)
{
  ChangePosition (sta->GetNode ());

  Ptr<RegularWifiMac> sta_mac = sta->GetMac()->GetObject<RegularWifiMac>();

  Mac48Address to = Mac48Address::ConvertFrom (recvAddr);

  Ptr<FtmSession> session = sta_mac->NewFtmSession(to);
  if (session == 0)
    {
      NS_FATAL_ERROR ("ftm not enabled");
    }

  Ptr<FtmErrorModel> error_model;
  if (selected_error_mode == 0) {
      //create the wired error model
      Ptr<WiredFtmErrorModel> wired_error = CreateObject<WiredFtmErrorModel> ();
      wired_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);
      error_model = wired_error;
  }
  else if (selected_error_mode == 1) {
      //create wireless error model
      //map has to be created prior
      Ptr<WirelessFtmErrorModel> wireless_error = CreateObject<WirelessFtmErrorModel> ();
      wireless_error->SetFtmMap(map);
      wireless_error->SetNode(sta->GetNode());
      wireless_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);
      error_model = wireless_error;
  }
  else if (selected_error_mode == 2 || selected_error_mode == 3) {
      //create wireless signal strength error model
      //map has to be created prior
      Ptr<WirelessSigStrFtmErrorModel> wireless_sig_str_error = CreateObject<WirelessSigStrFtmErrorModel> ();
      wireless_sig_str_error->SetFtmMap(map);
      wireless_sig_str_error->SetNode(sta->GetNode());
      wireless_sig_str_error->SetChannelBandwidth(WiredFtmErrorModel::Channel_20_MHz);
      error_model = wireless_sig_str_error;
  }


  //using wired error model in this case
  session->SetFtmErrorModel(error_model);

  //create the parameter for this session and set them
  FtmParams ftm_params;
  ftm_params.SetStatusIndication(FtmParams::RESERVED);
  ftm_params.SetStatusIndicationValue(0);
  ftm_params.SetNumberOfBurstsExponent(1); //2 bursts
  ftm_params.SetBurstDuration(9); //32 ms burst duration, this needs to be larger due to long processing delay until transmission

  ftm_params.SetMinDeltaFtm(1); //100 us between frames
  ftm_params.SetPartialTsfNoPref(true);
  ftm_params.SetAsap(true);
  ftm_params.SetFtmsPerBurst(20);

  ftm_params.SetBurstPeriod(10); //1000 ms between burst periods
  session->SetFtmParams(ftm_params);

  session->SetSessionOverCallback(MakeCallback(&SessionOver));
  session->SessionBegin();

  if (curr_position_num < total_positions)
    {
      Simulator::Schedule(Seconds (5), &GenerateTraffic, ap, sta, recvAddr);
    }
}

int main (int argc, char *argv[])
{
  //double rss = -80;  // -dBm
  std::uint_least32_t seed = 13;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("error", "Currently Selected Error Mode", selected_error_mode);
  cmd.AddValue ("filename", "Used File Name for Saving", file_name);
  cmd.AddValue ("seed", "Seed for Position Generation", seed);
  cmd.Parse (argc, argv);

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
  wifiPhy.Set ("TxPowerStart", DoubleValue(14));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(14));
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

//  wifiChannel.AddPropagationLoss ("ns3::ThreeGppIndoorOfficePropagationLossModel");

  if (selected_error_mode == 0 || selected_error_mode == 1) {
      // The below FixedRssLossModel will cause the rss to be fixed regardless
      // of the distance between the two stations, and the transmit power
      wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (-40));
  }
  else if (selected_error_mode == 2) {
      wifiChannel.AddPropagationLoss ("ns3::ThreeLogDistancePropagationLossModel");
  }
  else if (selected_error_mode == 3) {
      wifiChannel.AddPropagationLoss ("ns3::ThreeLogDistancePropagationLossModel");
      wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
  }
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
  positionAlloc->Add (Vector (2, 0, 0));
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
  if (selected_error_mode != 0) {
      map->LoadMap ("src/wifi/ftm_map/dim_62.map");
  }

//  std::random_device rd; // obtain a random number from hardware
  gen = std::mt19937 (seed); // seed the generator
  dist = std::uniform_int_distribution<> (-3000, 3000); // define the range in cm, -30m to 30m


  // Tracing
//  wifiPhy.EnablePcap ("ftm-localization", devices);

  Simulator::ScheduleNow (&GenerateTraffic, wifi_ap, wifi_sta, recvAddr);

  //set time resolution to pico seconds for the time stamps, as default is in nano seconds. IMPORTANT
  Time::SetResolution(Time::PS);

  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  std::ofstream output (file_name);
  output << "#rtt sig_str x y" << "\n";
  while (!measurements.empty())
    {
      auto current = measurements.front();
      output << std::get<0>(current) << " "
          << std::get<1>(current) << " "
          << std::get<2>(current) << " "
          << std::get<3>(current) << "\n";
      measurements.pop_front();
    }
  output.close();

  return 0;
}
