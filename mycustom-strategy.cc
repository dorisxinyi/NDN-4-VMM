/*
 * @description: custom strategy for ndn-4-vmm
 *
 * @Created on: Jun 28, 2013
 * @Author: Ruitao Xie
 */

#include "mycustom-strategy.h"
#include "ns3/log.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-fib-entry.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-interest.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>
#include <math.h>

#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/examples/custom-apps/mycustom-app.h"
#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/examples/custom-apps/mycustom-app.cc"

// data structure
extern mytype::LoadTableList loadtablelist;
extern mytype::TokenPtrTableList tptablelist;
extern std::ofstream facefile;

// simulation parameters
#define LB_ALG 1 // type of load balancing algorithm (random(0); weighted round robin(1); greedy primal-dual(2))
#define CAL_UTIL_METHOD 1 // type of method calculating the utilization (by arrival time(0); by fixed interval(1))
#define TOKEN_START_SPREAD 1 // denote whether to spread the starting token or not
#define LEARN_ROUTE_NACK 0 // denote whether to learn route through NACK

// static constant
#define LB_ALG_RANDOM 0
#define LB_ALG_WRR 1
#define LB_ALG_GPD 2
#define GPD_SRV 1.0 // the coefficient of the reduction of virtual queue length in WRRA
#define GPD_INV_BETA 500 //beta = 1/GPD_INV_BETA: a parameter in WRRA
#define CAL_UTIL_ARRTIME 0
#define CAL_UTIL_FIXINTERVAL 1

using namespace std;

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED(MyCustomStrategy);

//NS_LOG_COMPONENT_DEFINE("MyCustomStrategy");

LogComponent MyCustomStrategy::g_log = LogComponent (MyCustomStrategy::GetLogName ().c_str ());

MyCustomStrategy::MyCustomStrategy()
: m_counter (0)
{
	// TODO Auto-generated constructor stub
}

MyCustomStrategy::~MyCustomStrategy() {
	// TODO Auto-generated destructor stub
}

std::string
MyCustomStrategy::GetLogName ()
{
	return "ndn.fw.MyCustomStrategy";
}

TypeId
MyCustomStrategy::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::ndn::fw::MyCustomStrategy").SetGroupName ("Ndn").SetParent <BaseClass> ().AddConstructor <MyCustomStrategy> ();
	return tid;
}

