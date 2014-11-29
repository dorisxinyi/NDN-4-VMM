/*
 * @Description: We will construct a fat-tree topology and run simulations on it
 * @Author: Ruitao Xie, City University of Hong Kong
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <map>
#include <sstream>
#include <time.h>

// include necessary files in
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/random-variable.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

// include files to trace
// ndn::L3AggregateTracer
#include <ns3/ndnSIM/utils/tracers/ndn-l3-aggregate-tracer.h>
// ndn::L3RateTracer
#include <ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.h>
// ndn::CsTracer
#include <ns3/ndnSIM/utils/tracers/ndn-cs-tracer.h>
// ndn::AppDelayTracer
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/examples/custom-strategies/mycustom-strategy.h"
#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/examples/custom-strategies/mycustom-strategy.cc"
#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/examples/custom-apps/mycustom-app.h"
#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/examples/custom-apps/mycustom-app.cc"
#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/examples/custom-apps/mycustom-producer.h"
#include "/home/sunshine/workspace/ndnsim/ns-3/src/ndnSIM/examples/custom-apps/mycustom-producer.cc"

#include "ns3/log.h"

// alias of data structures
mytype::LoadTableList loadtablelist;
mytype::TokenPtrTableList tptablelist;
mytype::PKTQUEUELIST queuelist;
mytype::INTERELIST interelist;
mytype::IdleIndicatorList idlelist;
mytype::SRVTIMELIST srvtimelist;

// output files
#define _ALGOUTFILENAME(x,y,z) "scratch/utilization_trace_runnum"#x"_alg"#y"_req"#z".txt"
#define ALGOUTFILENAME(x,y,z) _ALGOUTFILENAME(x,y,z)
std::ofstream algoutfile;

#define _FACEFILENAME(x,y,z) "scratch/interface_trace_runnum"#x"_alg"#y"_req"#z".txt"
#define FACEFILENAME(x,y,z) _FACEFILENAME(x,y,z)
std::ofstream facefile;

#define _BUSYTIMEFILENAME(x,y,z) "scratch/busytime_trace_runnum"#x"_alg"#y"_req"#z".txt"
#define BUSYTIMEFILENAME(x,y,z) _BUSYTIMEFILENAME(x,y,z)
std::ofstream busytimefile;

#define _QUEUEFILENAME(x,y,z) "scratch/queue_trace_runnum"#x"_alg"#y"_req"#z".txt"
#define QUEUEFILENAME(x,y,z) _QUEUEFILENAME(x,y,z)
std::ofstream queuefile;

#define _BUSYSTATENAME(x,y,z) "scratch/busy_state_runnum"#x"_alg"#y"_req"#z".txt"
#define BUSYSTATENAME(x,y,z) _BUSYSTATENAME(x,y,z)
std::ofstream busystatefile;

/*************************
* simulation parameters
*************************/

#define REQ_FREQ 600 // full utilization is 1280; the frequency of dispatched requests from gateway
#define _TOSTRING(x) #x
#define TOSTRING(x) _TOSTRING(x)
#define REQ_FREQ_STRINGVAL StringValue(TOSTRING(REQ_FREQ))
//#define REQ_FREQ_STRINGVAL StringValue("640") // full utilization is 1280; the frequency of dispatched requests from gateway
#define STOP_TIME 18.0001 // simulation stopping time // LB test: 95.0001
#define TIME_CHANGE_CAPACITY 30.00001
#define TIME_CHANGE_CAPACITY_2 60.00001
#define FRE_CAL_UTIL 1 // frequency (s) of utilization calculation
#define FRE_LBTAB_PRT Seconds(1) // frequency (s) of printing load balance table
#define TIME_MIG_STOP 8.00001 // migration stopping time
#define TIME_MIG_START 10.00001 // migration starting time
#define TEST 1 // type of simulation test: (test load balancing(0); test migration(1))
#define FRE_RANDOM 0 // type of randomness of dispatched requests from gateway: (Exponential distribution(1); Uniform distribution(2))
#define LINK_PROPG_DELAY 10 //ms // the delay (ms) of link propagation
#define LINK_DATA_RATE "100Mbps" // the transmission rate of link
#define MIRG_CAP 300 // the capacity of VMs in the migration simulation
//#define RUN_NUM 6 // the run number of GNR, i.e. the index of substream

