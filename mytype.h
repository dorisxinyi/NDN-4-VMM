/*
 * @description: define and maintain data structures
 *
 * @Created on: Jul 4, 2013
 * @Author: Ruitao Xie
 */

#ifndef MYTYPE_H_
#define MYTYPE_H_

#include <iostream>
#include <iomanip>
#include <ns3/drop-tail-queue.h>
using namespace std;

#define LOG_POSITION printf("\t%s:ln %d\n", __func__ , __LINE__);
namespace ns3{
	class MyCustomApp;
}

namespace mytype{
typedef std::map<ns3::Ptr<ns3::Node>, ns3::Ptr<ns3::MyCustomApp> > APPLIST;

typedef ns3::Ptr<ns3::ndn::Face> FACEPTR;
typedef ns3::Ptr<ns3::Node> NODEPTR;
typedef ns3::ndn::fib::FaceMetricContainer::type::nth_index<1>::type FACELIST;

typedef std::map<ns3::Ptr<ns3::Node>, bool > IdleIndicatorList;
typedef std::map<ns3::Ptr<ns3::Node>, ns3::Ptr<ns3::DropTailQueue> > PKTQUEUELIST;
typedef std::map<ns3::Ptr<ns3::Packet>, ns3::Ptr<const ns3::ndn::Interest> > INTERETABLE;
typedef std::map<ns3::Ptr<ns3::Node>, INTERETABLE > INTERELIST;

inline void create_queuelist(ns3::NodeContainer& nodelist, PKTQUEUELIST& queuelist)
{
	for(uint32_t i=0; i< nodelist.size(); i++)
	{
		NODEPTR thisnode = nodelist[i];
		ns3::Ptr<ns3::DropTailQueue> queue = ns3::CreateObject<ns3::DropTailQueue> ();
		bool ver = queue->SetAttributeFailSafe("MaxPackets", ns3::UintegerValue (10000));
		NS_ASSERT_MSG(ver, "the attribute setting is failed");
		queuelist[thisnode] = queue;
	}
}

inline void create_interelist(ns3::NodeContainer& nodelist, INTERELIST& interelist)
{
	for(uint32_t i=0; i< nodelist.size(); i++)
	{
		NODEPTR thisnode = nodelist[i];
		interelist[thisnode] = INTERETABLE();
	}
}

inline void create_idlelist(ns3::NodeContainer& nodelist, IdleIndicatorList& idlelist)
{
	for(uint32_t i=0; i< nodelist.size(); i++)
	{
		NODEPTR thisnode = nodelist[i];
		idlelist[thisnode] = true;
	}
}

inline void print_busylist(ns3::NodeContainer& nodelist, IdleIndicatorList& idlelist, std::ostream& os, double now_time)
{
	for(uint32_t i=0; i< nodelist.size(); i++)
	{
		NODEPTR thisnode = nodelist[i];
		if (idlelist[thisnode]){
			os<<now_time<<
					"\t"<<thisnode->GetId()<<
					"\t"<<0<<std::endl;
		}
		else{
			os<<now_time<<
					"\t"<<thisnode->GetId()<<
					"\t"<<1<<std::endl;
		}
	}
}

struct TRITIME{
	double avg_srv_time; //ms
	double start_time; //s
	double busy_time; //s
	uint32_t queue_len;
	// the first element is average service time,
	// the second element is the start time of a service,
	// the third element is the busy time of the current calculation interval.
};

typedef std::map<ns3::Ptr<ns3::Node>, TRITIME > SRVTIMELIST;
inline void create_srvtimelist(ns3::NodeContainer& nodelist, SRVTIMELIST& srvtimelist)
{
	for(uint32_t i=0; i< nodelist.size(); i++)
	{
		NODEPTR thisnode = nodelist[i];
		TRITIME tritime;
		tritime.avg_srv_time = 0;
		tritime.start_time = 0;
		tritime.busy_time = 0;
		tritime.queue_len = 0;
		srvtimelist[thisnode] = tritime;
	}
}

inline void print_srvtimelist(SRVTIMELIST &srvtimelist, IdleIndicatorList idlelist, std::ostream& os, double now_time)
{
	for(SRVTIMELIST::iterator iter=srvtimelist.begin(); iter!=srvtimelist.end(); iter++)
	{
		TRITIME &tritime = iter->second;
		NODEPTR node = iter->first;
		if(idlelist[node] == false){
			tritime.busy_time += now_time - tritime.start_time;
		}
		os<<now_time<<
				"\t"<<node->GetId()<<
				"\t"<<tritime.avg_srv_time<<
				"\t"<<tritime.busy_time<<
				"\t"<<tritime.start_time<<
				"\t"<<tritime.queue_len<<std::endl;
		tritime.busy_time = 0;
		if(idlelist[node] == false){
			tritime.start_time = now_time;
		}
	}
}

struct QUEUEINFO{
	double time;
	uint32_t node_id;
	std::string srv_name;
	int seq_no;
	uint32_t event;
	double srv_time;
	uint32_t queue_len;
};
inline void print_queueinfo(QUEUEINFO info, std::ostream& os){
	os<<info.time<<
			"\t"<<info.node_id<<
			"\t"<<info.srv_name<<
			"\t"<<info.seq_no<<
			"\t"<<info.event<<
			"\t"<<info.srv_time<<
			"\t"<<info.queue_len<<std::endl;
}

uint32_t ctr_rmd = 0;
uint32_t get_unique_id()
{
	return ctr_rmd++;
}

struct LoadTableEntry
{
	std::string strname;
	//ns3::ndn::Name nsname;
	ns3::Ptr<const ns3::ndn::Name> nsname;
	FACEPTR outgoing_interface;
	double avg_inter_arrival_time;
	double last_used_time;
	double util;
	double capacity;
	uint32_t pkt_num;
	double load;
	double pkt_num_tunit;
	double nrm_cap;
	double token;
	int face_id;
	double vir_queue;


