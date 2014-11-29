/*
 * @description: custom strategy for ndn-4-vmm
 *
 * @Created on: Jun 28, 2013
 * @Author: Ruitao Xie
 */

#ifndef MYCUSTOM_STRATEGY_H_
#define MYCUSTOM_STRATEGY_H_

#include "/home/sunshine/workspace/ndnsim/ns-3/scratch/mytype.h"
#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/model/fw/nacks.h"

namespace ns3 {
namespace ndn {
namespace fw {

typedef ForwardingStrategy BaseStrategy;
typedef ns3::ndn::fw::Nacks BaseClass;

//class MyCustomStrategy: public ns3::ndn::ForwardingStrategy

class MyCustomStrategy: public ns3::ndn::fw::Nacks
{
public:
	MyCustomStrategy();
	virtual ~MyCustomStrategy();

public:
	static TypeId
	GetTypeId ();

	static std::string
	GetLogName ();

	bool
	OnControlMessage (Ptr<Face> inFace,
			Ptr<const Interest> header,
			Ptr<const Packet> origPacket,
			Ptr<pit::Entry> pitEntry);

	void
	UpdateCapacityOnRecControl (ns3::Ptr<ns3::Node> innode,
			mytype::LoadTable& thistable,
			std::string prefix,
			ns3::Ptr<ns3::ndn::Face> face);
	void
	UpdateCapacityOnRecInterest
	(ns3::Ptr<ns3::Node> innode,
			mytype::LoadTable& thistable,
			std::string prefix,
			ns3::Ptr<ns3::ndn::Face> face);
	void
	UpdateToken(ns3::Ptr<ns3::Node> innode,
			std::string prefix_str);

	void
	UpdateQueue(ns3::Ptr<ns3::Node> innode,
			std::string prefix_str);

protected:
	virtual bool
	DoPropagateInterest (Ptr<Face> incomingFace,
			Ptr<const Interest> header,
			Ptr<const Packet> origPacket,
			Ptr<pit::Entry> pitEntry);

	void
	Print_FIB_Node (Ptr<Node> node);

public:
	virtual void
	DidSendOutInterest (Ptr<Face> inFace, Ptr<Face> outFace,
			Ptr<const Interest> header,
			Ptr<const Packet> origPacket,
			Ptr<pit::Entry> pitEntry);

	virtual void
	WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry);

	virtual void
	WillSatisfyPendingInterest (Ptr<Face> inFace,
			Ptr<pit::Entry> pitEntry);

	virtual void
	DidExhaustForwardingOptions (Ptr<Face> inFace,
			Ptr<const Interest> header,
			Ptr<const Packet> packet,
			Ptr<pit::Entry> pitEntry);
	virtual void
	DidReceiveValidNack (Ptr<Face> inFace,
			uint32_t nackCode,
			Ptr<const Interest> header,
			Ptr<const Packet> origPacket,
			Ptr<pit::Entry> pitEntry);

protected:
	static LogComponent g_log;

	// private:
	//   std::string m_variable;

	private:
	uint32_t m_counter;


};
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */
#endif /* MYCUSTOM_STRATEGY_H_ */
