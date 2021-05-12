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

#ifndef FTM_SESSION_H_
#define FTM_SESSION_H_

#include "ns3/object.h"
#include "ns3/mac48-address.h"
#include "ns3/packet.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/ftm-header.h"
#include "ns3/nstime.h"
#include "ns3/ftm-error-model.h"


namespace ns3 {

/**
 * \brief the FTM session implementation.
 * \ingroup FTM
 *
 * This class represents the FTM session. It is bound to a specific partner and manages all the FTM frames
 * that are exchanged with that partner. Once the session with a partner finishes and a session over callback is
 * specified on the initiator side, it returns itself to the user. This way the RTT can be accessed.
 */
class FtmSession : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  FtmSession ();
  virtual
  ~FtmSession ();

  /**
   * The session types.
   */
  enum SessionType {
    FTM_INITIATOR,
    FTM_RESPONDER,
    FTM_UNINITIALIZED
  };

  /**
   * \brief FTM dialog implementation.
   * \ingroup FTM
   *
   * This represents a complete dialog exchange between initiator and responder.
   * It includes the dialog token and all its time stamps.
   */
  class FtmDialog : public SimpleRefCount<FtmDialog>
  {
  public:
    uint8_t dialog_token;
    uint64_t t1;
    uint64_t t2;
    uint64_t t3;
    uint64_t t4;
  };

  /**
   * Initializes the session with the needed parameters to create a basic FTM session.
   *
   * \param partner_addr the Mac48Address of the partner
   * \param type the SessionType of this session, either initiator or responder
   * \param callback the callback to the FtmManager to send packets
   */
  void InitSession (Mac48Address partner_addr, SessionType type, Callback <void, Ptr<Packet>, WifiMacHeader> callback);

  /**
   * Set the preamble detection duration from the physical layer. This is later used in RTT calculations.
   *
   * \param duration the preamble detection duration
   */
  void SetPreambleDetectionDuration (Time duration);

  /**
   * Set the callback when the session ends. This function should be used by the user to specify
   * the function which is called, when the session ends.
   *
   * \param callback the user function callback
   */
  void SetSessionOverCallback (Callback<void, FtmSession> callback);

  /**
   * Set the session over callback of the manager. Used to remove sessions that have ended.
   *
   * \param callback the manager session over function
   */
  void SetSessionOverCallbackManager (Callback<void, Mac48Address> callback);

  /**
   * Set the block session callback of the manager. Used to not accept new sessions if a previous
   * session request has failed and a timeout has been specified by the responder.
   *
   * \param callback the manager block function
   */
  void SetBlockSessionCallback (Callback<void, Mac48Address, Time> callback);

  /**
   * Set the FTM parameters to be used in this session.
   *
   * \param params the FtmParams
   */
  void SetFtmParams (FtmParams params);

  /**
   * Returns the FTM parameters of the session.
   *
   * \return the FtmParams
   */
  FtmParams GetFtmParams (void);

  /**
   * Processes a received FTM request frame.
   *
   * \param ftm_req the FTM request
   */
  void ProcessFtmRequest (FtmRequestHeader ftm_req);

  /**
   * Processes a received FTM response frame.
   *
   * \param ftm_res the FTM response
   */
  void ProcessFtmResponse (FtmResponseHeader ftm_res);

  /**
   * Starts the FTM session. This should be called by the user after the setup of the session has been completed.
   * Session setup may include setting: FTM parameters, Error model, Session over callback and live RTT feedback.
   */
  void SessionBegin (void);

  /**
   * Sets T1.
   *
   * \param dialog_token the dialog token
   * \param timestamp the time stamp
   */
  void SetT1 (uint8_t dialog_token, uint64_t timestamp);

  /**
   * Sets T2.
   *
   * \param dialog_token the dialog token
   * \param timestamp the time stamp
   */
  void SetT2 (uint8_t dialog_token, uint64_t timestamp);

  /**
   * Sets T3.
   *
   * \param dialog_token the dialog token
   * \param timestamp the time stamp
   */
  void SetT3 (uint8_t dialog_token, uint64_t timestamp);

  /**
   * Sets T4.
   *
   * \param dialog_token the dialog token
   * \param timestamp the time stamp
   */
  void SetT4 (uint8_t dialog_token, uint64_t timestamp);

  /**
   * Returns the map with all saved FTM dialogs. Includes a maximum of the last 255 dialogs.
   *
   * \return the FtmDialog map
   */
  std::map<uint8_t, Ptr<FtmDialog>> GetFtmDialogs (void);

  /**
   * Returns the mean RTT.
   *
   * \return the mean RTT
   */
  int64_t GetMeanRTT (void);

  /**
   * Returns a list with all the RTTs.
   *
   * \return list of RTTs
   */
  std::list<int64_t> GetIndividualRTT (void);

  /**
   * Set the FtmErrorModel for this session.
   *
   * \param error_model the FtmErrorModel
   */
  void SetFtmErrorModel (Ptr<FtmErrorModel> error_model);

  /**
   * Enables live RTT feedback. This means the RTT is given to the specified function, immediately after calculation.
   *
   * \param callback the callback for live RTT
   */
  void EnableLiveRTTFeedback (Callback<void, int64_t> callback);

  /**
   * Disabled the live RTT feedback. Only useful after EnableLiveRTTFeedback has been called.
   */
  void DisableLiveRTTFeedback (void);

