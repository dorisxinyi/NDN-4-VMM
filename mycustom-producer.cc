/*
 * @description: custom producer for ndn-4-vmm
 *
 * @Created on: Jun 28, 2013
 * @Author: Ruitao Xie
 */

#ifndef MYCUSTOMPRODUCERCC_H_
#define MYCUSTOMPRODUCERCC_H_

#include "mycustom-producer.h"
#include "ns3/log.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-fib.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

#define SRV_TIME_MEAN 0.01//10 //ms
#define SRV_TIME_BOUND 50 //times of mean

extern mytype::PKTQUEUELIST queuelist;
extern mytype::INTERELIST interelist;
extern mytype::IdleIndicatorList idlelist;
extern mytype::SRVTIMELIST srvtimelist;
extern std::ofstream queuefile;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (MyCustomProducer);

NS_LOG_COMPONENT_DEFINE("MyCustomProducer");

TypeId
MyCustomProducer::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::ndn::MyCustomProducer")
				.SetGroupName ("Ndn")
				.SetParent<App> ()
				.AddConstructor<MyCustomProducer> ()
				.AddAttribute ("Prefix","Prefix, for which producer has the data",
						StringValue ("/"),
						MakeNameAccessor (&MyCustomProducer::m_prefix),
						MakeNameChecker ())
						.AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
								UintegerValue (1024),
								MakeUintegerAccessor(&MyCustomProducer::m_virtualPayloadSize),
								MakeUintegerChecker<uint32_t>())
								.AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
										TimeValue (Seconds (0)),
										MakeTimeAccessor (&MyCustomProducer::m_freshness),
										MakeTimeChecker ())
										;

	return tid;
}


/*
MyCustomProducer::MyCustomProducer() {
	// TODO Auto-generated constructor stub

}
 */


// inherited from Application base class.
void
MyCustomProducer::StartApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);

	App::StartApplication ();

	NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());

	Ptr<Fib> fib = GetNode ()->GetObject<Fib> ();

	Ptr<fib::Entry> fibEntry = fib->Add (m_prefix, m_face, 0);

	fibEntry->UpdateStatus (m_face, fib::FaceMetric::NDN_FIB_GREEN);

	// // make face green, so it will be used primarily
	// StaticCast<fib::FibImpl> (fib)->modify (fibEntry,
	//                                        ll::bind (&fib::Entry::UpdateStatus,
	//                                                  ll::_1, m_face, fib::FaceMetric::NDN_FIB_GREEN));
}

void
MyCustomProducer::StopApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);

	App::StopApplication ();
}

void
MyCustomProducer::HandleInterest (const Ptr<const Interest> &interest, Ptr<Packet> origPacket)
{
	App::OnInterest (interest, origPacket); // tracing inside

	NS_LOG_FUNCTION (this << interest);

	if (!m_active) return;

	static ContentObjectTail tail;
	Ptr<ContentObject> header = Create<ContentObject> ();
	header->SetName (Create<Name> (interest->GetName ()));
	header->SetFreshness (m_freshness);

	NS_LOG_INFO ("node("<< GetNode()->GetId() <<") respodning with ContentObject:\n" << boost::cref(*header));

	Ptr<Packet> packet = Create<Packet> (m_virtualPayloadSize);

	packet->AddHeader (*header);
	packet->AddTrailer (tail);

	// Echo back FwHopCountTag if exists
	FwHopCountTag hopCountTag;
	if (origPacket->RemovePacketTag (hopCountTag))
	{
		packet->AddPacketTag (hopCountTag);
	}

	m_protocolHandler (packet);

	m_transmittedContentObjects (header, packet, this, m_face);

	// after processing
	// update the busy time
	Ptr<Node> node = GetNode();
	double now_time = Simulator::Now().ToDouble(Time::S);
	srvtimelist[node].busy_time += now_time - srvtimelist[node].start_time;
	idlelist[node] = true;
	// dequeue
	Ptr<DropTailQueue> & queue = queuelist[node];
	if (queue->IsEmpty()){
		NS_LOG_DEBUG("the packet queue is empty");
	}
	else{
		Ptr<Packet> nextPacket = queue->Dequeue();
		if (nextPacket != 0){
			mytype::INTERETABLE& interetable = interelist[node];
			const Ptr<const Interest> nextinterest = interetable[nextPacket];
			NS_LOG_INFO("dequeue the next interest "<<nextinterest->GetName());
#if false
			if(now_time > 8.00001 && now_time < 10.00001 && node->GetId() == 20){
				std::cout<<"node 20 dequeue when its migration starts, the interest is "<<interest->GetName()<<std::endl;
			}
#endif
			interetable.erase(nextPacket);
			OnInterest(nextinterest, nextPacket);
		}
	}
}

