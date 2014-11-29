NDN-4-VMM
=========

simulation codes for paper "Supporting Seamless Virtual Machine Migration via Named Data Networking in Cloud Data Center"

1. install and configure ndnsim;

2. download simulation codes and configure a simulation testbed;
2.1 put following custom strategy files in folder "ndnsim/ns-3/src/ndnSIM/examples/custom-strategies/":
    "mycustom-strategy.h"
    "mycustom-strategy.cc"

2.2 put following custom app and producer files in folder "ndnsim/ns-3/src/ndnSIM/examples/custom-apps/":
    "mycustom-app.h"
    "mycustom-app.cc"
    "mycustom-producer.h"
    "mycustom-producer.cc"

2.3 put following data structure file and simulation script file in folder "ndnsim/ns-3/scratch/":
    "mytype.h"
    "sim-ndn-4-vmm.cc"
    
3. customize the simulation parameters (search "simulation parameters") in "sim-ndn-4-vmm.cc" and run it.