// handle the control message
bool
MyCustomStrategy::OnControlMessage (Ptr<Face> inFace,
		Ptr<const Interest> header,
		Ptr<const Packet> origPacket,
		Ptr<pit::Entry> pitEntry)
{
	int propagatedCount = 0;
	ns3::Ptr<ns3::Node> innode = inFace->GetNode();
	Ptr<const Name> NamePtr= header->GetNamePtr();
	Name::const_iterator iterName = NamePtr->begin();
	std::string ctr_state_str = "/"+*iterName;
	iterName++;
	std::string ctr_prefix_str = "/"+*iterName;
	iterName++;
	std::string ctr_cap_str = *iterName;
	iterName++;
	std::string ctr_hop_str = *iterName;
	NS_LOG_DEBUG("the control message is "<<ctr_state_str<<"\t"<<ctr_prefix_str<<"\t"<<ctr_cap_str<<"\t"<<ctr_hop_str);
	//LOG_POSITION; std::cout<<ctr_state_str<<"\t"<<ctr_prefix_str<<"\t"<<ctr_cap_str<<"\t"<<ctr_hop_str<<std::endl;

	// update the FIB entry
	Ptr<ndn::Fib> fib = innode->GetObject<ndn::Fib> ();
	Ptr<ndn::Name> app_prefix = Create<ndn::Name> (ctr_prefix_str);
	Ptr<ndn::Name> update_prefix = Create<ndn::Name>(ctr_state_str + ctr_prefix_str);

	double ctr_cap_double;
	std::istringstream is(ctr_cap_str);
	is >> ctr_cap_double;

	int ctr_hop_int;
	std::istringstream iss(ctr_hop_str);
	iss >> ctr_hop_int;

	mytype::LoadTable& thistable = loadtablelist[innode];
	mytype::LoadTableEntryKey thiskey;
	thiskey.second = inFace;

	if (ctr_hop_int == 0){
		NS_LOG_INFO ("at the server node" );
		//LOG_POSITION; std::cout<<"at the server node"<<std::endl;

		Ptr<ndn::fib::Entry> fibEntry_update = fib->Find(*update_prefix);
		thiskey.first = ctr_state_str + ctr_prefix_str;
		if(fibEntry_update==NULL){
			fib->Add (*update_prefix, inFace, 1);
			//LOG_POSITION; Print_FIB_Node(innode);
			// add entry into the LoadTable
			NS_ASSERT_MSG(thistable.find(thiskey) == thistable.end(), "");
			mytype::LoadTableEntry tbentry;
			tbentry.strname = ctr_state_str + ctr_prefix_str;
			tbentry.nsname = update_prefix;
			tbentry.capacity = ctr_cap_double;
			tbentry.outgoing_interface = inFace;
			thistable[thiskey] = tbentry;
			NS_LOG_DEBUG("new entry <" << tbentry.strname.c_str() <<"," << tbentry.outgoing_interface->GetId() <<"> is added into the LoadTable");
			//LOG_POSITION; printf("new entry <%s, %d> is added into the LoadTable\n", tbentry.strname.c_str(), tbentry.outgoing_interface->GetId());
			UpdateCapacityOnRecControl(innode, thistable, ctr_prefix_str, inFace);
		}
		else{
			NS_ASSERT_MSG(fibEntry_update!=NULL,"");
			mytype::FACELIST& facelist = fibEntry_update->m_faces.get<1>();
			mytype::FACELIST::iterator iterFace = facelist.begin();
			NS_ASSERT_MSG(iterFace != facelist.end(),"");
			if (iterFace->GetFace()->GetId() == inFace->GetId()){
				NS_LOG_DEBUG("face exists, face id: " << iterFace->GetFace()->GetId());
				//LOG_POSITION; std::cout<<"face exists, face id: "<<iterFace->GetFace()->GetId()<<std::endl;
				// update the entry in the LoadTable
				NS_ASSERT_MSG(thistable.find(thiskey) != thistable.end(), "");
				mytype::LoadTable::iterator thistbentry = thistable.find(thiskey);
				NS_LOG_DEBUG("before update: "<<thistbentry->second.capacity);
				//LOG_POSITION; std::cout<<"before update: "<<thistbentry->second.capacity<<std::endl;
				thistbentry->second.capacity = ctr_cap_double;
				NS_LOG_DEBUG("after update: "<<thistbentry->second.capacity);
				//LOG_POSITION; std::cout<<"after update: "<<thistbentry->second.capacity<<std::endl;
				UpdateCapacityOnRecControl(innode, thistable, ctr_prefix_str, inFace);
			}
		}
	}
	else if (ctr_hop_int > 0){
		NS_LOG_INFO ("at the switch node" );
		//LOG_POSITION; std::cout<<"at the switch"<<std::endl;
		thiskey.first = ctr_prefix_str;
		if (thistable.find(thiskey) == thistable.end()){
			NS_LOG_DEBUG("no fib entry for this prefix");
			//LOG_POSITION; std::cout<<"no fib entry for this prefix"<<std::endl;
			fib->Add (*app_prefix, inFace, 1);
			//LOG_POSITION; Print_FIB_Node(innode);
			// add entry into the LoadTable
			NS_ASSERT_MSG(thistable.find(thiskey) == thistable.end(), "");
			mytype::LoadTableEntry tbentry;
			tbentry.strname = ctr_prefix_str;
			tbentry.nsname = app_prefix;
			tbentry.capacity = ctr_cap_double;
			tbentry.outgoing_interface = inFace;
			thistable[thiskey] = tbentry;
			NS_LOG_DEBUG("new entry <" <<tbentry.strname.c_str()<<","<<tbentry.outgoing_interface->GetId()<<"> is added into the LoadTable");
			//LOG_POSITION; printf("new entry <%s, %d> is added into the LoadTable\n", tbentry.strname.c_str(), tbentry.outgoing_interface->GetId());
		}
		else{
			mytype::LoadTable::iterator thistbentry = thistable.find(thiskey);
			NS_LOG_DEBUG("before update: "<<thistbentry->second.capacity);
			//LOG_POSITION; std::cout<<"before update: "<<thistbentry->second.capacity<<std::endl;
			thistbentry->second.capacity = ctr_cap_double;
			NS_LOG_DEBUG("after update: "<<thistbentry->second.capacity);
			//LOG_POSITION; std::cout<<"after update: "<<thistbentry->second.capacity<<std::endl;
			//LOG_POSITION; LOG_POSITION; printf("entry <%s, %d> is updated in the LoadTable\n", thistbentry->second.strname.c_str(), thistbentry->second.outgoing_interface->GetId());
		}
		// new the token in loadbalancetable and tokenptr in tokenptrtable
		if(LB_ALG == LB_ALG_WRR){
			UpdateToken(innode, ctr_prefix_str);
		}
		if(LB_ALG == LB_ALG_GPD){
			// initialize the vir_queue in loadbalancetable
			UpdateQueue(innode, ctr_prefix_str);
			UpdateQueue(innode, "/");
		}
	}
	//LOG_POSITION; Print_FIB_Node(innode);
	double sum_cap = 0;
	if (ctr_hop_int == 0)
		sum_cap = ctr_cap_double;
	else if (ctr_hop_int > 0){
		Ptr<ndn::fib::Entry> fibEntry = fib->Find(*app_prefix);
		mytype::FACELIST& facelist_sum = fibEntry->m_faces.get<1>();
		mytype::FACELIST::iterator iterFace_sum = facelist_sum.begin();
		for(; iterFace_sum!=facelist_sum.end(); iterFace_sum++)
		{
			thiskey.first = ctr_prefix_str;
			thiskey.second = iterFace_sum->GetFace();
			NS_ASSERT_MSG(thistable.find(thiskey)!=thistable.end(), "");
			mytype::LoadTable::iterator thistbentry = thistable.find(thiskey);
			sum_cap = sum_cap + thistbentry->second.capacity;
		}
		NS_LOG_DEBUG("sum of cap is "<<sum_cap);
		//LOG_POSITION; std::cout<<"sum of cap is "<<sum_cap<<std::endl;
	}
	// new packet and forward it to all up interfaces
	if(ctr_hop_int < 3){
		Ptr<ndn::Name> new_prefix = Create<ndn::Name> (ctr_state_str + ctr_prefix_str);
		std::ostringstream os;
		os << sum_cap;
		new_prefix->Add(os.str());
		ctr_hop_int ++;
		std::ostringstream os2;
		os2 << ctr_hop_int;
		new_prefix->Add(os2.str());
		uint32_t rdm = mytype::get_unique_id();
		std::ostringstream ss3;
		ss3 << rdm;
		new_prefix->Add(ss3.str());
		NS_LOG_DEBUG("Sending Update Control Message for " << *new_prefix);
		//LOG_POSITION; std::cout<<"Sending Update Control Message for " << *new_prefix<<std::endl;
		ndn::Interest new_interestHeader;
		UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
		new_interestHeader.SetNonce            (rand.GetValue ());
		new_interestHeader.SetName             (new_prefix);
		new_interestHeader.SetInterestLifetime (Seconds (1.0));
		Ptr<Packet> new_packet = Create<Packet> ();
		new_packet->AddHeader (new_interestHeader);
		NS_LOG_DEBUG("Sending Update Control Message for " <<new_interestHeader);
		//		LOG_POSITION; std::cout<<"Sending Update Control Message for ";
		//		new_interestHeader.Print(std::cout);
		//		cout<<std::endl;

		Ptr<ndn::Name> root_prefix = Create<ndn::Name> ("/");
		Ptr<ndn::fib::Entry> fibEntry_root = fib->Find(*root_prefix);
		mytype::FACELIST& faces_root = fibEntry_root->m_faces.get<1>();
		mytype::FACELIST::iterator iter_root = faces_root.begin();
		for (; iter_root != faces_root.end(); iter_root++){
			if (TrySendOutInterest(inFace, iter_root->GetFace(), &new_interestHeader, new_packet, pitEntry)){
				propagatedCount ++;
				NS_LOG_DEBUG("up forwarding to face id: "<<iter_root->GetFace()->GetId());
				//LOG_POSITION; std::cout<<"up forwarding to face id: "<<iter_root->GetFace()->GetId()<<std::endl;
			}
		}
	}
	else // core node does not propagate the control message
		propagatedCount = 1;
	return propagatedCount > 0;
}

