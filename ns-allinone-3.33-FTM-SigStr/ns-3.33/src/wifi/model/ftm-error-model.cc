/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (C) 2022 Christos Laskos
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
FtmErrorModel::GetFtmError (double sig_str)
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
WiredFtmErrorModel::GetFtmError (double sig_str)
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
WirelessFtmErrorModel::GetFtmError (double sig_str)
{
  if (m_node == 0 || m_map == 0)
    {
      return 0 + WiredFtmErrorModel::GetFtmError (sig_str);
    }
  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
  Vector position = mobility->GetPosition();

  double bias = m_map->GetBias(position.x, position.y);

  int error = bias + WiredFtmErrorModel::GetFtmError (sig_str);
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


NS_OBJECT_ENSURE_REGISTERED (WirelessSigStrFtmErrorModel);

TypeId
WirelessSigStrFtmErrorModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WirelessSigStrFtmErrorModel")
    .SetParent<WirelessFtmErrorModel> ()
    .SetGroupName ("FTM")
    .AddConstructor<WirelessSigStrFtmErrorModel>()
    ;
  return tid;
}

WirelessSigStrFtmErrorModel::WirelessSigStrFtmErrorModel()
{
  NS_LOG_FUNCTION (this);
  initDistributions ();
}

WirelessSigStrFtmErrorModel::WirelessSigStrFtmErrorModel(std::uint_least32_t seed)
: WirelessFtmErrorModel (seed) {
  NS_LOG_FUNCTION (this);
  initDistributions ();
}

WirelessSigStrFtmErrorModel::~WirelessSigStrFtmErrorModel()
{
  NS_LOG_FUNCTION (this);

}

int
WirelessSigStrFtmErrorModel::GetFtmError(double sig_str)
{
  int closest_sig_str = getClosestSigStr(sig_str);
  johnsonsuParams j_p = m_sig_str_map[closest_sig_str];

  double u = m_uniform_generator(m_generator);
  // for safety to not get exactly 0 and throw an error in the normal cdf inverse function
  while(u == 0.0) {
      u = m_uniform_generator(m_generator);
  }
  double phi_inv_u = NormalCDFInverse(u);
  double johnson_error_value = j_p.lamda * sinh((phi_inv_u - j_p.gamma) / j_p.delta) + j_p.xi;
  johnson_error_value = std::round(johnson_error_value);
  int error = (int) johnson_error_value;
//  std::cout << error << std::endl;

  return error + WirelessFtmErrorModel::GetFtmError(sig_str);
}