// static constant
#define TEST_LB 0
#define TEST_MIG 1
#define FRE_EXP 1
#define FRE_UNIFORM 2

using namespace ns3;
using namespace std;

void
PeriodicStatsPrinter (Ptr<Node> node, Time next)
{
	Ptr<ndn::Pit> pit = node->GetObject<ndn::Pit> ();
	Ptr<ndn::Fib> fib = node->GetObject<ndn::Fib> ();

	std::cout << Simulator::Now ().ToDouble (Time::S) << "\t"
			<< node->GetId () << "\t"
			<< Names::FindName (node) << "\t"
			<< pit->GetSize () << "\t"
			<< fib->GetSize () << "\n";

	Ptr<ndn::fib::Entry > entry = fib->Begin ();
	for (unsigned int i = 0; i < fib->GetSize(); i++ )
	{
		std::cout << "FIB\t"
				<< Simulator::Now ().ToDouble (Time::S) << "\t"
				<< node->GetId () << "\t"
				<< Names::FindName (node) << "\t"
				<< *entry->m_prefix << "\t"
				<< *entry << "\n";
		entry = fib->Next (entry);
	}

	/*
	Ptr<ndn::pit::Entry > pit_entry = pit->Begin ();
	for (unsigned int i = 0; i < pit->GetSize(); i++ )
	{
		std::cout << "PIT\t"
				<< Simulator::Now ().ToDouble (Time::S) << "\t"
				<< node->GetId () << "\t"
				<< Names::FindName (node) << "\t"
				<< *pit_entry << "\n";
		pit_entry = pit->Next (pit_entry);
	}
	 */
	Simulator::Schedule (next, PeriodicStatsPrinter, node, next);
}


void
PeriodicLoadTablePrinter (Time next)
{
	double now_time = Simulator::Now ().ToDouble (Time::S);
	mytype::print_table_list(loadtablelist, algoutfile, now_time);
	mytype::print_srvtimelist(srvtimelist, idlelist, busytimefile, now_time);
	Simulator::Schedule (next, PeriodicLoadTablePrinter, next);
}

void
PeriodicBusyPrinter (ns3::NodeContainer& nodelist, Time next)
{
	double now_time = Simulator::Now ().ToDouble (Time::S);
	mytype::print_busylist(nodelist, idlelist, busystatefile, now_time);
	Simulator::Schedule (next, PeriodicBusyPrinter, nodelist, next);
}

void
PeriodicCalUtil (double timestep)
{
	Time next = Seconds(timestep);
	mytype::cal_util(loadtablelist, timestep);
	Simulator::Schedule (next, PeriodicCalUtil, timestep);
}

void
AppUpdateCap (ns3::Ptr<ns3::MyCustomApp> _app, double _cap)
{
	_app->UpdateCap(_cap);
}


void
AppStop (ns3::Ptr<ns3::MyCustomApp> _app)
{
	//_app->StopApplication(); // this function will delete the local face
	_app->UpdateCap(0);
}
void
ProducerUpdateCap (ns3::NodeContainer &nodelist, mytype::SRVTIMELIST *_srvtimelist, double cap)
{
	mytype::SRVTIMELIST& srvtimelist = *_srvtimelist;
	for(uint32_t i=0; i< nodelist.size(); i++)
	{
		ns3::Ptr<ns3::Node> thisnode = nodelist[i];
		//srvtimelist[thisnode].avg_srv_time = 1000/cap;
		mytype::TRITIME &tritime = srvtimelist[thisnode];
		tritime.avg_srv_time = 1000/cap;
		//NS_LOG_DEBUG("the average service time of node "<<thisnode->GetId()<<" is "<<srvtimelist[thisnode].avg_srv_time);
		//double now_time = Simulator::Now ().ToDouble (Time::S);
		//std::cout<<"time "<<now_time<<" the average service time of node "<<thisnode->GetId()<<" is "<<tritime.avg_srv_time<<std::endl;
	}
}

