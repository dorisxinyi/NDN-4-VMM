/*
 * @description: custom app for ndn-4-vmm
 *
 * @Created on: Jun 28, 2013
 * @Author: Ruitao Xie
 */

// mycustom-app.cc
#ifndef MYCUSTOMAPPCC_H_
#define MYCUSTOMAPPCC_H_

#include "mycustom-app.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"

#include "ns3/ndn-fib.h"
#include "ns3/random-variable.h"

#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("MyCustomApp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MyCustomApp);

// register NS-3 type
TypeId
MyCustomApp::GetTypeId ()
{
	static TypeId tid = TypeId ("ns3::MyCustomApp")
    										.SetParent<ndn::App> ()
    										.AddConstructor<MyCustomApp> ()
    										;
	return tid;
}

// Processing upon start of the application
void
MyCustomApp::StartApplication ()
{
	// initialize ndn::App
	ndn::App::StartApplication ();

	//	// Create a name components object for name ``/prefix/sub``
	//	Ptr<ndn::Name> prefix = Create<ndn::Name> (); // now prefix contains ``/``
	//	prefix->Add(m_interestName);
	//
	//	/////////////////////////////////////////////////////////////////////////////
	//	// Creating FIB entry that ensures that we will receive incoming Interests //
	//	/////////////////////////////////////////////////////////////////////////////
	//
	//	// Get FIB object
	//	Ptr<ndn::Fib> fib = GetNode ()->GetObject<ndn::Fib> ();
	//
	//	// Add entry to FIB
	//	// Note that ``m_face`` is cretaed by ndn::App
	//	Ptr<ndn::fib::Entry> fibEntry = fib->Add (*prefix, m_face, m_capacity);

	//Simulator::Schedule (Seconds (1.0), &MyCustomApp::SendInterest, this);
	//int state = APP_START;
	SendInterest();
}

// Processing when application is stopped
void
MyCustomApp::StopApplication ()
{
	m_capacity = 0;
	SendInterest ();
	// cleanup ndn::App
	ndn::App::StopApplication ();
}

void
MyCustomApp::UpdateCap(double cap){
	m_capacity = cap;
	SendInterest ();
}

void
MyCustomApp::SendInterest ( )
{
	Ptr<ndn::Name> prefix = Create<ndn::Name> ();
	prefix->Add("UPDATE");
	prefix->Add(m_interestName);
	std::ostringstream ss;
	ss << m_capacity;
	prefix->Add(ss.str());
	int hop = 0;
	std::ostringstream ss2;
	ss2 << hop;
	prefix->Add(ss2.str());
	uint32_t rdm = mytype::get_unique_id();
	std::ostringstream ss3;
	ss3 << rdm;
	prefix->Add(ss3.str());

	// Create and configure ndn::Interest
	ndn::Interest interestHeader;
	UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
	interestHeader.SetNonce            (rand.GetValue ());
	interestHeader.SetName             (prefix);
	interestHeader.SetInterestLifetime (Seconds (1.0));

	// Create packet and add ndn::Interest
	Ptr<Packet> packet = Create<Packet> ();
	packet->AddHeader (interestHeader);
	LOG_POSITION;
	std::cout<<"Prefix is: ";
	prefix->Print(std::cout);
	std::cout<<"\nCapacity is: "<<m_capacity<<std::endl;
	// Forward packet to lower (network) layer
	m_protocolHandler (packet);

	// Call trace (for logging purposes)
	m_transmittedInterests (&interestHeader, this, m_face);
}

// Callback that will be called when Interest arrives
void
MyCustomApp::OnInterest (const Ptr<const ndn::Interest> &interest, Ptr<Packet> origPacket)
{
	LOG_POSITION;
	//	NS_LOG_DEBUG ("Received Interest packet for " << interest->GetName ());
	//
	//	// Create and configure ndn::ContentObject and ndn::ContentObjectTail
	//	// (header is added in front of the packet, tail is added at the end of the packet)
	//
	//	// Note that Interests send out by the app will not be sent back to the app !
	//
	//	ndn::ContentObject data;
	//	data.SetName (Create<ndn::Name> (interest->GetName ())); // data will have the same name as Interests
	//
	//	ndn::ContentObjectTail trailer; // doesn't require any configuration
	//
	//	// Create packet and add header and trailer
	//	Ptr<Packet> packet = Create<Packet> (1024);
	//	packet->AddHeader (data);
	//	packet->AddTrailer (trailer);
	//
	//	NS_LOG_DEBUG ("Sending ContentObject packet for " << data.GetName ());
	//
	//	// Forward packet to lower (network) layer
	//	m_protocolHandler (packet);
	//
	//	// Call trace (for logging purposes)
	//	m_transmittedContentObjects (&data, packet, this, m_face);
}

// Callback that will be called when Data arrives
void
MyCustomApp::OnContentObject (const Ptr<const ndn::ContentObject> &contentObject,
		Ptr<Packet> payload)
{
	//	NS_LOG_DEBUG ("Receiving ContentObject packet for " << contentObject->GetName ());
	//
	//	std::cout << "DATA received for name " << contentObject->GetName () << std::endl;
}


} // namespace ns3
#endif
