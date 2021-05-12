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


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LaterationTest");

std::list<int64_t> rtts;
Ptr<WirelessFtmErrorModel::FtmMap> map;
int position_num;
std::list<double> x_pos;
std::list<double> y_pos;
std::mt19937 gen;
std::uniform_int_distribution<> dist;

uint8_t positions;
uint8_t ftms;
uint8_t bursts;
std::string error_model;
WiredFtmErrorModel::ChannelBandwidth bandwidth;

void SessionOver (FtmSession session)
{
  rtts.push_back (session.GetMeanRTT());
}

void ChangePosition (Ptr<Node> sta) {
  Ptr<MobilityModel> mobility = sta->GetObject<MobilityModel>();
  Vector position = mobility->GetPosition();

  double x = (double) dist(gen) / 100;
  double y = (double) dist(gen) / 100;

  double distance_ap = sqrt(pow(x, 2) + pow(y, 2));

  bool too_close = false;
  auto x_it = x_pos.begin();
  auto y_it = y_pos.begin();
  for(unsigned int i = 0; i < x_pos.size(); ++i)
    {
      double x_curr = *x_it;
      double y_curr = *y_it;
      double distance_point = sqrt(pow(x - x_curr, 2) + pow(y - y_curr, 2));
      if (distance_point < 1)
        {
          too_close = true;
          break;
        }
      std::advance(x_it, 1);
      std::advance(y_it, 1);
    }

  if(distance_ap < 1 || too_close)
    {
      ChangePosition (sta);
      return;
    }

  x_pos.push_back (x);
  y_pos.push_back (y);

  position.x = x;
  position.y = y;

  position_num++;

  mobility->SetPosition(position);
}

static void GenerateTraffic (Ptr<WifiNetDevice> sta, Address recvAddr)
{
  ChangePosition (sta->GetNode ());

  Ptr<RegularWifiMac> sta_mac = sta->GetMac()->GetObject<RegularWifiMac>();

  Mac48Address to = Mac48Address::ConvertFrom (recvAddr);

  Ptr<FtmSession> session = sta_mac->NewFtmSession(to);
  if (session == 0)
    {
      NS_FATAL_ERROR ("ftm not enabled");
    }

  Ptr<FtmErrorModel> ftm_error_model;

  if (error_model == "wired")
    {
        Ptr<WiredFtmErrorModel> wired_error = CreateObject<WiredFtmErrorModel> ();
        wired_error->SetChannelBandwidth(bandwidth);
        ftm_error_model = wired_error;
    }
  else if (error_model == "wireless")
    {
      Ptr<WirelessFtmErrorModel> wireless_error = CreateObject<WirelessFtmErrorModel> ();
      wireless_error->SetFtmMap(map);
      wireless_error->SetNode(sta->GetNode());
      wireless_error->SetChannelBandwidth(bandwidth);
      ftm_error_model = wireless_error;
    }
  else
    {
      NS_FATAL_ERROR ("Unrecognized error model.");
      return;
    }

  session->SetFtmErrorModel(ftm_error_model);

  FtmParams ftm_params;
  ftm_params.SetStatusIndication(FtmParams::RESERVED);
  ftm_params.SetStatusIndicationValue(0);
  ftm_params.SetNumberOfBurstsExponent(bursts); //4 bursts
  ftm_params.SetBurstDuration(9); //4 ms burst duration, this needs to be larger due to long processing delay until transmission

  ftm_params.SetMinDeltaFtm(4); //400 us between frames
  ftm_params.SetPartialTsfTimer(0);
  ftm_params.SetPartialTsfNoPref(false);
  ftm_params.SetAsapCapable(false);
  ftm_params.SetAsap(true);
  ftm_params.SetFtmsPerBurst(ftms);

  ftm_params.SetFormatAndBandwidth(0);
  ftm_params.SetBurstPeriod(0);
  if (bursts > 0)
    {
      ftm_params.SetBurstPeriod(4); //400 ms between burst periods
    }
  session->SetFtmParams(ftm_params);

  session->SetSessionOverCallback(MakeCallback(&SessionOver));
  session->SessionBegin();

  if (position_num < positions - 1)
    {
      Simulator::Schedule(Seconds (5), &GenerateTraffic, sta, recvAddr);
    }
}

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double rss = -80;  // -dBm
  bool verbose = false;
  positions = 10;
  std::uint_least32_t seed = 13;
  ftms = 2;
  bursts = 0;
  error_model = "wired";
  int bandwidth_value = 20;
  std::string map_path = "";
  int simulation_duration = 60;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("positions", "number of positions", positions);
  cmd.AddValue ("seed", "Seed for the position generator", seed);
  cmd.AddValue ("ftms", "FTMs per burst", ftms);
  cmd.AddValue ("bursts", "Number of bursts exponent", bursts);
  cmd.AddValue ("errorModel", "FTM error model. Wired or Wireless.", error_model);
  cmd.AddValue ("bandwidth", "20 or 40 MHz for Wired error model", bandwidth_value);
  cmd.AddValue ("map", "The FTM map.", map_path);
  cmd.AddValue ("simDuration", "Simulation duration", simulation_duration);
  cmd.Parse (argc, argv);

  if (bandwidth_value == 40)
    {
      bandwidth = WiredFtmErrorModel::Channel_40_MHz;
    }
  else
    {
      bandwidth = WiredFtmErrorModel::Channel_20_MHz;
    }

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (2);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }
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
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

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
  positionAlloc->Add (Vector (15, 0, 0));
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

  Ptr<RegularWifiMac> ap_mac = wifi_ap->GetMac()->GetObject<RegularWifiMac>();
  Ptr<RegularWifiMac> sta_mac = wifi_sta->GetMac()->GetObject<RegularWifiMac>();
  ap_mac->EnableFtm();
  sta_mac->EnableFtm();

  position_num = 0;
//  std::random_device rd; // obtain a random number from hardware
  gen = std::mt19937 (seed); // seed the generator
  dist = std::uniform_int_distribution<> (-1000, 1000); // define the range

  if (error_model == "wireless")
    {
      if (map_path != "")
        {
          map = CreateObject<WirelessFtmErrorModel::FtmMap> ();
          map->LoadMap (map_path);
        }
      else
        {
          NS_FATAL_ERROR ("Map file not specified when usingg wireless error model.");
        }
    }

  // Tracing
  wifiPhy.EnablePcap ("ftm-lateration", devices);

  // Output what we are doing
  NS_LOG_UNCOND ("Testing " << (int) ftms  << " FTMs with " << bandwidth_value
                 << " MHz " << error_model << " error model.");

  Simulator::ScheduleNow (&GenerateTraffic, wifi_sta, recvAddr);
  Simulator::Schedule (Seconds (100), &GenerateTraffic, wifi_sta, recvAddr);


  //set time resolution to pico seconds for the timestamps, as default is in nano seconds TODO important
  Time::SetResolution(Time::PS);

  Simulator::Stop (Seconds (simulation_duration));
  Simulator::Run ();
  Simulator::Destroy ();

  std::string file_name = "ftm_lateration/" + error_model + "_" + std::to_string(bandwidth_value) + "mhz_"
                          + std::to_string(ftms) + "ftms_" + std::to_string(seed) + "seed.csv";
  std::ofstream output (file_name);
  output << "rtt;x;y" << std::endl;
  auto x_it = x_pos.begin();
  auto y_it = y_pos.begin();
  for (int64_t rtt : rtts)
    {
      output << rtt << ";" << *x_it << ";" << *y_it << std::endl;
      std::advance(x_it, 1);
      std::advance(y_it, 1);
    }
  output.close();


  return 0;
}
