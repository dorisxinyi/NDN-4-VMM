/*
 * @description: custom producer for ndn-4-vmm
 *
 * @Created on: Jun 28, 2013
 * @Author: Ruitao Xie
 */

#ifndef MYCUSTOM_PRODUCER_H_
#define MYCUSTOM_PRODUCER_H_

#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/apps/ndn-app.h"

#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-content-object.h"

namespace ns3 {
namespace ndn {

class MyCustomProducer: public ns3::ndn::App {
public:
	static TypeId
	GetTypeId (void);

	MyCustomProducer() {};
	//virtual ~MyCustomProducer();

	// generate a processing delay and handle the interest
	void HandleInterest (const Ptr<const Interest> &interest, Ptr<Packet> packet);

	// inherited from NdnApp
	void OnInterest (const Ptr<const Interest> &interest, Ptr<Packet> packet);

protected:
	// inherited from Application base class.
	virtual void
	StartApplication ();    // Called at time specified by Start

	virtual void
	StopApplication ();     // Called at time specified by Stop

private:
	Name m_prefix;
	uint32_t m_virtualPayloadSize;
	Time m_freshness;
};

} /* namespace ndn */

} /* namespace ns3 */
#endif /* MYCUSTOM_PRODUCER_H_ */
