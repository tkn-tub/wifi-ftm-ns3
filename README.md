# wifi-ftm-ns3
Standard compliant implementation of the IEEE 802.11-FTM protocol in ns3.


How to use:
This file describes the basic usage of the FTM Model in ns-3.
An example file can be found in the scratch directory, named "FTM-Example.cc".
!!!!  IMPORTANT !!!!
As .map files are ignored in the git repository, you should create your own .map file first and change the name in the example file. This is found in line 177.

Before usage:
If a wireles error model is to be used, it should first be generated with the ftm_map_generator.py in src/wifi/ftm_map directory.

1st:
Enable FTM support on all the nodes via the regular wifi mac using the EnableFtm method.
Load map for wireless error model with the WirelessFtmErrorModel class.
All this should be done before the simulation begins.

2nd:
Use the Regular Wifi Mac to create a new ftm session via the NewFtmSession method. Specify the address of the FTM Responder.
This method should return a new FtmSession pointer to the newly created session.
If this is 0 one of the following cases is true:
-FTM is not enabled
-The partner address is invalid
-A session with the specified partner already exists
-The FTM partner has blocked new sessions for some amount of time

3rd:
With the session object you can create a new FtmParams object to specify the parameters to be used and set it with the SetFtmParams method.
The error model should also be created and set at this point. This can be done with the SetFtmErrorModel method.
The SessionOverCallback should also be set so the RTT can be retrieved after the FTM session finished.
When the setup of the session if finished, you can call the SessionBegin method to start the FTM session.

4th:
When the session if over the Callback you specified earlier will be called with the FtmSession as parameter. This way you can access the average RTT and also each individual RTT
with the methods:
-GetAvgRTT (), which return an int64_t
-GetIndividualRTT (), which returns a std::list<int64_t>

After the simulation is completed, the trace files can also be found in the root directory