void
WirelessSigStrFtmErrorModel::initDistributions (void)
{
  // generator for values between 0 and 1
  m_uniform_generator = std::uniform_real_distribution<double> (0, 1);

  // set the distributions parameters for each signal strength
  johnsonsuParams sig_34_db;
  sig_34_db.gamma = 3.185354317604147;
  sig_34_db.delta = 5.478262165530669;
  sig_34_db.xi = 6607.306595955903;
  sig_34_db.lamda = 10570.049082397905;
  m_sig_str_map[-34] = sig_34_db;

  johnsonsuParams sig_54_db;
  sig_54_db.gamma = 5.244677635165819;
  sig_54_db.delta = 6.7849632493308984;
  sig_54_db.xi = 11337.621653529686;
  sig_54_db.lamda = 13422.49177763342;
  m_sig_str_map[-54] = sig_54_db;

  johnsonsuParams sig_57_db;
  sig_57_db.gamma = 11.526219535401548;
  sig_57_db.delta = 11.752569979080313;
  sig_57_db.xi = 23972.68246527834;
  sig_57_db.lamda = 21157.45764517224;
  m_sig_str_map[-57] = sig_57_db;

  johnsonsuParams sig_60_db;
  sig_60_db.gamma = 2.9557921354424597;
  sig_60_db.delta = 5.964536004213013;
  sig_60_db.xi = 7871.392874146462;
  sig_60_db.lamda = 16622.55659360096;
  m_sig_str_map[-60] = sig_60_db;

  johnsonsuParams sig_63_db;
  sig_63_db.gamma = 6.531332615907574;
  sig_63_db.delta = 9.919020162635562;
  sig_63_db.xi = 20472.998112906615;
  sig_63_db.lamda = 30324.20728528186;
  m_sig_str_map[-63] = sig_63_db;

  johnsonsuParams sig_66_db;
  sig_66_db.gamma = 1.5679223209733015;
  sig_66_db.delta = 2.651102194031285;
  sig_66_db.xi = 6467.335055986082;
  sig_66_db.lamda = 10780.685947891918;
  m_sig_str_map[-66] = sig_66_db;

  johnsonsuParams sig_69_db;
  sig_69_db.gamma = 1.1919025159806425;
  sig_69_db.delta = 1.7569740011989712;
  sig_69_db.xi = 5315.277549936767;
  sig_69_db.lamda = 9231.043791364496;
  m_sig_str_map[-69] = sig_69_db;

  johnsonsuParams sig_72_db;
  sig_72_db.gamma = 1.6836266134414828;
  sig_72_db.delta = 1.5382463243606552;
  sig_72_db.xi = 9816.63892830534;
  sig_72_db.lamda = 9491.487540612658;
  m_sig_str_map[-72] = sig_72_db;

  johnsonsuParams sig_74_db;
  sig_74_db.gamma = 4.689858735589265;
  sig_74_db.delta = 1.9587337465051236;
  sig_74_db.xi = 28439.426802970185;
  sig_74_db.lamda = 6570.588773099477;
  m_sig_str_map[-74] = sig_74_db;

  johnsonsuParams sig_75_db;
  sig_75_db.gamma = 5.456350713475308;
  sig_75_db.delta = 1.9477684103673694;
  sig_75_db.xi = 30054.10998533451;
  sig_75_db.lamda = 4864.245629274696;
  m_sig_str_map[-75] = sig_75_db;

  johnsonsuParams sig_76_db;
  sig_76_db.gamma = 7.139744646153247;
  sig_76_db.delta = 2.1710686890594744;
  sig_76_db.xi = 38988.747394210804;
  sig_76_db.lamda = 3882.820051622388;
  m_sig_str_map[-76] = sig_76_db;

  johnsonsuParams sig_77_db;
  sig_77_db.gamma = 8.262140730541272;
  sig_77_db.delta = 2.1345127965798465;
  sig_77_db.xi = 42133.22288891718;
  sig_77_db.lamda = 2389.29904971697;
  m_sig_str_map[-77] = sig_77_db;

  johnsonsuParams sig_78_db;
  sig_78_db.gamma = 8.522080144367578;
  sig_78_db.delta = 2.687266469105907;
  sig_78_db.xi = 64794.77378244052;
  sig_78_db.lamda = 7235.395990126872;
  m_sig_str_map[-78] = sig_78_db;

  johnsonsuParams sig_79_db;
  sig_79_db.gamma = 9.641640107814577;
  sig_79_db.delta = 3.0128025233396336;
  sig_79_db.xi = 81275.30052138754;
  sig_79_db.lamda = 8765.970931713084;
  m_sig_str_map[-79] = sig_79_db;

  johnsonsuParams sig_80_db;
  sig_80_db.gamma = 10.561011243252771;
  sig_80_db.delta = 3.34567183184721;
  sig_80_db.xi = 99724.65034040553;
  sig_80_db.lamda = 11258.496064080893;
  m_sig_str_map[-80] = sig_80_db;

  johnsonsuParams sig_81_db;
  sig_81_db.gamma = 15.27327368062722;
  sig_81_db.delta = 5.383312465271288;
  sig_81_db.xi = 190229.12414263037;
  sig_81_db.lamda = 27177.723635919916;
  m_sig_str_map[-81] = sig_81_db;

  johnsonsuParams sig_82_db;
  sig_82_db.gamma = 21.623359857149143;
  sig_82_db.delta = 7.2130572012096055;
  sig_82_db.xi = 282943.14579305204;
  sig_82_db.lamda = 33155.660042021365;
  m_sig_str_map[-82] = sig_82_db;
}

int
WirelessSigStrFtmErrorModel::getClosestSigStr (double sig_str)
{
  int sig_str_int = (int) std::round(sig_str);
  int closest = signal_strengths[0];
  for(int strength : signal_strengths)
    {
      if (std::abs(sig_str_int - strength) < std::abs(sig_str_int - closest))
      {
          closest = strength;
      }
    }
  return closest;
}

// source start here
// based on https://www.johndcook.com/blog/cpp_phi_inverse/
double
WirelessSigStrFtmErrorModel::RationalApproximation(double t)
{
    // Abramowitz and Stegun formula 26.2.23.
    // The absolute value of the error should be less than 4.5 e-4.
    double c[] = {2.515517, 0.802853, 0.010328};
    double d[] = {1.432788, 0.189269, 0.001308};
    return t - ((c[2]*t + c[1])*t + c[0]) /
               (((d[2]*t + d[1])*t + d[0])*t + 1.0);
}

double
WirelessSigStrFtmErrorModel::NormalCDFInverse(double p)
{
    if (p <= 0.0 || p >= 1.0)
    {
        std::stringstream os;
        os << "Invalid input argument for inverse normal CDF. (" << p
           << "); must be larger than 0 but less than 1.";
        NS_FATAL_ERROR( os.str() );
    }

    // See https://www.johndcook.com/blog/normal_cdf_inverse/ for explanation of this section.
    if (p < 0.5)
    {
        // F^-1(p) = - G^-1(p)
        return -RationalApproximation( sqrt(-2.0*log(p)) );
    }
    else
    {
        // F^-1(p) = G^-1(1-p)
        return RationalApproximation( sqrt(-2.0*log(1-p)) );
    }
}
// source end here

} /* namespace ns3 */