struct fib_update{
	int num_pod;
	int num_group;
	int num_core;
	int num_agg;
	int num_edge;
	int num_host;
	int num_gateway;
	ns3::NodeContainer* core;
	ns3::NodeContainer* agg;
	ns3::NodeContainer* edge;
	ns3::NodeContainer** host;
	ns3::NodeContainer gateway;
	double* cap;
	ns3::ndn::StackHelper& ndnHelper;

	fib_update(	int _num_pod,
			int _num_group,
			int _num_core,
			int _num_agg,
			int _num_edge,
			int _num_host,
			int _num_gateway,
			ns3::NodeContainer* _core,
			ns3::NodeContainer* _agg,
			ns3::NodeContainer* _edge,
			ns3::NodeContainer** _host,
			ns3::NodeContainer _gateway,
			double* _cap,
			ns3::ndn::StackHelper& _ndnHelper ):
				num_pod(_num_pod),
				num_group(_num_group),
				num_core(_num_core),
				num_agg(_num_agg),
				num_edge(_num_edge),
				num_host(_num_host),
				num_gateway(_num_gateway),
				core(_core),
				agg(_agg),
				edge(_edge),
				host(_host),
				gateway(_gateway),
				cap(_cap),
				ndnHelper(_ndnHelper){}
};

void
AddUpwardFIB (fib_update thisfib_update){
	int num_pod = thisfib_update.num_pod;
	int num_group  = thisfib_update.num_group;
	int num_core = thisfib_update.num_core;
	int num_agg = thisfib_update.num_agg;
	int num_edge = thisfib_update.num_edge;
	int num_host = thisfib_update.num_host;
	int num_gateway = thisfib_update.num_gateway;
	ns3::NodeContainer* core = thisfib_update.core;
	ns3::NodeContainer* agg = thisfib_update.agg;
	ns3::NodeContainer* edge = thisfib_update.edge;
	ns3::NodeContainer** host = thisfib_update.host;
	ns3::NodeContainer gateway = thisfib_update.gateway;
	//double* cap = thisfib_update.cap;
	ns3::ndn::StackHelper& ndnHelper = thisfib_update.ndnHelper;

	int i;
	int j;
	int h;
	// =============== add FIB entry ==========================//
	// host
	for (i=0;i<num_pod;i++){
		for (j=0;j<num_edge; j++){
			for (h=0; h< num_host;h++){
				ndnHelper.AddRoute(host[i][j].Get(h), "/", edge[i].Get(j), 1);
			}
		}
	}
	// edge
	for (i=0;i<num_pod;i++){
		for (j=0;j<num_edge; j++){
			for (h=0;h<num_agg; h++){
				//cout<<"node#"<< (edge[i].Get(j))->GetId() <<"  other node#"<< (agg[i].Get(h))->GetId() <<endl;
				ndnHelper.AddRoute(edge[i].Get(j), "/", agg[i].Get(h), 1);
			}
		}
	}
	// aggregation
	for (i=0; i<num_group; i++){		// num_group maps to num_agg
		for (j=0; j < num_core; j++){
			for (h=0; h < num_pod; h++){
				//cout<<"node#"<< (agg[h].Get(i))->GetId() <<"  other node#"<< (core[i].Get(j))->GetId() <<endl;
				ndnHelper.AddRoute(agg[h].Get(i), "/", core[i].Get(j), 1); // agg(pod).Get(agg)<----->core(group).Get(core)
			}
		}
	}
	// gateway
	for (i=0; i<num_gateway;i++){
		for (j=0; j<num_group;j++){
			for (h=0; h<num_core; h++){
				ndnHelper.AddRoute(gateway.Get(i), "/", core[j].Get(h), 1);
			}
		}
	}
	// add a shadow FIB entry to core, so that DoPropagateInterest function can be called in core
	for (i=0; i<num_group; i++){
		for (j=0; j<num_core;j++){
			ndnHelper.AddRoute(core[i].Get(j), "/", core[i].Get(j), 0);
		}
	}
}