// an example of prefix is "/service-1"
void
MyCustomStrategy::UpdateCapacityOnRecControl(ns3::Ptr<ns3::Node> innode, mytype::LoadTable& thistable, std::string prefix, ns3::Ptr<ns3::ndn::Face> face)
{
	// update the capacity in LoadTable: /UPDATE/service-1.capacity ----> /service-1.capacity
	mytype::LoadTableEntryKey key_src;
	key_src.first = "/UPDATE" + prefix;
	key_src.second = face;

	mytype::LoadTableEntryKey key_dst;
	key_dst.first = prefix;
	Ptr<ndn::Name> prefix_dst = Create<ndn::Name> (prefix);
	Ptr<ndn::Fib> fib = innode->GetObject<ndn::Fib> ();
	Ptr<ndn::fib::Entry> fibEntry_dst = fib->Find(*prefix_dst);
	if (fibEntry_dst != NULL){
		mytype::FACELIST& faces_dst = fibEntry_dst->m_faces.get<1>();
		key_dst.second = faces_dst.begin()->GetFace();
		mytype::LoadTable::iterator iter_src = thistable.find(key_src);
		mytype::LoadTable::iterator iter_dst = thistable.find(key_dst);
		if (iter_dst != thistable.end()){
			NS_LOG_DEBUG("before update: "<<iter_dst->second.capacity);
			//LOG_POSITION; std::cout<<"before update: "<<iter_dst->second.capacity<<std::endl;
			iter_dst->second.capacity = iter_src->second.capacity;
			NS_LOG_DEBUG("after update: "<<iter_dst->second.capacity);
			//LOG_POSITION; std::cout<<"after update: "<<iter_dst->second.capacity<<std::endl;
			NS_LOG_DEBUG("the entry <"<<prefix.c_str()<<", "<<key_dst.second->GetId()<<"> in Load Table is updated");
			//LOG_POSITION; printf("the entry <%s, %d> in Load Table is updated\n", prefix.c_str(), key_dst.second->GetId());
			UpdateToken(innode, prefix);
			UpdateQueue(innode, prefix);
			UpdateQueue(innode, "/");
		}
		else{
			NS_LOG_DEBUG("the entry <"<<prefix.c_str()<<", "<<key_dst.second->GetId()<<"> does not exist in Load Table");
			//LOG_POSITION; printf("the entry <%s, %d> does not exist in Load Table\n", prefix.c_str(), key_dst.second->GetId());
		}
	}
	else
		NS_LOG_DEBUG("the entry <"<<prefix.c_str()<<", *> does not exist in Load Table");
	//LOG_POSITION; printf("the entry <%s, *> does not exist in Load Table\n", prefix.c_str());

}
void
MyCustomStrategy::UpdateCapacityOnRecInterest(ns3::Ptr<ns3::Node> innode, mytype::LoadTable& thistable, std::string prefix, ns3::Ptr<ns3::ndn::Face> face)
{
	// update the capacity in LoadTable: /UPDATE/service-1.capacity ----> /service-1.capacity
	mytype::LoadTableEntryKey key_src;
	key_src.first = "/UPDATE" + prefix;
	mytype::LoadTableEntryKey key_dst;
	key_dst.first = prefix;
	key_dst.second = face;

	Ptr<ndn::Name> prefix_src = Create<ndn::Name> ("/UPDATE" + prefix);
	Ptr<ndn::Fib> fib = innode->GetObject<ndn::Fib> ();
	Ptr<ndn::fib::Entry> fibEntry_src = fib->Find(*prefix_src);
	if (fibEntry_src != NULL){
		mytype::FACELIST& faces_src = fibEntry_src->m_faces.get<1>();
		key_src.second = faces_src.begin()->GetFace();
		mytype::LoadTable::iterator iter_src = thistable.find(key_src);
		mytype::LoadTable::iterator iter_dst = thistable.find(key_dst);
		if (iter_dst != thistable.end()){
			iter_dst->second.capacity = iter_src->second.capacity;
			//LOG_POSITION; std::cout<<"before update: "<<iter_dst->second.capacity<<std::endl;
			//LOG_POSITION; std::cout<<"after update: "<<iter_dst->second.capacity<<std::endl;
			NS_LOG_DEBUG("the entry <"<<prefix.c_str()<<", "<<face->GetId()<<"> in Load Table is updated");
			//LOG_POSITION; printf("the entry <%s, %d> in Load Table is updated\n", prefix.c_str(), face->GetId());
			UpdateToken(innode, prefix);
			UpdateQueue(innode, prefix);
			UpdateQueue(innode, "/");
		}
		else{
			NS_LOG_DEBUG("the entry <"<<prefix.c_str()<<", "<<face->GetId()<<"> does not exist in Load Table");
			//LOG_POSITION; printf("the entry <%s, %d> does not exist in Load Table\n", prefix.c_str(), face->GetId());
		}
	}
	else
		NS_LOG_DEBUG("the entry <"<<prefix.c_str()<<", "<<face->GetId()<<"> does not exist in Load Table");
	//LOG_POSITION; printf("the entry <%s, %d> does not exist in Load Table\n", prefix.c_str(), face->GetId());

}

