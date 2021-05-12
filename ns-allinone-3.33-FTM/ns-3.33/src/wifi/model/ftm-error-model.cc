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

#include "ftm-error-model.h"
#include <ns3/mobility-model.h>
#include <ns3/pointer.h>
#include <ns3/double.h>
#include <ns3/enum.h>
#include <ns3/integer.h>
#include <fstream>
#include <algorithm>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FtmErrorModel");

NS_OBJECT_ENSURE_REGISTERED (FtmErrorModel);

TypeId
FtmErrorModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FtmErrorModel")
    .SetParent<Object> ()
    .SetGroupName ("FTM")
    .AddConstructor<FtmErrorModel>()
    ;
  return tid;
}

FtmErrorModel::FtmErrorModel ()
{
  NS_LOG_FUNCTION (this);
}

FtmErrorModel::~FtmErrorModel ()
{
  NS_LOG_FUNCTION (this);
}

int
FtmErrorModel::GetFtmError (void)
{
  return 0;
}


NS_OBJECT_ENSURE_REGISTERED (WiredFtmErrorModel);

TypeId
WiredFtmErrorModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WiredFtmErrorModel")
    .SetParent<FtmErrorModel> ()
    .SetGroupName ("FTM")
    .AddConstructor<WiredFtmErrorModel>()
    .AddAttribute("Mean",
                  "The mean to be used for the gaussian distribution.",
                  DoubleValue (0.0),
                  MakeDoubleAccessor (&WiredFtmErrorModel::SetMean,
                                      &WiredFtmErrorModel::GetMean),
                  MakeDoubleChecker<double> ())
    .AddAttribute("Standard_Deviation",
                  "The Standard Deviation to be used for the gaussion distribution.",
                  DoubleValue (2562.69),
                  MakeDoubleAccessor (&WiredFtmErrorModel::SetStandardDeviation,
                                      &WiredFtmErrorModel::GetStandardDeviation),
                  MakeDoubleChecker<double> ())
    .AddAttribute("Channel_Bandwidth",
                  "The Channel Bandwidth to be used for the error. "
                  "This changes the Standard Deviation to a real measured value.",
                  EnumValue (WiredFtmErrorModel::Channel_20_MHz),
                  MakeEnumAccessor (&WiredFtmErrorModel::SetChannelBandwidth),
                  MakeEnumChecker<ChannelBandwidth> (WiredFtmErrorModel::Channel_20_MHz, "Channel_20_MHz",
                                                     WiredFtmErrorModel::Channel_40_MHz, "Channel_40_MHz",
                                                     WiredFtmErrorModel::Channel_80_MHz, "Channel_80_MHz",
                                                     WiredFtmErrorModel::Channel_160_MHz, "Channel_160_MHz"))
    ;
  return tid;
}

WiredFtmErrorModel::WiredFtmErrorModel ()
{
  NS_LOG_FUNCTION (this);
  std::random_device random;
  m_seed = random ();
  m_generator = std::mt19937 (m_seed);
  m_gauss_dist = std::normal_distribution<double> (m_mean, m_standard_deviation_20MHz);
}

WiredFtmErrorModel::WiredFtmErrorModel (std::uint_least32_t seed)
{
  NS_LOG_FUNCTION (this);
  m_seed = seed;
  m_generator = std::mt19937 (m_seed);
  m_gauss_dist = std::normal_distribution<double> (m_mean, m_standard_deviation_20MHz);
}

WiredFtmErrorModel::~WiredFtmErrorModel ()
{
  NS_LOG_FUNCTION (this);
}

int
WiredFtmErrorModel::GetFtmError (void)
{
  return (int) m_gauss_dist(m_generator);
}

void
WiredFtmErrorModel::SetChannelBandwidth (ChannelBandwidth bandwidth)
{
  switch (bandwidth)
  {
    case Channel_20_MHz:
      m_standard_deviation = m_standard_deviation_20MHz;
      break;
    case Channel_40_MHz:
      m_standard_deviation = m_standard_deviation_40MHz;
      break;
    case Channel_80_MHz:
      return;
    case Channel_160_MHz:
      return;
  }
  m_mean = 0;
  UpdateDistribution ();
}

void
WiredFtmErrorModel::SetSeed (std::uint_least32_t seed)
{
  m_seed = seed;
  m_generator = std::mt19937 (m_seed);
}

std::uint_least32_t
WiredFtmErrorModel::GetSeed (void) const
{
  return m_seed;
}

void
WiredFtmErrorModel::SetMean (double mean)
{
  m_mean = mean;
  UpdateDistribution ();
}

double
WiredFtmErrorModel::GetMean (void) const
{
  return m_mean;
}

void
WiredFtmErrorModel::SetStandardDeviation (double sd)
{
  m_standard_deviation = sd;
  UpdateDistribution ();
}

double
WiredFtmErrorModel::GetStandardDeviation (void) const
{
  return m_standard_deviation;
}

void
WiredFtmErrorModel::UpdateDistribution (void)
{
  m_gauss_dist = std::normal_distribution<double> (m_mean, m_standard_deviation);
}