// Main function
int 
main(int argc, char *argv[])
{
	// Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
	CommandLine cmd;
	cmd.Parse (argc, argv);

	int RUN_NUM = 0;
	for(int iter=0; iter<1; iter++){


		mytype::LoadTableList loadtablelist_new;
		mytype::TokenPtrTableList tptablelist_new;
		mytype::PKTQUEUELIST queuelist_new;
		mytype::INTERELIST interelist_new;
		mytype::IdleIndicatorList idlelist_new;
		mytype::SRVTIMELIST srvtimelist_new;
		loadtablelist = loadtablelist_new;
		tptablelist = tptablelist_new;
		queuelist = queuelist_new;
		interelist = interelist_new;
		idlelist = idlelist_new;
		srvtimelist = srvtimelist_new;

#if false
		LogComponentEnable("ndn.fw.MyCustomStrategy", LOG_LEVEL_INFO);
		LogComponentEnable("ndn.fw.MyCustomStrategy", LOG_PREFIX_FUNC);
		LogComponentEnable("ndn.fw.MyCustomStrategy", LOG_PREFIX_TIME);
		LogComponentEnable("ndn.fw.MyCustomStrategy", LOG_PREFIX_NODE);
#endif

#if false
		LogComponentEnable("MyCustomProducer", LOG_LEVEL_INFO);
		LogComponentEnable("MyCustomProducer", LOG_PREFIX_FUNC);
		LogComponentEnable("MyCustomProducer", LOG_PREFIX_TIME);
		LogComponentEnable("MyCustomProducer", LOG_PREFIX_NODE);
#endif

		//srand(time(NULL));
		//RngSeedManager::SetSeed (time(NULL));
		RngSeedManager::SetRun(RUN_NUM);

		std::ostringstream stringstream4;
		stringstream4<<"scratch/utilization_trace_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		std::string filename = stringstream4.str();
		//algoutfile.open(ALGOUTFILENAME(RUN_NUM, LB_ALG, REQ_FREQ),std::ios_base::trunc);
		algoutfile.open(filename.c_str(), std::ios_base::trunc);
		if(!algoutfile.is_open())
		{
			cerr<<"cannot open file "<<filename<<std::endl;
			exit(0);
		}
		else{
			algoutfile<<"time\tnode_id\tname\tface_id\tcap\ttoken\tutil\t"<<
					"avg_inter_time\tlast_time\tpkt_num\tload\tpkt_num_tunit\tvir_queue\n";
		}

		std::ostringstream stringstream5;
		stringstream5<<"scratch/interface_trace_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		filename = stringstream5.str();
		facefile.open(filename.c_str(),std::ios_base::trunc);
		if(!facefile.is_open())
		{
			cerr<<"cannot open file "<<filename<<std::endl;
			exit(0);
		}
		else{
			facefile<<"time\tnode_id\tface_id\tseq_no\tNACK\n";
		}

		std::ostringstream stringstream6;
		stringstream6<<"scratch/busytime_trace_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		filename = stringstream6.str();
		busytimefile.open(filename.c_str(),std::ios_base::trunc);
		if(!busytimefile.is_open())
		{
			cerr<<"cannot open file "<<filename<<std::endl;
			exit(0);
			NS_ASSERT_MSG(0, "");
		}
		else{
			busytimefile<<"time\tnode_id\tavg_srv_time\tbusy_time\tstart_time\tqueue_len\n";
		}

		std::ostringstream stringstream7;
		stringstream7<<"scratch/queue_trace_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		filename = stringstream7.str();
		queuefile.open(filename.c_str(),std::ios_base::trunc);
		if(!queuefile.is_open())
		{
			cerr<<"cannot open file "<<filename<<std::endl;
			exit(0);
			NS_ASSERT_MSG(0, "");
		}
		else{
			queuefile<<"time\tnode_id\tsrv_name\tseq_no\tevent\tsrv_time\tqueue_len\n";
		}

		std::ostringstream stringstream8;
		stringstream8<<"scratch/busy_state_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		filename = stringstream8.str();
		busystatefile.open(filename.c_str(), std::ios_base::trunc);
		if(!busystatefile.is_open())
		{
			cerr<<"cannot open file "<<filename<<std::endl;
			exit(0);
			NS_ASSERT_MSG(0, "");
		}
		else{
			busystatefile<<"time\tnode_id\tbusy_state\n";
		}

		//=========== Define parameters based on value of k ===========//
		int k = 4;			// number of ports per switch
		int num_pod = k;		// number of pod
		int num_host = (k/2);		// number of hosts under a switch
		int num_edge = (k/2);		// number of edge switch in a pod
		int num_agg = (k/2);		// number of aggregation switch in a pod
		int num_group = k/2;		// number of group of core switches
		int num_core = (k/2);		// number of core switch in a group
		int total_host = k*k*k/4;	// number of hosts in the entire network

		// Output some useful information
		std::cout << "Value of k =  "<< k<<"\n";
		std::cout << "Total number of hosts =  "<< total_host<<"\n";
		std::cout << "Number of hosts under each switch =  "<< num_host<<"\n";
		std::cout << "Number of edge switch under each pod =  "<< num_edge<<"\n";
		std::cout << "------------- "<<"\n";

		// Initialize other variables
		int i = 0;
		int j = 0;
		int h = 0;

		double cap0 [4];
		cap0[0] = 1*20;
		cap0[1] = 2*20;
		cap0[2] = 5*20;
		cap0[3] = 8*20;
		double cap1 [4];
		cap1[0] = 4*20;
		cap1[1] = 4*20;
		cap1[2] = 4*20;
		cap1[3] = 4*20;
		double cap2 [4];
		cap2[0] = 8*20;
		cap2[1] = 5*20;
		cap2[2] = 2*20;
		cap2[3] = 1*20;

		// Install CCNx stack on all nodes
		ndn::StackHelper ndnHelper;
		//	ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::MyCustomStrategy");
		ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::MyCustomStrategy", "EnableNACKs", "true");
		ndnHelper.SetContentStore ("ns3::ndn::cs::Stats::Lru", "MaxSize", "1");
		ndnHelper.SetPit ("ns3::ndn::pit::Lru", "MaxSize", "0");

		//=========== Creation of Node Containers ===========//
		NodeContainer core[num_group];				// NodeContainer for core switches
		for (i=0; i<num_group;i++){
			core[i].Create (num_core);
			mytype::add_nodes_to_load_table(core[i],loadtablelist);
			mytype::add_nodes_to_tokenptr_table(core[i],tptablelist);
		}

		NodeContainer agg[num_pod];				// NodeContainer for aggregation switches
		for (i=0; i<num_pod;i++){
			agg[i].Create (num_agg);
			mytype::add_nodes_to_load_table(agg[i],loadtablelist);
			mytype::add_nodes_to_tokenptr_table(agg[i],tptablelist);
		}

		NodeContainer edge[num_pod];				// NodeContainer for edge switches
		for (i=0; i<num_pod;i++){
			edge[i].Create (num_edge);
			mytype::add_nodes_to_load_table(edge[i],loadtablelist);
			mytype::add_nodes_to_tokenptr_table(edge[i],tptablelist);
		}
		NodeContainer** host = mytype::alloc_array2<NodeContainer>(num_pod,num_edge);
		//NodeContainer host[num_pod][num_edge];		// NodeContainer for hosts
		for (i=0; i<k;i++){
			for (j=0;j<num_edge;j++){
				host[i][j].Create (num_host);
				mytype::add_nodes_to_load_table(host[i][j],loadtablelist);
				mytype::add_nodes_to_tokenptr_table(host[i][j],tptablelist);
				mytype::create_queuelist(host[i][j], queuelist);
				mytype::create_interelist(host[i][j], interelist);
				mytype::create_idlelist(host[i][j], idlelist);
				mytype::create_srvtimelist(host[i][j], srvtimelist);
				if(TEST == TEST_LB){
					ProducerUpdateCap (host[i][j], &srvtimelist, cap0[i]);
				}
				else if(TEST == TEST_MIG){
					if(i<2)
						ProducerUpdateCap (host[i][j], &srvtimelist, MIRG_CAP);
					else
						ProducerUpdateCap (host[i][j], &srvtimelist, 0);
				}

			}
		}

#if false
		// use for test
		for(mytype::SRVTIMELIST::iterator iter=srvtimelist.begin(); iter!=srvtimelist.end(); iter++)
		{
			mytype::NODEPTR node = iter->first;
			std::cout<<node->GetId()<<std::endl;
		}

		for (i=0; i<k;i++){
			for (j=0;j<num_edge;j++){
				std::cout<<host[i][j][0]->GetId()<<std::endl;
				std::cout<<host[i][j][1]->GetId()<<std::endl;
			}
		}
		NS_ASSERT_MSG(0, "");
#endif


		// Initialize PointtoPoint helper
		// setting default parameters for PointToPoint links and channels
		Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("20"));
		PointToPointHelper p2p;
		p2p.SetDeviceAttribute ("DataRate", StringValue (LINK_DATA_RATE));
		p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (LINK_PROPG_DELAY)));

		//=========== Connect edge switches to hosts ===========//
		NetDeviceContainer eh[num_pod][num_agg][num_host];
		for (i=0;i<num_pod;i++){
			for (j=0;j<num_edge; j++){
				for (h=0; h< num_host;h++){
					eh[i][j][h] = p2p.Install (edge[i].Get(j), host[i][j].Get(h));
					//cout<<"node#"<< (edge[i].Get(j))->GetId() <<"  other node#"<< (host[i][j].Get(h))->GetId() <<endl;
				}
			}
		}
		//LOG_POSITION; std::cout << "Finished connecting edge switches and hosts  "<< "\n";

		//=========== Connect aggregate switches to edge switches ===========//
		NetDeviceContainer ae[num_pod][num_agg][num_edge];
		for (i=0;i<num_pod;i++){
			for (j=0;j<num_agg;j++){
				for (h=0;h<num_edge;h++){
					ae[i][j][h] = p2p.Install(agg[i].Get(j), edge[i].Get(h));
					//cout<<"node#"<< (agg[i].Get(j))->GetId() <<"  other node#"<< (edge[i].Get(h))->GetId() <<endl;
				}
			}
		}
		//LOG_POSITION; std::cout << "Finished connecting aggregation switches and edge switches  "<< "\n";

		//=========== Connect core switches to aggregate switches ===========//
		NetDeviceContainer ca[num_group][num_core][num_pod];
		for (i=0; i<num_group; i++){		// num_group maps to num_agg
			for (j=0; j < num_core; j++){
				for (h=0; h < num_pod; h++){
					ca[i][j][h] = p2p.Install(core[i].Get(j), agg[h].Get(i)); // agg(pod).Get(agg)<----->core(group).Get(core)
					//cout<<"node#"<< (core[i].Get(j))->GetId() <<"  other node#"<< (agg[h].Get(i))->GetId() <<endl;
				}
			}
		}
		//LOG_POSITION; std::cout << "Finished connecting core switches and aggregation switches  "<< "\n";

		//========================= the gateway ==========================//
		int num_gateway = 2;
		NodeContainer gateway;
		gateway.Create(num_gateway);
		mytype::add_nodes_to_load_table(gateway,loadtablelist);
		mytype::add_nodes_to_tokenptr_table(gateway, tptablelist);

		for (i=0; i<num_gateway;i++){
			for (j=0; j<num_group;j++){
				for (h=0; h<num_core; h++){
					p2p.Install(gateway.Get(i), core[j].Get(h));
					//LOG_POSITION; cout<<"node#"<<gateway.Get(i)->GetId() << " other node#"<<core[j].Get(h)->GetId()<<endl;
				}
			}
		}
		ndnHelper.InstallAll ();

		fib_update params(num_pod, num_group, num_core, num_agg, num_edge, num_host, num_gateway,
				core, agg, edge, host, gateway, cap0, ndnHelper);
		AddUpwardFIB(params);

		//=============== Installing applications ================//
		// control message consumer
		mytype::APPLIST applist_ctr;
		ndn::AppHelper controlHelper ("ns3::MyCustomApp");
		for (i=0; i<num_pod;i++){
			for (j=0;j<num_edge;j++){
				for (h=0; h<num_host;h++){
					ns3::ApplicationContainer app_cont_ctr = controlHelper.Install(host[i][j].Get(h));
					ns3::Ptr<ns3::MyCustomApp> app_ctr = ns3::DynamicCast<ns3::MyCustomApp>(*app_cont_ctr.Begin());
					NS_ASSERT_MSG(app_ctr!=NULL,"");
					app_ctr->SetName ("service-1");
					if (TEST == TEST_LB)
						app_ctr->SetCap (cap0[i]);
					else if (TEST == TEST_MIG){
						if (i < 2)
							app_ctr->SetCap (MIRG_CAP);
						else
							app_ctr->SetCap (0);
					}
					applist_ctr[host[i][j].Get(h)] = app_ctr;
					if (TEST == TEST_LB){
						Simulator::Schedule (Seconds(TIME_CHANGE_CAPACITY), AppUpdateCap, applist_ctr[host[i][j].Get(h)], cap1[i]);
						Simulator::Schedule (Seconds(TIME_CHANGE_CAPACITY_2), AppUpdateCap, applist_ctr[host[i][j].Get(h)], cap2[i]);
						Simulator::Schedule (Seconds(TIME_CHANGE_CAPACITY), ProducerUpdateCap, host[i][j], &srvtimelist, cap1[i]);
						Simulator::Schedule (Seconds(TIME_CHANGE_CAPACITY_2), ProducerUpdateCap, host[i][j], &srvtimelist, cap2[i]);
					}
					else if (TEST == TEST_MIG){
						if (i == 0){
							Simulator::Schedule (Seconds(TIME_MIG_STOP), AppUpdateCap, applist_ctr[host[i][j].Get(h)], 0);
							//Simulator::Schedule (Seconds(TIME_MIG_STOP), ProducerUpdateCap, host[i][j], &srvtimelist, 0);
						}
						else if(i == 2){
							Simulator::Schedule (Seconds(TIME_MIG_START), AppUpdateCap, applist_ctr[host[i][j].Get(h)], MIRG_CAP);
							Simulator::Schedule (Seconds(TIME_MIG_START), ProducerUpdateCap, host[i][j], &srvtimelist, MIRG_CAP);
						}
					}
				}
			}
		}

		// Consumer
		ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
		consumerHelper.SetAttribute ("Frequency", REQ_FREQ_STRINGVAL); // # interests per second
		if (FRE_RANDOM == FRE_UNIFORM){
			consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));
		}
		if (FRE_RANDOM == FRE_EXP){
			consumerHelper.SetAttribute ("Randomize", StringValue ("exponential"));
		}
		consumerHelper.SetPrefix ("/service-1"); // Consumer will request /prefix/0, /prefix/1, ...
		ns3::ApplicationContainer app_cont_req = consumerHelper.Install (gateway[0]);
		ns3::Ptr<ns3::Application> app_req = ns3::DynamicCast<ns3::Application>(*app_cont_req.Begin());
		NS_ASSERT_MSG(app_req!=NULL,"");
		app_req->SetStartTime(Seconds(1));

		// Producer
		ndn::AppHelper producerHelper ("ns3::ndn::MyCustomProducer");
		producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
		// Producer will reply to all requests starting with /prefix
		producerHelper.SetPrefix ("/service-1");
		for (i=0; i<k;i++){
			for (j=0;j<num_edge;j++){
				for (h=0; h<num_host; h++){
					producerHelper.Install (host[i][j].Get(h));
				}
			}
		}
		//producerHelper.Install (host[0][0].Get(0));

		//=======================create the loadtable==================//
		for(i=0; i<num_group;i++)
			mytype::add_table_by_nodelist(core[i],loadtablelist);
		for(i=0;i<num_pod;i++)
			mytype::add_table_by_nodelist(agg[i],loadtablelist);
		for(i=0;i<num_pod;i++)
			mytype::add_table_by_nodelist(edge[i],loadtablelist);
		for (i=0;i<num_pod;i++){
			for (j=0;j<num_edge; j++){
				mytype::add_table_by_nodelist(host[i][j],loadtablelist);
			}
		}
		mytype::add_table_by_nodelist(gateway,loadtablelist);
		//double now_time = Simulator::Now().ToDouble(Time::S);
		//LOG_POSITION; mytype::print_table_list(loadtablelist,cout, now_time);

		// ================== display the FIB entries ===========//