  /**
   * Set the callback for over riding the session. This is called when a FTM request comes in with
   * FTM parameters specified after the session has already started.
   *
   * \param callback the callback to the manager over ride function
   */
  void SetOverrideCallback (Callback<void, Mac48Address, FtmRequestHeader> callback);

  /**
   * Set the default parameters for this session. These are used when no parameters are set.
   *
   * \param params the FtmParams
   */
  void SetDefaultFtmParams (FtmParams params);

  /**
   * Set the default parameters for this session. These are used when no parameters are set.
   * This method is used by the attribute system so a user can specify default FTM parameters for all created sessions,
   * without needing to call the method every time. The FtmParamsHolder object is used only here, for this
   * special case.
   *
   * \param params the FtmParamsHolder object which includes the FtmParams to be used
   */
  void SetDefaultFtmParamsHolder (Ptr<FtmParamsHolder> params);

private:
  Mac48Address m_partner_addr; //!< The partner MAC address.
  SessionType m_session_type; //!< The session type.
  FtmParams m_ftm_params;  //!< The FtmParams.
  FtmParams m_default_ftm_params;  //!< The default FtmParams.
  uint64_t m_preamble_detection_duration;  //!< The preamble detection duration.

  Ptr<FtmDialog> m_current_dialog;  //!< The current dialog.
  uint8_t m_current_dialog_token;  //!< The current dialog token.
  uint8_t m_previous_dialog_token;  //!< The previous dialog token.
  uint32_t m_number_of_bursts_remaining; //!< The remaining bursts.
  uint8_t m_ftms_per_burst_remaining; //!< The remaining FTMs for the current burst.
  Time m_current_burst_end; //!< The time when the current burst ends.
  Time m_next_burst_period; //!< The time when the next burst starts.
  Time m_next_ftm_packet; //!< The time when the next FTM packet is send.

  /**
   * If the dialog tokens overflowed, if true the old dialog with the same token, gets deleted.
   */
  bool m_dialog_token_overflow;

  bool m_session_active; //!< If the session is active.

  bool m_live_rtt_enabled; //!< If live RTT is enabled.

  bool m_session_over_callback_set; //!< If a session over callback has been specified.

  Ptr<FtmErrorModel> m_ftm_error_model; //!< The FTM error model.

  std::list<int64_t> m_rtt_list; //!< The RTT list.


  std::map<uint8_t, Ptr<FtmDialog>> m_ftm_dialogs; //!< The FTM dialog map.

  Callback <void, Ptr<Packet>, WifiMacHeader> send_packet; //!< Send packet callback.
  Callback<void, Mac48Address> session_over_ftm_manager_callback; //!< Session over in the FtmManager callback.
  Callback<void, FtmSession> session_over_callback; //!< Session over user callback.
  Callback<void, Mac48Address, Time> block_session; //!< Block session callback.
  Callback<void, int64_t> live_rtt; //!< Live RTT callback.

  /**
   * Creates the default FtmParams
   */
  void CreateDefaultFtmParams (void);

  /**
   * Sets the default FtmParams as the active FtmParams
   */
  void UseDefaultFtmParams (void);

  /**
   * Checks if the FTM parameters are valid.
   *
   * \return true if valid, false otherwise
   */
  bool ValidateFtmParams (void);

  /**
   * Checks if the time stamps are set. Used before sending a new packet, as it only makes sense to send
   * the next packet if the previous packet time stamps are set.
   *
   * \return true if time stamps are set, false otherwise
   */
  bool CheckTimestampSet (void);

  /**
   * Sends the next FTM packet.
   */
  void SendNextFtmPacket (void);

  /**
   * Starts the next burst.
   */
  void StartNextBurst (void);

  /**
   * Sends the trigger frame to the responder.
   */
  void SendTrigger (void);

  /**
   * Finds the dialog with the specified dialog token.
   *
   * \param dialog_token the dialog token
   *
   * \return the FtmDialog if the token has been found, 0 otherwise
   */
  Ptr<FtmDialog> FindDialog (uint8_t dialog_token);

  /**
   * Deletes the FTM dialog that has the specified dialog token.
   *
   * \param dialog_token the dialog token
   */
  void DeleteDialog (uint8_t dialog_token);

  /**
   * Creates a new empty dialog with the specified dialog token.
   *
   * \param dialog_token the dialog token
   *
   * \return the new FtmDialog
   */
  Ptr<FtmDialog> CreateNewDialog (uint8_t dialog_token);

  /**
   * Called when a trigger frame has been received.
   */
  void TriggerReceived (void);

  /**
   * Routine to end the session.
   */
  void EndSession (void);

  /**
   * Denies the incoming session.
   */
  void DenySession (void);

  /**
   * Calculates the RTT for the given dialog.
   *
   * \param dialog the FtmDialog to calculate the RTT of
   */
  void CalculateRTT (Ptr<FtmDialog> dialog);

  Callback<void, Mac48Address, FtmRequestHeader> session_override; //!< The session over ride callback to the manager.

  /**
   * Even number of callbacks break the code following declaration, odd number works. So this code fix callback
   * is in place. The callback only breaks the following declaration on the responder side and writes weird values
   * into the following declaration. Thus breaking the code without an explanation.
   */
  Callback<void> code_fix_callback;
//  int test_value = 0;
};

} /* namespace ns3 */

#endif /* FTM_SESSION_H_ */