void
MyCustomStrategy::Print_FIB_Node (Ptr<Node> node)
{
	Ptr<ndn::Fib> fib = node->GetObject<ndn::Fib> ();
	Ptr<ndn::fib::Entry > entry = fib->Begin ();
	for (unsigned int i = 0; i < fib->GetSize(); i++ )
	{
		std::cout << "FIB\t"
				<< node->GetId () << "\t"
				<< Names::FindName (node) << "\t"
				<< *entry->m_prefix << "\t"
				<< *entry << "\n";
		entry = fib->Next (entry);
	}
}

bool
MyCustomStrategy::DoPropagateInterest (Ptr<Face> inFace,
		Ptr<const Interest> header,
		Ptr<const Packet> origPacket,
		Ptr<pit::Entry> pitEntry)
{
	ns3::Ptr<ns3::Node> innode = inFace->GetNode();
	NS_LOG_DEBUG("node id is: "<<innode->GetId());
	//	LOG_POSITION; std::cout<<"node id is: "<<innode->GetId()<<std::endl;

	// handle the control message
	Ptr<const Name> NamePtr= header->GetNamePtr();
	int num_com_prefix = NamePtr->size();
	if (num_com_prefix == 5){
		bool isPropagated = OnControlMessage(inFace, header, origPacket, pitEntry);
		return isPropagated;
	}

	int propagatedCount = 0;
	typedef fib::FaceMetricContainer::type::index<fib::i_metric>::type FacesByMetric;
	FacesByMetric &faces = pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ();
	FacesByMetric::iterator faceIterator = faces.begin ();
	static int ncalls = 0;
	ncalls++;
	double util[faces.size()];
	double cap[faces.size()];
	double token[faces.size()];
	double gpd_metric[faces.size()];
	double sum_queue = 0;
	uint32_t loc_val_loadtab[faces.size()];
	uint32_t num_cand_faces = 0;
	int i=0;
	ns3::Ptr<ns3::ndn::Face> outface;

	mytype::LoadTable& thistable = loadtablelist[innode];
	mytype::LoadTableEntryKey thiskey;
	std::ostringstream os;
	pitEntry->GetFibEntry()->m_prefix->Print(os);
	std::string prefix = os.str();
	thiskey.first = prefix;

	// cout the interest information
	NS_LOG_DEBUG("pit entry on node "<<innode->GetId()<<"\n"<<*pitEntry);
	// cout information in selected faces
	NS_LOG_DEBUG("selected faces number = "<<faces.size());
	outface = faceIterator->GetFace();
	thiskey.second = outface;

	if(thistable.find(thiskey)==thistable.end()){ // at the server node
		mytype::update_table_from_node(innode, thistable, true);
		UpdateCapacityOnRecInterest(innode, thistable, prefix, outface);
	}

	// find a face
	// read the information of the candidate interfaces (whose capacity is positive)
	int loc_iter = 0;
	for (faceIterator = faces.begin(); faceIterator != faces.end (); faceIterator++){ // get utilization and capacity value of all outgoing faces
		outface = faceIterator->GetFace();
		thiskey.second = outface;
		mytype::LoadTableEntry ent;
		ent = thistable[thiskey];
		if(ent.capacity > 0){
			util[i] = ent.util;
			cap[i] = ent.capacity;
			token[i] = ent.token;
			gpd_metric[i] = ent.vir_queue / ent.capacity;
			sum_queue = sum_queue + ent.vir_queue;
			loc_val_loadtab[i] = loc_iter; // this vector records the mapping from the item in the nonzero capacity list to the item in the face list
			i++;
		}
		loc_iter++;
	}
	num_cand_faces = i;
	uint32_t prob_itv[num_cand_faces];
	std::partial_sum (cap, cap+num_cand_faces, prob_itv);

	// the interface is unreachable if the capacity is 0
	// face_no is the index of selected interface in the candidate interfaces (whose capacity is positive)
	if (num_cand_faces > 0){
		uint32_t face_no;
		if (LB_ALG == LB_ALG_GPD){
			NS_LOG_DEBUG("GPD alg: selected faces number is "<<faces.size());
			for(uint32_t tmp = 0; tmp< faces.size(); tmp++){
				NS_LOG_DEBUG("GPD alg: gpd metric is "<<gpd_metric[tmp]);}
			NS_ASSERT_MSG(num_cand_faces > 0, "GPD alg: no candidate interfaces");
			face_no = std::min_element(gpd_metric, gpd_metric+num_cand_faces) - gpd_metric;
			NS_LOG_DEBUG("GPD alg: selected face no is "<<face_no);
		}
		else if (LB_ALG == LB_ALG_RANDOM){	// find the face randomly according to the probability of cap
			LOG_POSITION; for(uint32_t i=0; i<num_cand_faces;i++){std::cout<<cap[i]<<"\t"<<prob_itv[i]<<std::endl;}
			UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
			uint32_t rdm = rand.GetInteger(0,std::numeric_limits<uint32_t>::max ()) % prob_itv[num_cand_faces-1];
			//uint32_t rdm = rand() % prob_itv[num_cand_faces-1];
			LOG_POSITION; std::cout<<rdm<<std::endl;
			for(face_no=0; face_no<num_cand_faces && rdm>=prob_itv[face_no]; face_no++);
		}
		else if (LB_ALG == LB_ALG_WRR){ // find the face by weighted round robin
			mytype::TokenPtrTable& thistptable = tptablelist[innode];
			mytype::TokenPtrTableEntry& tpent = thistptable[prefix];
			NS_ASSERT_MSG(thistptable.find(prefix) != thistptable.end(), "");
			bool allzero = true;
			size_t j = 0;
			for(; j < num_cand_faces; j++){
				if (token[j] > 0){
					allzero = false;
					break;
				}
			}
			if (allzero == true){ // initialize
				NS_LOG_DEBUG("all token are zero, start initialization of token");
				//LOG_POSITION; std::cout<<"all token are zero, start initialization of token"<<std::endl;

				double min_cap = *std::min_element(cap, cap+num_cand_faces);
				NS_ASSERT_MSG(min_cap > 0, ""); NS_ASSERT_MSG(!isinf(min_cap), "");
				//LOG_POSITION; std::cout<<"min_cap = "<<min_cap<<std::endl;
				if (min_cap != 0){
					for (j = 0; j < num_cand_faces; j++){
						token[j] = cap[j] / min_cap;
						NS_ASSERT_MSG(!isinf(token[j]), "");
						NS_ASSERT_MSG((token[j] >= 0), "");
					}
					// update the token in loadbalance table
					int j = 0;
					for (faceIterator = faces.begin(); faceIterator != faces.end (); faceIterator++){ // get utilization and capacity value of all outgoing faces
						outface = faceIterator->GetFace();
						thiskey.second = outface;
						mytype::LoadTableEntry& ent = thistable[thiskey];
						if(ent.capacity > 0){
							ent.token = token[j];
							j++;
						}
					}
				}

				if (TOKEN_START_SPREAD == 1){
					int xth_nonzero = innode->GetId() % num_cand_faces;
					/*
					// find the mapping location of the location in the candidate interfaces (whose capacity is positive)
					int xth_iter = 0;
					for (j = 0; j < faces.size(); j++){
						if (token[j] != 0){
							if (xth_iter == xth_nonzero){
								tpent.token_ptr = j;
								break;
							}
							xth_iter ++;
						}
					}
					 */
					tpent.token_ptr = xth_nonzero;
					//NS_ASSERT_MSG(0, "");
				}
				else
					tpent.token_ptr = 0;
			}
			face_no = tpent.token_ptr; // get face number from tokenptr table
			NS_LOG_DEBUG("the starting face no is "<<face_no);
			for(; token[face_no]==0; face_no = (face_no + 1) % num_cand_faces);
			NS_LOG_DEBUG("the selected face no is "<<face_no);
			NS_ASSERT_MSG(token[face_no] > 0, "");
		}

		while (true){
			faceIterator = faces.begin();
			NS_ASSERT_MSG(face_no >= 0 && face_no < num_cand_faces, "");
			for(uint32_t i = 0; i < loc_val_loadtab[face_no]; i++){	//find the node of the outgoing face
				faceIterator ++;
			}
			// send
			NS_ASSERT_MSG(faceIterator != faces.end (), "NULL face\n");
			outface = faceIterator->GetFace();
			if (TrySendOutInterest(inFace, outface, header, origPacket, pitEntry)){
				propagatedCount ++;
				NS_LOG_DEBUG("forwarding <face id = " << outface->GetId()<<", utilization = "<<util[face_no]<<">");
				// update the loadtable entry
				thiskey.second = outface;
				mytype::LoadTableEntry& ent = thistable[thiskey];
				double now_time = Simulator::Now ().ToDouble (Time::S);
				double alpha = 0.2;
				ent.avg_inter_arrival_time = (1-alpha) * ent.avg_inter_arrival_time + alpha * (now_time - ent.last_used_time);
				if (CAL_UTIL_METHOD == CAL_UTIL_ARRTIME)
					ent.load = 1 / (now_time - ent.last_used_time);
				ent.last_used_time = now_time;
				ent.pkt_num ++;
				if (CAL_UTIL_METHOD == CAL_UTIL_ARRTIME)
					ent.util = ent.load / ent.capacity;
				if (CAL_UTIL_METHOD == CAL_UTIL_FIXINTERVAL){
					ent.pkt_num_tunit ++; // it records the number of packets per time step
				}
				if (LB_ALG == LB_ALG_WRR){ // update the token in the loadbalance table and in the tokenptr table
					ent.token --;
					NS_ASSERT_MSG(ent.token >= 0, "");
					mytype::TokenPtrTable& thistptable = tptablelist[innode];
					mytype::TokenPtrTableEntry& tpent = thistptable[prefix];
					tpent.token_ptr = (face_no + 1)%num_cand_faces;
				}
				if (LB_ALG == LB_ALG_GPD){ // update the virtual queue length in the loadbalance table
					ent.vir_queue = ent.vir_queue + 1/ent.capacity;
					sum_queue = sum_queue + 1/ent.capacity;
					if (sum_queue >= GPD_INV_BETA){ // update the virtual queue length of all candidate interfaces
						for (faceIterator = faces.begin(); faceIterator != faces.end (); faceIterator++){ // get utilization and capacity value of all outgoing faces
							outface = faceIterator->GetFace();
							thiskey.second = outface;
							mytype::LoadTableEntry& ent = thistable[thiskey];
							if(ent.capacity > 0){
								double min_cap = *std::min_element(cap, cap+num_cand_faces);
								double tmp = ent.vir_queue - GPD_SRV / min_cap;
								ent.vir_queue = tmp > 0 ? tmp : 0;
								NS_LOG_DEBUG("vir_queue="<<ent.vir_queue);
							}
						}
					}
				}

				// print in the facefile
				Ptr<const Name> NamePtr= header->GetNamePtr();
				Name::const_iterator iterName = NamePtr->begin();
				iterName++;
				facefile<<now_time<<"\t"<<innode->GetId()<<"\t"<<outface->GetId()<<"\t"<<*iterName<<std::endl;
				break;
			}
			else{
				LOG_POSITION; // to test the deadloop
				if (LB_ALG == LB_ALG_GPD){
					uint32_t face_no_bk = face_no;
					face_no = (face_no + 1) % num_cand_faces;
					if (face_no == face_no_bk){ // one cycle
						break;
					}
				}
				if (LB_ALG == LB_ALG_WRR){
					face_no = (face_no + 1) % num_cand_faces;
					for(; token[face_no]==0; face_no = (face_no + 1) % num_cand_faces);
					NS_LOG_DEBUG("the selected face no is "<<face_no);
					NS_ASSERT_MSG(token[face_no] > 0, "");
					mytype::TokenPtrTable& thistptable = tptablelist[innode];
					mytype::TokenPtrTableEntry& tpent = thistptable[prefix];
					if (face_no == tpent.token_ptr) // one cycle
						break;
				}
			}
		}
	}
	else{
		NS_LOG_DEBUG("no interface can be used");
	}
	return propagatedCount > 0;
}

