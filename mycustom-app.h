/*
 * @description: custom app for ndn-4-vmm
 *
 * @Created on: Jun 28, 2013
 * @Author: Ruitao Xie
 */

// mycustom-app.h

#ifndef MYCUSTOM_APP_H_
#define MYCUSTOM_APP_H_

#include "ns3/ndn-app.h"
#include "/home/sunshine/workspace/ndnsim/ns-3/scratch/mytype.h"

#define APP_START 1
#define APP_STOP 2
#define APP_UPDATE 3

namespace ns3 {

/**
 * @brief A simple custom application
 *
 * This applications demonstrates how to send Interests and respond with ContentObjects to incoming interests
 *
 * When application starts it "sets interest filter" (install FIB entry) for /prefix/sub, as well as
 * sends Interest for this prefix
 *
 * When an Interest is received, it is replied with a ContentObject with 1024-byte fake payload
 */

class MyCustomApp : public ndn::App
{
public:
  // register NS-3 type "MyCustomApp"
  static TypeId
  GetTypeId ();
  
  // (overridden from ndn::App) Processing upon start of the application
  virtual void
  StartApplication ();

  // (overridden from ndn::App) Processing when application is stopped
  virtual void
  StopApplication ();

  // (overridden from ndn::App) Callback that will be called when Interest arrives
  virtual void
  OnInterest (const Ptr<const ndn::Interest> &interest, Ptr<Packet> origPacket);

  // (overridden from ndn::App) Callback that will be called when Data arrives
  virtual void
  OnContentObject (const Ptr<const ndn::ContentObject> &contentObject,
                   Ptr<Packet> payload);

  void
  SetName (std::string _name){
	  m_interestName = _name;
  }

  void
  SetCap (double _cap){
	  m_capacity = _cap;
  }

  void
  UpdateCap(double cap);


protected:
  std::string     m_interestName;        ///< \brief NDN Name of the Interest (use Name)
  double	m_capacity;
  int m_state_type;
private:
  void
  SendInterest ( );
};

} // namespace ns3

#endif // MYCUSTOM_APP_H_
