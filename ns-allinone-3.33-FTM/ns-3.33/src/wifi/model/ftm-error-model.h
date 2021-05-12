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

#ifndef FTM_ERROR_MODEL_H_
#define FTM_ERROR_MODEL_H_

#include <ns3/object.h>
#include <random>
#include <ns3/node.h>

namespace ns3 {

/**
 * \brief base class for all FTM Error models
 * \ingroup FTM
 *
 * This Class defines the Base for the FTM Error models. It is used as the default for all
 * FtmSession objects without any error.
 */
class FtmErrorModel : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  FtmErrorModel ();
  virtual ~FtmErrorModel ();

  /**
   * Virtual method to retrieve the error. Used by child classes.
   *
   * \return always returns 0 as by default no error model is used.
   */
  virtual int GetFtmError (void);
};

/**
 * \brief Error model based on wired channel.
 * \ingroup FTM
 *
 * This class models a wired channel. The error is calculated with a gaussian distribution.
 * Default values are based on measured data with a coax cable. Implemented Bandwidths are 20
 * and 40 MHz.
 */
class WiredFtmErrorModel : public FtmErrorModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  WiredFtmErrorModel ();
  /**
   * Creates the WiredFtmErrorModel with the specified seed.
   * \param seed the seed
   */
  WiredFtmErrorModel (std::uint_least32_t seed);
  virtual ~WiredFtmErrorModel ();

  /**
   * Get the calculated error.
   *
   * \return the error calculated based on this model
   */
  int GetFtmError (void);

  /**
   * Enumeration of the available Channel Bandwidths.
   * Currently only 20 and 40 MHz are implemented.
   */
  enum ChannelBandwidth {
    Channel_20_MHz,
    Channel_40_MHz,
    Channel_80_MHz,
    Channel_160_MHz
  };

  /**
   * Set the seed for the mt19937 generator.
   * \param seed the seed
   */
  void SetSeed (std::uint_least32_t seed);

  /**
   * Returns the currently used seed for the mt19937 generator.
   * \return the seed
   */
  std::uint_least32_t GetSeed (void) const;

  /**
   * Changes the gaussian distribution to the predefined value of the given channel bandwidth.
   *
   * \param bandwidth the bandwidth to be used in the error model
   */
  void SetChannelBandwidth (ChannelBandwidth bandwidth);

  /**
   * Set the mean for the gaussian distribution.
   *
   * \param mean the mean to be used
   */
  void SetMean (double mean);

  /**
   * \return the mean currently used
   */
  double GetMean (void) const;

  /**
   * Set the standard deviation for the gaussian distribution.
   *
   * \param sd the standard deviation to be used
   */
  void SetStandardDeviation (double sd);

  /**
   * \return the standard deviation currently used
   */
  double GetStandardDeviation (void) const;

protected:
  std::mt19937 m_generator; //!< random generator
  std::normal_distribution<double> m_gauss_dist; //!< the distribution
  std::uint_least32_t m_seed;

  double m_mean = 0; //!< mean currently used
  double m_standard_deviation = 0; //!< standard deviation currently used
  const double m_standard_deviation_20MHz = 2562.69; //!< value for 20MHz channel bandwidth
  const double m_standard_deviation_40MHz = 1074.91; //!< value for 40MHz channel bandwidth

  /**
   * Updates the distribution when mean or standard deviation changes.
   */
  void UpdateDistribution (void);
};

/**
 * \brief Error model based on wireless channel.
 * \ingroup FTM
 *
 * This class models a wireless indoor channel where multipath influences the accuracy.
 * In addition to the WiredErrorModel, a map is used to determine
 * bias. The node is used to determine the bias at its current position.
 */
class WirelessFtmErrorModel : public WiredFtmErrorModel
{
public:
  class FtmMap;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  WirelessFtmErrorModel ();
  /**
   * Creates Error model with the specified seed.
   * \param seed the seed
   */
  WirelessFtmErrorModel (std::uint_least32_t seed);
  virtual ~WirelessFtmErrorModel ();

  /*
   * Get the calculated error.
   *
   * \return the calculated error based on this model
   */
  int GetFtmError (void);

  /**
   * Sets the FtmMap to be used for this model.
   *
   * \param map FtmMap to be used
   */
  void SetFtmMap (Ptr<FtmMap> map);

  /**
   * \return the FtmMap used by this model
   */
  Ptr<FtmMap> GetFtmMap (void) const;

  /**
   * Sets the Node which is associated with this error model. Used to determine its position.
   *
   * \param node the node for the model
   */
  void SetNode (Ptr<Node> node);

  /**
   * \return the node used in this model
   */
  Ptr<Node> GetNode (void);

private:
  Ptr<FtmMap> m_map; //!< Pointer to the map.
  Ptr<Node> m_node; //!< Pointer to the node.
};

/**
 * \brief This class loads the FtmMap to be used by the wireless error model.
 * \ingroup FTM
 *
 * Loads the generated by the python script to be used in the wireless error model.
 * Also used to get the bias at a given point.
 * The map generator script can be found in the folder src/wifi/ftm_map/ and is called
 * ftm_map_generator.py. It can be used to create maps with custom size and bias.
 */
class WirelessFtmErrorModel::FtmMap : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  FtmMap ();
  virtual ~FtmMap ();

  /**
   * Loads an existing map file created by the map generator.
   *
   * \param filename the path/name to an existing .map file to be loaded
   */
  void LoadMap (std::string filename);

  /**
   * Returns the bias from the map at a given point.
   *
   * \param x the x coordinate
   * \param y the y coordinate
   * \return the bias at the given point
   */
  double GetBias (double x, double y);

private:
  double *map; //!< the map

  double xmin; //!< x axis minimum
  double xmax; //!< x axis maximum
  double ymin; //!< y axis minimum
  double ymax; //!< y axis maximum
  double resolution; //!< map resolution

  int xsize; //!< x axis size
  int ysize; //!< y axis size
};

} /* namespace ns3 */

#endif /* FTM_ERROR_MODEL_H_ */