void
MyCustomStrategy::UpdateToken(ns3::Ptr<ns3::Node> innode,
		std::string prefix_str)
{
	Ptr<ndn::Name> prefix = Create<ndn::Name> (prefix_str);
	Ptr<ndn::Fib> fib = innode->GetObject<ndn::Fib> ();
	Ptr<ndn::fib::Entry> fibEntry = fib->Find(*prefix);
	NS_ASSERT_MSG(fibEntry,"NULL pointer");
	mytype::FACELIST& facelist = fibEntry->m_faces.get<1>();
	NS_LOG_DEBUG("/ has "<<facelist.size()<<" faces");
	//LOG_POSITION; std::cout<<"/ has "<<facelist.size()<<" faces"<<std::endl;
	mytype::FACELIST::iterator iterFace = facelist.begin();

	mytype::LoadTable& thistable = loadtablelist[innode];
	mytype::LoadTableEntryKey thiskey;

	double cap[facelist.size()];
	int i = 0;
	for(; iterFace!=facelist.end(); iterFace++)
	{
		thiskey.first = prefix_str;
		thiskey.second = iterFace->GetFace();
		NS_ASSERT_MSG(thistable.find(thiskey)!=thistable.end(), "");
		mytype::LoadTable::iterator thistbentry = thistable.find(thiskey);
		cap[i] = thistbentry->second.capacity;
		i++;
	}

	double max_cap = *std::max_element(cap, cap+facelist.size());
	if (max_cap!=0){
		NS_LOG_DEBUG("at least one interface has nonzero capacity");
		// first remove the zero items
		double cap_nonzero[facelist.size()];
		int num_cap_nonzero = 0;
		for (size_t i = 0; i < facelist.size(); i++){
			if (cap[i] != 0){
				cap_nonzero[num_cap_nonzero] = cap[i];
				num_cap_nonzero++;
			}
		}
		double min_cap_nonzero = *std::min_element(cap_nonzero, cap_nonzero+num_cap_nonzero);
		NS_ASSERT_MSG(min_cap_nonzero != 0, "");
		if (min_cap_nonzero != 0){
			iterFace = facelist.begin();
			for (size_t i=0; i<facelist.size(); i++){
				thiskey.first = prefix_str;
				thiskey.second = iterFace->GetFace();
				mytype::LoadTable::iterator thistbentry = thistable.find(thiskey);
				thistbentry->second.token = cap[i] / min_cap_nonzero;
				iterFace++;
				NS_ASSERT_MSG(!isinf(thistbentry->second.token), "");
			}
		}
	}
	else{
		NS_LOG_DEBUG("all capacity is zero");
		//		LOG_POSITION; std::cout<<"all capacity is zero"<<std::endl;
		for(iterFace = facelist.begin(); iterFace!=facelist.end(); iterFace++)
		{
			thiskey.first = prefix_str;
			thiskey.second = iterFace->GetFace();
			mytype::LoadTable::iterator thistbentry = thistable.find(thiskey);
			thistbentry->second.token = 0;
		}
	}
	mytype::TokenPtrTable& thistptable = tptablelist[innode];

	if (thistptable.find(prefix_str) == thistptable.end()){
		mytype::TokenPtrTableEntry tpent;
		tpent.name = prefix_str;
		tpent.token_ptr = 0;
		thistptable[prefix_str] = tpent;
	}
	else{
		mytype::TokenPtrTableEntry& tpent = thistptable[prefix_str];
		tpent.token_ptr = 0;
	}
}