void
MyCustomProducer::OnInterest (const Ptr<const Interest> &interest, Ptr<Packet> origPacket)
{
	Ptr<Node> node = GetNode();
	bool &idlestatus = idlelist[node];
	//if (interest->)
	if (idlestatus){
		// processing this packet
		NS_LOG_INFO("start processing the interest "<<interest->GetName());
		idlestatus = false;
		idlelist[node] = idlestatus;
#if false
		if (idlelist[node])
			NS_LOG_DEBUG("the idle status of node "<< node->GetId() <<" is idle");
		else
			NS_LOG_DEBUG("the idle status of node "<< node->GetId() <<" is busy");
#endif

		// get the average service time from the "srvtimelist"
		double mean = srvtimelist[node].avg_srv_time;
#if true
		double bound = SRV_TIME_BOUND * mean;
		// The expected value for the mean of the values returned by an
		// exponentially distributed random variable is equal to mean.
		Ptr<ExponentialRandomVariable> x = CreateObject<ExponentialRandomVariable> ();
		x->SetAttribute ("Mean", DoubleValue (mean));
		x->SetAttribute ("Bound", DoubleValue (bound));
#endif

#if false
		Ptr<ParetoRandomVariable> x = CreateObject<ParetoRandomVariable> ();
		double shape = 2.0;
		x->SetAttribute ("Mean", DoubleValue (mean));
		x->SetAttribute ("Shape", DoubleValue (shape));
#endif
		double value = x->GetValue ()/1000; // to second
		//double value = mean/1000; // a constant service time to second
		NS_LOG_INFO("the processing delay is "<< value*1000 <<" ms");

		srvtimelist[node].start_time = Simulator::Now().ToDouble(Time::S);
		Simulator::Schedule(Seconds(value), &MyCustomProducer::HandleInterest, this, interest, origPacket);

		// trace
		mytype::QUEUEINFO info;
		info.time = Simulator::Now().ToDouble(Time::S);
		info.node_id = node->GetId();
		Name::const_iterator iterName = interest->GetNamePtr()->begin();
		info.srv_name = *iterName;
		iterName++;
		std::istringstream iss(*iterName);
		iss >> info.seq_no;
		info.event = 1; // represents the start running or dequeue
		info.srv_time = value;
		Ptr<DropTailQueue>& queue = queuelist[node];
		info.queue_len = queue->GetNPackets();
		mytype::print_queueinfo(info, queuefile);
		srvtimelist[node].queue_len = info.queue_len;
	}
	else{
		// enqueue this packet
		NS_LOG_INFO("enqueue the interest "<<interest->GetName());
		Ptr<DropTailQueue>& queue = queuelist[node];
		bool ver = queue->Enqueue(origPacket);
		ns3::UintegerValue maxpkt;
		queue->GetAttributeFailSafe("MaxPackets", maxpkt);
		NS_LOG_INFO("the max queue length is "<<maxpkt.Get());
		NS_LOG_INFO("the current queue length is "<<queue->GetNPackets());
		NS_ASSERT_MSG(ver, "enqueue is failed");
		queuelist[node] = queue;
		mytype::INTERETABLE& interetable = interelist[node];
		interetable[origPacket] = interest;

		// trace
		mytype::QUEUEINFO info;
		info.time = Simulator::Now().ToDouble(Time::S);
		info.node_id = node->GetId();
		Name::const_iterator iterName = interest->GetNamePtr()->begin();
		info.srv_name = *iterName;
		iterName++;
		std::istringstream iss(*iterName);
		iss >> info.seq_no;
		info.event = 0; // represents the enqueue
		info.srv_time = 0;
		info.queue_len = queue->GetNPackets();
		mytype::print_queueinfo(info, queuefile);
		srvtimelist[node].queue_len = info.queue_len;
	}
}

} // namespace ndn
} // namespace ns3
#endif