	LoadTableEntry():avg_inter_arrival_time(10),
			last_used_time(0),
			util(0),capacity(0),
			pkt_num(0),load(0),pkt_num_tunit(0),
			nrm_cap(0), token(0), face_id(0), vir_queue(0){}
};

struct TokenPtrTableEntry{
	std::string name;
	uint32_t token_ptr;

	TokenPtrTableEntry():token_ptr(0){}
};

typedef std::pair<std::string,FACEPTR> LoadTableEntryKey;
typedef std::map<LoadTableEntryKey,LoadTableEntry> LoadTable;
typedef std::map<NODEPTR,LoadTable> LoadTableList;

typedef std::map<std::string, TokenPtrTableEntry> TokenPtrTable;
typedef std::map<NODEPTR, TokenPtrTable> TokenPtrTableList;

inline void add_nodes_to_tokenptr_table(ns3::NodeContainer& nodelist, TokenPtrTableList& tblist)
{
	for(uint32_t i=0; i< nodelist.size(); i++)
	{
		NODEPTR thisnode = nodelist[i];
		tblist[thisnode] = TokenPtrTable();
	}
}

// new an empty table for each node and add the table into table list
// this function is called when node is created
inline void add_nodes_to_load_table(ns3::NodeContainer& nodelist, LoadTableList& tblist)
{
	for(uint32_t i=0; i< nodelist.size(); i++)
	{
		NODEPTR thisnode = nodelist[i];
		tblist[thisnode] = LoadTable();
	}
}

//add entries to table, if that entry exists in the table, don't touch it
inline void update_table_from_node(ns3::Ptr<ns3::Node> node, LoadTable& table, bool copycapacity)
{
	typedef ns3::ndn::fib::FaceMetricContainer::type::nth_index<1>::type FACELIST;
	ns3::Ptr<ns3::ndn::Fib> fib = node->GetObject<ns3::ndn::Fib>();
	ns3::Ptr<ns3::ndn::fib::Entry>  fentry = fib->Begin();

	int i=0;
	while(fentry != fib->End())
	{
		std::ostringstream os;
		fentry->m_prefix->Print(os);
		std::string prefix = os.str();

		//printf("processing fib entry %d(%s)\n",i,prefix.c_str());
		FACELIST& facelist = fentry->m_faces.get<1>();
		//FACELIST& facelist = fentry->m_faces.get<ns3::ndn::fib::i_metric>();

		int k=0;
		//printf("\tFIB entry %d, #face=%d, #face original =%d\n",k,(int)facelist.size(),(int)fentry->m_faces.size());
		//std::cout<<*fentry<<std::endl;
		for(FACELIST::iterator iter = facelist.begin(); iter!=facelist.end(); iter++)
		{
			//printf("\tprocessing %d-th face\n",k);
			mytype::LoadTableEntryKey thiskey;
			thiskey.first = prefix;
			thiskey.second = iter->GetFace();

			//this entry cannot be found, add to table
			if(table.find(thiskey)==table.end())
			{
				LoadTableEntry tbentry;
				tbentry.strname = prefix;
				tbentry.nsname = fentry->m_prefix;
				tbentry.capacity = iter->GetRoutingCost();
				tbentry.outgoing_interface = iter->GetFace();
				tbentry.token = 0;

				/*//capacity should be copied from existing entry
				if(copycapacity && !table.empty())
				{
					tbentry.capacity = table.begin()->second.capacity;
				}*/

				table[thiskey] = tbentry;
			}
			k++;
		}
		fentry = fib->Next(fentry);
		i++;

	}
}

inline LoadTable create_table_from_node(ns3::Ptr<ns3::Node> node)
{
	LoadTable table;
	update_table_from_node(node,table,false);
	return table;
}

inline void add_table_by_nodelist(ns3::NodeContainer& nodelist, LoadTableList& tablelist)
{
	for(uint32_t i=0; i<nodelist.size(); i++)
	{
		LoadTable newtable = create_table_from_node(nodelist[i]);
		tablelist[nodelist[i]] = newtable;
	}
}

inline void print_table(LoadTable& table, std::ostream& os)
{
	for(LoadTable::iterator iter = table.begin(); iter != table.end(); iter++ )
	{
		LoadTableEntryKey key = iter->first;
		LoadTableEntry ent = iter->second;
		os<<"name = "<<ent.strname<<"\n"
					<<"face_id = "<<ent.outgoing_interface->GetId()<<"\n"
					<<"capacity = "<<ent.capacity<<"\n"
					<<"utilization = "<<ent.util<<"\n"
					<<"avg_inter_time = "<<ent.avg_inter_arrival_time<<"\n"
					<<"last_used_time = "<<ent.last_used_time<<"\n"
					<<"pkt_num = "<<ent.pkt_num<<"\n"
					<<"load = "<<ent.load<<"\n"
					<<"token = "<<ent.token<<"\n"
					<<"vir_queue = "<<ent.vir_queue<<std::endl;
	}
}

inline void print_entry(LoadTableEntry ent, std::ostream& os, double now_time){
	os<<"time = "<<now_time<<"\n"
			<<"name = "<<ent.strname<<"\n"
			<<"face_id = "<<ent.outgoing_interface->GetId()<<"\n"
			<<"capacity = "<<ent.capacity<<"\n"
			<<"utilization = "<<ent.util<<"\n"
			<<"avg_inter_time = "<<ent.avg_inter_arrival_time<<"\n"
			<<"last_used_time = "<<ent.last_used_time<<"\n"
			<<"pkt_num = "<<ent.pkt_num<<"\n"
			<<"load = "<<ent.load<<"\n"
			<<"token = "<<ent.token<<"\n"
			<<"vir_queue = "<<ent.vir_queue<<std::endl;
}

inline void print_table_list(LoadTableList& tblist, std::ostream& os, double now_time)
{
	for(LoadTableList::iterator iter=tblist.begin(); iter!=tblist.end(); iter++)
	{
//		os<<"node id="<<iter->first->GetId()<<" #entry="<<iter->second.size()<<std::endl;
//		print_table(iter->second,os);
		LoadTable& table = iter->second;
		for(LoadTable::iterator entiter = table.begin(); entiter != table.end(); entiter++ )
		{
			LoadTableEntryKey key = entiter->first;
			LoadTableEntry ent = entiter->second;

			os<<now_time<<
					"\t"<<iter->first->GetId()<<
					"\t"<<ent.strname<<
					"\t"<<ent.outgoing_interface->GetId()<<
					"\t"<<ent.capacity<<
					"\t"<<ent.token<<
					"\t"<<ent.util<<
					"\t"<<ent.avg_inter_arrival_time<<
					"\t"<<ent.last_used_time<<
					"\t"<<ent.pkt_num<<
					"\t"<<ent.load<<
					"\t"<<ent.pkt_num_tunit<<
					"\t"<<ent.vir_queue<<std::endl;
		}
	}
}

inline void cal_util(LoadTableList& tblist, double timestep)
{
	//double now_time = Simulator::Now ().ToDouble (Time::S);
	for(LoadTableList::iterator iter=tblist.begin(); iter!=tblist.end(); iter++)
	{
		LoadTable& table = iter->second;
		for(LoadTable::iterator entiter = table.begin(); entiter != table.end(); entiter++ )
		{
			LoadTableEntryKey key = entiter->first;
			LoadTableEntry& ent = entiter->second;

			double load_tmp = ent.pkt_num_tunit / timestep;
			ent.load = load_tmp;
			ent.util = ent.load / ent.capacity;
			ent.pkt_num_tunit = 0;
		}
	}
}

template<typename T>
inline T** alloc_array2(int dim1, int dim2)
{
	typedef T* TPTR;
	T** arr = new TPTR[dim1];
	for(int i=0;i<dim1;i++)
	{
		arr[i] = new T[dim2];
	}
	return arr;
}

template<typename T>
inline void delete_array2(T** arr,int dim1)
{
	for(int i=0;i<dim1;i++)
	{
		delete[] arr[i];
	}
	delete[] arr;
}

}


#endif /* MYTYPE_H_ */