void
MyCustomStrategy::UpdateQueue(ns3::Ptr<ns3::Node> innode,
		std::string prefix_str)
{
	NS_LOG_DEBUG("Update the virtual queue");

	Ptr<ndn::Name> prefix = Create<ndn::Name> (prefix_str);
	Ptr<ndn::Fib> fib = innode->GetObject<ndn::Fib> ();
	Ptr<ndn::fib::Entry> fibEntry = fib->Find(*prefix);
	NS_ASSERT_MSG(fibEntry,"NULL pointer");
	mytype::FACELIST& facelist = fibEntry->m_faces.get<1>();
	NS_LOG_DEBUG("/ has "<<facelist.size()<<" faces");
	mytype::FACELIST::iterator iterFace = facelist.begin();

	mytype::LoadTable& thistable = loadtablelist[innode];
	mytype::LoadTableEntryKey thiskey;

	uint32_t num_cand_faces = 1;
	double sum_capacity = 0;
	for(; iterFace!=facelist.end(); iterFace++)
	{
		thiskey.first = prefix_str;
		thiskey.second = iterFace->GetFace();
		NS_ASSERT_MSG(thistable.find(thiskey)!=thistable.end(), "");
		mytype::LoadTable::iterator thistbentry = thistable.find(thiskey);
		if (thistbentry->second.capacity > 0){
			num_cand_faces ++;
			sum_capacity = sum_capacity + thistbentry->second.capacity;
		}
	}

	if (num_cand_faces > 1){
		num_cand_faces --;
		for(iterFace = facelist.begin(); iterFace!=facelist.end(); iterFace++)
		{
			thiskey.first = prefix_str;
			thiskey.second = iterFace->GetFace();
			NS_ASSERT_MSG(thistable.find(thiskey)!=thistable.end(), "");
			mytype::LoadTableEntry& ent = thistable[thiskey];
			if(ent.capacity > 0){
				ent.vir_queue = GPD_INV_BETA;
				ent.vir_queue = ent.vir_queue * ent.capacity / sum_capacity;
				NS_LOG_DEBUG("num_cand_faces="<<num_cand_faces<<"\nvir_queue="<<ent.vir_queue);
			}
		}
	}

}