NS_OBJECT_ENSURE_REGISTERED (WirelessFtmErrorModel);

TypeId
WirelessFtmErrorModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WirelessFtmErrorModel")
    .SetParent<WiredFtmErrorModel> ()
    .SetGroupName ("FTM")
    .AddConstructor<WirelessFtmErrorModel>()
    .AddAttribute("FtmMap",
                  "The FtmMap from which to select bias."
                  "For valid results the Map should be big enough for all positions during Simulation.",
                  PointerValue (),
                  MakePointerAccessor (&WirelessFtmErrorModel::SetFtmMap,
                                       &WirelessFtmErrorModel::GetFtmMap),
                  MakePointerChecker<FtmMap> ())
    ;
  return tid;
}

WirelessFtmErrorModel::WirelessFtmErrorModel ()
{
  NS_LOG_FUNCTION (this);

  m_node = 0;
  m_map = 0;
}

WirelessFtmErrorModel::WirelessFtmErrorModel (std::uint_least32_t seed)
: WiredFtmErrorModel (seed)
{
  NS_LOG_FUNCTION (this);

  m_node = 0;
  m_map = 0;
}

WirelessFtmErrorModel::~WirelessFtmErrorModel()
{
  NS_LOG_FUNCTION (this);
}

int
WirelessFtmErrorModel::GetFtmError (void)
{
  if (m_node == 0 || m_map == 0)
    {
      return 0;
    }
  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
  Vector position = mobility->GetPosition();

  double bias = m_map->GetBias(position.x, position.y);

  int error = bias + WiredFtmErrorModel::GetFtmError ();
  return error;
}

void
WirelessFtmErrorModel::SetFtmMap (Ptr<FtmMap> map)
{
  m_map = map;
}

Ptr<WirelessFtmErrorModel::FtmMap>
WirelessFtmErrorModel::GetFtmMap (void) const
{
  return m_map;
}

void
WirelessFtmErrorModel::SetNode (Ptr<Node> node)
{
  m_node = node;
}

Ptr<Node>
WirelessFtmErrorModel::GetNode (void)
{
  return m_node;
}

//NS_OBJECT_ENSURE_REGISTERED (WirelessFtmErrorModel::FtmMap); //does not work for some reason

TypeId
WirelessFtmErrorModel::FtmMap::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WirelessFtmErrorModel::FtmMap")
    .SetParent<Object> ()
    .SetGroupName ("FTM")
    .AddConstructor<WirelessFtmErrorModel::FtmMap>()
    ;
  return tid;
}

WirelessFtmErrorModel::FtmMap::FtmMap ()
{
  NS_LOG_FUNCTION (this);

  map = 0;

  xmin = 0;
  xmax = 0;
  ymin = 0;
  ymax = 0;
  resolution = 0;
  xsize = 0;
  ysize = 0;
}

WirelessFtmErrorModel::FtmMap::~FtmMap ()
{
  NS_LOG_FUNCTION (this);

  delete [] map;
}

void
WirelessFtmErrorModel::FtmMap::LoadMap (std::string filename)
{
  std::ifstream file (filename);
  if (!file.is_open ())
    {
      file.close();
      NS_FATAL_ERROR ("Specified map file can not be opened!");
      return;
    }
  std::string line;
  std::getline(file, line);

  line.erase(std::remove_if (line.begin(), line.end(), [](unsigned char x){return std::isspace(x);}), line.end());
  auto begin = line.begin();
  begin++; //first character is not important
  double *header = new double [7];
  int i = 0;
  std::string tmp = "";
  bool save = false;
  for (auto it = begin; it != line.end(); ++it)
    {
      if (*it == '=')
        {
          save = true;
          continue;
        }
      if (*it == ',')
        {
          save = false;
          header[i] = std::stod(tmp);
          tmp = "";
          ++i;
        }
      if(save)
        {
          tmp += *it;
        }
    }
  header[i] = std::stod(tmp);

  xmin = header[0];
  xmax = header[1];
  ymin = header[2];
  ymax = header[3];
  resolution = header[6];

  xsize = ((xmax - xmin) / resolution) + 1;
  ysize = ((ymax - ymin) / resolution) + 1;
  map = new double [xsize * ysize];

  std::getline(file, line);
  int y = 0;
  while(std::getline(file, line))
    {
      int x = 0;
      std::string tmp = "";
      for (auto it = line.begin(); it != line.end(); ++it)
        {
          if (*it == ' ')
            {
              map [y * xsize + x] = std::stod (tmp);
              tmp = "";
              ++x;
            }
          else
            {
              tmp += *it;
            }
        }
      map [y * xsize + x] = std::stod (tmp);
      ++y;
    }
  file.close();
}

double
WirelessFtmErrorModel::FtmMap::GetBias (double x, double y)
{
  if (x < xmin || y < ymin || x > xmax || y > ymax)
    {
      return 0.0;
    }

  int x_val = (std::abs(xmin - x)) / resolution;
  int y_val = (std::abs(ymax - y)) / resolution;

  return map[y_val * xsize + x_val];
}

} /* namespace ns3 */