#if false
		std::cout << "Displaying FIB entries on all switches  "<< "\n";
		for (i=0; i<num_group; i++){
			for (j=0; j<num_core; j++){
				PeriodicStatsPrinter (core[i].Get(j), Seconds (1));
			}
		}
		for (i=0;i<num_pod;i++){
			for (j=0;j<num_edge; j++){
				PeriodicStatsPrinter (edge[i].Get(j), Seconds (1)); // the time parameter is useless
				PeriodicStatsPrinter (agg[i].Get(j), Seconds (1));
			}
		}
		for (i=0; i<num_pod; i++){
			for (j=0; j<num_edge; j++){
				for (h=0; h<num_host; h++){
					PeriodicStatsPrinter (host[i][j].Get(h), Seconds (1));
				}
			}
		}
		for (i=0; i<num_gateway; i++){
			PeriodicStatsPrinter (gateway.Get(i), Seconds (1));
		}
#endif
		//============= print the load table =========//
		// IMPORTANT: calculate first and print second
		PeriodicCalUtil(FRE_CAL_UTIL);
		PeriodicLoadTablePrinter(FRE_LBTAB_PRT);
		for (i=0;i<num_pod;i++){
			for (j=0;j<num_edge; j++){
				PeriodicBusyPrinter(host[i][j],Seconds(0.001));
			}
		}

		//=========== Start the simulation ===========//
		Simulator::Stop (Seconds(STOP_TIME));

		std::ostringstream stringstream;
		stringstream<<"scratch/aggregate-trace_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		filename = stringstream.str();
		boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
		aggTracers = ndn::L3AggregateTracer::InstallAll (filename, Seconds (0.5));
		std::ostringstream stringstream1;
		stringstream1<<"scratch/rate-trace_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		filename = stringstream1.str();
		boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3RateTracer> > >
		rateTracers = ndn::L3RateTracer::InstallAll (filename, Seconds (0.5));
		std::ostringstream stringstream2;
		stringstream2<<"scratch/cs-trace_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		filename = stringstream2.str();
		boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::CsTracer> > >
		csTracers = ndn::CsTracer::InstallAll (filename, Seconds (0.5));
		std::ostringstream stringstream3;
		stringstream3<<"scratch/app-delays-trace_runnum"<<RUN_NUM<<"_alg"<<LB_ALG<<"_req"<<REQ_FREQ<<".txt";
		filename = stringstream3.str();
		boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::AppDelayTracer> > >
		appDelaytracers = ndn::AppDelayTracer::InstallAll (filename);

		Simulator::Run ();
		Simulator::Destroy ();
		algoutfile.close();
		facefile.close();
		busytimefile.close();
		queuefile.close();
		busystatefile.close();
		mytype::delete_array2(host,num_pod);
		std::cout << "Simulation finished "<<"\n";

		RUN_NUM ++;
	}

	return 0;
}