void
MyCustomStrategy::DidSendOutInterest (Ptr<Face> inFace, Ptr<Face> outFace,
		Ptr<const Interest> header,
		Ptr<const Packet> origPacket,
		Ptr<pit::Entry> pitEntry)
{
	m_counter ++;
}

void
MyCustomStrategy::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
	for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
			face != pitEntry->GetOutgoing ().end ();
			face ++)
	{
		m_counter --;
	}

	BaseStrategy::WillEraseTimedOutPendingInterest (pitEntry);
}


void
MyCustomStrategy::WillSatisfyPendingInterest (Ptr<Face> inFace,
		Ptr<pit::Entry> pitEntry)
{
	for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
			face != pitEntry->GetOutgoing ().end ();
			face ++)
	{
		m_counter --;
	}

	BaseStrategy::WillSatisfyPendingInterest (inFace, pitEntry);
}

void
MyCustomStrategy::DidExhaustForwardingOptions (Ptr<Face> inFace,
		Ptr<const Interest> header,
		Ptr<const Packet> packet,
		Ptr<pit::Entry> pitEntry)
{
	ns3::Ptr<ns3::Node> innode = inFace->GetNode();
	NS_LOG_DEBUG("send NACK at node "<<innode->GetId());
	double now_time = Simulator::Now ().ToDouble (Time::S);
	//header->Print(std::cout);
	Ptr<const Name> NamePtr= header->GetNamePtr();
	Name::const_iterator iterName = NamePtr->begin();
	iterName++;
	ndn::pit::Entry::in_container incomingfaces = pitEntry->GetIncoming();
	ndn::pit::IncomingFace outface = *incomingfaces.begin();
	facefile<<now_time<<"\t"<<innode->GetId()<<"\t"<<outface.m_face->GetId()<<"\t"<<*iterName<<"\t NACK"<<std::endl;
	//NS_ASSERT_MSG(0, "");
	BaseClass::DidExhaustForwardingOptions(inFace, header, packet, pitEntry);
}

