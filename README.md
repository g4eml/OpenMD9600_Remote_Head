# OpenMD9600_Remote_Head

The MD9600 is a good DMR mobile radio but, as standard, it does not have the ability to mount the control head remotely to the main body of the radio. 

The Control Head of the radio is normally connected to the front panel by a 30 conductor Flat Flexible cable. It is impractical to try to extend this cable. 

Now that the radio is able to run the excellent OpenMD9600 firmware it became possible to develop a way of communicating between the radio and the front panel using only a single RJ45 Cat7 cable. This requires a modified version of the OpenMD9600 firmware. 

### <mark>**Note that this modification will only work with the OpenMD9600 firmware. It will not work with the original TYT factory firmware.**</mark>

The mod requres two new small PCBs. 

### Radio Body Interface PCB

This PCB is fitted to the front of the radio body and is connected to the existing 30 way ribbon cable. The necessary signals are routed to a RJ45 socket. A second RJ45 socket is also provided for direct connection of the microphone to the radio body.  Whilst the microphone can still be connected to the remotely mounted head this can suffer from pickup of noise due to the poor screening of the RJ45 cable. It is therefore recommended to connect the microphone directly to the radio body, using an extension cable if necessary. 

The modifed OpenMD9600 firmware detects the presence of the interface board and reconfigures the existing signals into a suitable format. This is a bi-directional 250000 Baud serial link.  Using this link display data is sent to the remote head PCB and front panel key presses are received from it. 

The PCB also include a digital potentiometer chip which is used to control the radio volume.  This chip is controlled by the remote head PCB.  This is necessary to reduce audio noise pickup on the long cable. 

### Remote Head Interface PCB

This PCB is fitted to the rear of the remote head and connects to the existing 30 way connector.  Serial data to and from the radio body is handled by a Raspberry Pi Pico module which decodes the data and sends it to the Front Panel display, it also reads the front panel buttons and sends their staus to the radio body. 

### Instalation

To convert a MD9600 to remote head operation requires the following steps. 

1. Remove front panel from the radio and disconnect the 30 way ribbon cable from the main body. Leave this cable connected to the head.

2. Fit a new 30 way cable between the radio body and the radio body Interface PCB. 

3. Attach the PCB to the front of the radio body using double sided foam tape. 

4. Connect the existing front panel 30 way cable to the remote head interface PCB. 

5. Attach the PCB to the rear of the front panel using double sided foam tape. 

6. Connect the two PCBs togetehr with the RJ45 cable. 

7. Connect the microphone to the second socket on the radio body PCB. 

8. Ensure the MD9600 is running the correct version of OpenMD9600.

9. Power on the radio. It should behave exactly as it did before. 



### How do I make these boards?

All of the information neede to make your own boards is included in this repository. The gerber files for the PCBs are provided and it is now very easy and cheap to get PCBs made in China. Companies such as PCBWAY will accespt the Gerber files as provided and produce 10 PCBs for about £5.  so that is £10 for 10 of both PCBs. I woul dsuggest that you get together with other people in your country and order the boards. 

The components needs to assemble the boards are easily available and a parts list is provided with Farnell, Mouser and Digikey part numbers. 

The only difficult parts to solder are the ribbon cale connectors and the digital potentiometer chip. However these can both be done with very carefull hand soldering. 



### Programming the Raspberry Pi Pico.