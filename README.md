# OpenMD9600_Remote_Head

The MD9600 is a good DMR mobile radio but, as standard, it does not have the ability to mount the control head remotely to the main body of the radio. 

The Control Head of the radio is normally connected to the front panel by a 30 conductor Flat Flexible cable. It is impractical to try to extend this cable. 

Now that the radio is able to run the excellent OpenMD9600 firmware it became possible to develop a way of communicating between the radio and the front panel using only a single RJ45 Cat7 cable. This requires a modified version of the OpenMD9600 firmware. 

### <mark>**Note that this modification will only work with the OpenMD9600 firmware. It will not work with the original TYT factory firmware.**</mark>

The mod requres two new small PCBs. 

### Radio Body Interface PCB

This PCB is fitted to the front of the radio body and is connected to the existing 30 way ribbon cable. The necessary signals are routed to a RJ45 socket. A second RJ45 socket is also provided for direct connection of the microphone to the radio body.  Whilst the microphone can still be connected to the remotely mounted head this can suffer from pickup of noise due to the poor screening of the RJ45 cable. It is therefore recommended to connect the microphone directly to the radio body, using an extension cable if necessary. 

### Remote Head Interface PCB