void
MyCustomStrategy::DidReceiveValidNack (Ptr<Face> inFace,
		uint32_t nackCode,
		Ptr<const Interest> header,
		Ptr<const Packet> origPacket,
		Ptr<pit::Entry> pitEntry)
{
	// remove the incoming face from the face list
	if (LEARN_ROUTE_NACK == 1){
		if (nackCode == Interest::NACK_GIVEUP_PIT){
			//			LOG_POSITION; std::cout<<"learning route from NACK"<<std::endl;
			//			NS_ASSERT_MSG(0, "");
			// get the name of service
			std::ostringstream os;
			Ptr<const Name> NamePtr= header->GetNamePtr();
			Name::const_iterator iterName = NamePtr->begin();
			std::string prefix_str = "/"+*iterName;
			//LOG_POSITION; std::cout<<prefix_str<<std::endl;
			NS_LOG_DEBUG(prefix_str);

			ns3::Ptr<ns3::Node> innode = inFace->GetNode();
			mytype::LoadTable& thistable = loadtablelist[innode];
			mytype::LoadTableEntryKey thiskey;
			thiskey.first = prefix_str;
			thiskey.second = inFace;

			if(thistable.find(thiskey) == thistable.end()){
				// add the FIB entry
				Ptr<ndn::Fib> fib = innode->GetObject<ndn::Fib> ();
				Ptr<ndn::Name> prefix = Create<ndn::Name> (prefix_str);
				Ptr<ndn::Name> root_prefix = Create<ndn::Name> ("/");
				Ptr<ndn::fib::Entry> fibEntry = fib->Find(*root_prefix);
				LOG_POSITION;
				NS_ASSERT_MSG(fibEntry,"NULL pointer");
				mytype::FACELIST& facelist = fibEntry->m_faces.get<1>();
				//				LOG_POSITION; std::cout<<"/ has "<<facelist.size()<<" upwarding faces"<<std::endl;
				NS_LOG_DEBUG("/ has "<<facelist.size()<<" upwarding faces");

				mytype::FACELIST::iterator iterFace = facelist.begin();
				Ptr<ndn::Face> outface;

				for (; iterFace != facelist.end (); iterFace++){
					outface = iterFace->GetFace();
					// add entry in fib
					//					LOG_POSITION; std::cout<<"no fib entry for this prefix"<<std::endl;
					NS_LOG_DEBUG("no fib entry for this prefix");
					fib->Add (*prefix, outface, 1);
					//					LOG_POSITION; Print_FIB_Node(innode);
					// add entry into the LoadTable
					mytype::LoadTableEntry tbentry;
					tbentry.strname = prefix_str;
					tbentry.nsname = prefix;
					tbentry.capacity = 1;
					tbentry.outgoing_interface = outface;
					mytype::LoadTableEntryKey newkey;
					newkey.first = prefix_str;
					newkey.second = outface;
					thistable[newkey] = tbentry;
					//					LOG_POSITION; printf("new entry <%s, %d> is added into the LoadTable\n", tbentry.strname.c_str(), tbentry.outgoing_interface->GetId());
					NS_LOG_DEBUG("new entry <"<<tbentry.strname.c_str()<<", "<<tbentry.outgoing_interface->GetId()<<"> is added into the LoadTable");
				}
			}
			NS_ASSERT_MSG(thistable.find(thiskey)!=thistable.end(), "");
			mytype::LoadTableEntry& ent = thistable[thiskey];
			ent.capacity = 0;
			//			LOG_POSITION; printf("capacity of <%s, %d> in the LoadTable becomes 0 due to NACK\n", ent.strname.c_str(), ent.outgoing_interface->GetId());
			NS_LOG_DEBUG("capacity of <"<<ent.strname.c_str()<<ent.outgoing_interface->GetId()<<"> in the LoadTable becomes 0 due to NACK");
		}
	}
	BaseClass::DidReceiveValidNack(inFace, nackCode, header, origPacket, pitEntry);
}

} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */
