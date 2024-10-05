To run the NS3 file assignment2.cc, copy the file to the scratch folder of NS3 and run the following command:
./ns3 run scratch/demo -- --simTime=10 --scheduler=MT --speed=0 --RngRun=10 --fullBufferFlag=1

scheduler=MT is the Max Throughput scheduler
scheduler=RR is the Round Robin scheduler
scheduler=PF is the Proportional Fair scheduler
scheduler=BATS is the Blind Average Throughput Schedulers

simTime is simulation time
speed is the speed of the nodes in the network

fullBufferFlag=1 is to enable full buffer mode
fullBufferFlag=0 is to disable full buffer mode

RngRun is the random number generator seed

The code internally will run for 5 times by incrementing seed value by 1 each time. 
The output will be stored in the file DlRlcStats-<scheduler>-<speed>-<seed>.csv 

Place the data in the fullbuffer or nonfullbuffer folder in graphs folder and run the python script located in graph folders to generate the graphs.
Copy the data printed by python file and paste it in the DAT file located in the graph folder to generate the graph.

Run the following command to generate the graph:
gnuplot script.gp

To get data for graph 1, uncomment the SINR code in the assignment2.cc file and run the command.
./ns3 run scratch/demo

rem.out file is generated which is data for graph 1.