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

#ifndef FTMMANAGER_H
#define FTMMANAGER_H

#include "ns3/object.h"
#include "ns3/wifi-phy.h"
#include "ns3/packet.h"
#include "ns3/ftm-session.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/qos-txop.h"
#include "ns3/ftm-header.h"
#include "ns3/mgt-headers.h"


namespace ns3 {

/**
 * \brief FTM manager class which takes care of all FTM sessions.
 * \ingroup FTM
 *
 * The FTM manager connects to the PHY layer and takes time stamps whenever a packet is sent or received. Processes it
 * and if it is a FTM packet it sets the appropriate time stamps.
 * It forwards the packets to the right sessions and manages them. This means creating and deleting them.
 */
class FtmManager : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  FtmManager ();
  /**
   * This is the main constructor that is used by the RegularWifiMac to set the PHY for the time stamps and
   * the Txop for sending packets.
   *
   * \param phy the WifiPhy
   * \param txop the Txop
   */
  FtmManager (Ptr<WifiPhy> phy, Ptr<Txop> txop);
  virtual
  ~FtmManager ();

  /**
   * Sets the own MAC address.
   *
   * \param addr the managers Mac48Address
   */
  void SetMacAddress (Mac48Address addr);

  /**
   * Creates a new session with the specified partner and type. The basic initialization of the FtmSession is
   * also done here.
   *
   * \param partner the partner Mac48Address
   * \param type the session type
   *
   * \return the newly created FtmSession
   */
  Ptr<FtmSession> CreateNewSession (Mac48Address partner, FtmSession::SessionType type);

  /**
   * Called from the RegularWifiMac when a FTM request has been received. It then gets forwarded to
   * the correct FtmSession.
   *
   * \param partner the partner address
   * \param ftm_req the FTM request
   */
  void ReceivedFtmRequest (Mac48Address partner, FtmRequestHeader ftm_req);

  /**
   * Called from the RegularWifiMac when a FTM response has been received. It then gets forwarded to
   * the correct FtmSession.
   *
   * \param partner the partner address
   * \param ftm_req the FTM response
   */
  void ReceivedFtmResponse (Mac48Address partner, FtmResponseHeader ftm_res);


private:

  /**
   * Structure to store all the header pieces.
   */
  struct PacketInPieces
  {
    WifiMacHeader mac_hdr;
    WifiActionHeader action_hdr;
    FtmRequestHeader ftm_req_hdr;
    FtmResponseHeader ftm_res_hdr;
  };

  /**
   * Finds the session with the specified partner, if it exists.
   *
   * \return the FtmSession if it exists, 0 otherwise
   */
  Ptr<FtmSession> FindSession (Mac48Address addr);

  /**
   * Called from the PHY layer when a frame starts transmitting and a time stamp gets taken.
   * Time stamp then gets added to the correct session, if it is an FTM frame.
   *
   * \param packet the packet
   * \param num a number, needed for the callback, not used
   */
  void PhyTxBegin(Ptr<const Packet> packet, double num);

  /**
   * Called from the PHY layer when a frame gets received and a time stamp gets taken.
   * Time stamp then gets added to the correct session, if it is an FTM frame.
   *
   * \param packet the packet
   * \param rxPowersW the power
   */
  void PhyRxBegin(Ptr<const Packet> packet, RxPowerWattPerChannelBand rxPowersW);

  /**
   * Sends the specified packet with the specified header.
   *
   * \param packet the packet
   * \param hdr the header
   */
  void SendPacket (Ptr<Packet> packet, WifiMacHeader hdr);

  /**
   * Blocks new sessions with the partner for the given duration.
   *
   * \param partner the partner address
   * \param duration the block duration
   */
  void BlockSession (Mac48Address partner, Time duration);

  /**
   * Unblocks new sessions with the partner, after the duration passed.
   *
   * \param partner the partner address
   */
  void UnblockSession (Mac48Address partner);

  /**
   * Checks if new sessions with the partner are blocked.
   *
   * \param partner the partner address
   *
   * \return true if the session is blocked, false otherwise
   */
  bool CheckSessionBlocked (Mac48Address partner);

  /**
   * Deletes the session, cause it is over.
   *
   * \param addr the partner address
   */
  void SessionOver (Mac48Address addr);

  /**
   * Over rides a session by deleting the old session and creating a new one with the FTM request.
   *
   * \param partner the partner address
   * \param ftm_req the FTM request
   */
  void OverrideSession (Mac48Address partner, FtmRequestHeader ftm_req);

  Mac48Address m_mac_address; //!< The mac address.
  std::map<Mac48Address, Ptr<FtmSession>> sessions; //!< The FTM sessions this manager has.
  unsigned int received_packets;  //!< How many packets have been received, after transmitting a FTM frame.
  bool awaiting_ack; //!< Next packet should be ack.

  unsigned int sent_packets; //!< How many packets have been sent, after receiving FTM frame.
  bool sending_ack; //!< Next packet should be ack.
  Mac48Address m_ack_to; //!< Who the ack should go to.

  Ptr<Txop> m_txop; //!< The Txop.

  Time m_preamble_detection_duration; //!< The preamble detection duration.

  PacketInPieces m_current_tx_packet; //!< The currently transmitted packet.
  PacketInPieces m_current_rx_packet; //!< The currently received packet.

  std::list<Mac48Address> m_blocked_partners; //!< List of all the blocked partners.

};

}



#endif /* FTMMANAGER_H_ */
