# Ollerus

A Tool for Monitoring & Control of Network Interfaces.

Makes use of "nl80211 / cfg80211"

Some mentionable Tools:
- WLAN Channel Monitoring
- (Netto) Bandwidth measurement
- Seamless Channel Switch
- Monitoring and Setting of Wireless Transmission parameters
- Dynamic Adaption of Wireless Transmission Power two set constraints


Commandline prompts:

First passed parameter has to be the interfacename (e.g. wlan0 or eth0)
-> dev [interfacename]
If you do not pass an interfacename, the tool works on default with
-> dev wlan0
Typing cmds directly after the dev [interface] argument prints you out the nl80211 commands which this interface supports. (This argument can be missed out...)

If you pass the dev, then the second parameter has to determine if to do a single Run or to run the program in a loop.
-> do [time-interval in ms]
Without the dev passed, the do [time] has to be the first parameter.
With [time-interval in ms] you give the pause-interval in milli seconds.
Completely without the do argument the ollerus does just a single Run-through.


Some other Arguments can be used together and in random order. The order in which you pass the arguments determines the order in which the output on console/in file ensues.
Compatible Arguments:
link – scan
set (use it only alone)
help (use it only alone)



Implemented Arguments:

link  - Ascertains certain Info about the established connection

scan  - Scan without further parameters scans for available WLAN-nets and prints them with their beacon-delivered info

db  - link & scan can have additional db arguments for database access

db send  - sends the data, ascertained from done link or scan, to the database

db get  - requests Data from the database

scan search [ssid]  - With this you can forward a specific ssid, to search for.
If one or more WLANs with this ssid are in range it prints them.

set  - Used to set settings, specified with further arguments

set txpower <auto | fixed | limited> [value in mBm]  - set the transmission power of your interface device

help  - Some helpful things like informational prints

endian  - Prints the System Endianess.

help print <iftype | cmdatts>  - iftype: Prints the Device Infrastructure Types
cmdatts: Prints the Attributes which are delivered after a request with the specified nl80211-command. For now the cmd has to specified directly inside the source-code.


son  - the pretty functions about the Self Organizing Networks

son adapttxpower  - run Ollerus seperately in this mode on Access Point and the stations and the Transmission Power of the clients gets adapted to minimal needed watts.

son optimizechannel  - run on Access Point and stations. The Access Point organizes the WLAN-Channels used to communicate with each client. Determines over Noise which channels best to be used.


Functions to check the Network-Performance
- debug
-> monitorbw  - Monitors the Bandwith of a connection. Shows current (Netto) Datarate and a Moving Average.
-> contcheck  - Checks the Continuity of a connection. Whines if it get’s intermitted.
-> monitorbwcont  - And now guess, what this could be…
Same Syntax for following Options for all Monitoring-Commandos:
	<Monitor-Cmd> [server | client] -||<Destination>||
	
schedule  - Time-Schedules the given Task.
<Date> <Time> <Task> ||<Further_Options>||
Date and Time Format: <YYYY-MM-DD> <HH:MM:SS>
e.g.: ollerus debug schedule 2017-03-25 00:45:57 monitorbw [Options_for_monitorbw]
Please regard, that it may not be started exactly at the given Second. Guaranteed can only be an accuracy of few seconds. But this depends on your system. Mostly Ollerus will hit exactly the second +/- few Milli-Seconds.
