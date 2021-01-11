# marax_monitor
Shows temperature data and pump activity of Lelit's MaraX (PL62X) espresso machine with an ESP8266 on a 3.2" Nextion display
![marax_monitor](/misc/View_left.jpg)

## Introduction
[Lelit's MaraX (PL62X)](https://marax.lelit.com/) espresso machine is a quite interesting device because of it's "coffee mode", i.e. 
temperature is regulated by measuring the HX temperature instead of the steam temperature. The control unit of the machine provides 
a serial interface which allows us to read all relevant data.

There are already many projects out there which read and display the machine's data, but I want to draw your attention to following 
two projects from where I got a lot of ideas and information (thank you!):

* [calin's excellent post on reddit](https://www.reddit.com/r/espresso/comments/hft5zv/data_visualisation_lelit_marax_mod/)
* [alexrus' great marax_timer project](https://github.com/alexrus/marax_timer)

Since a Nextion display was laying around in my lab, I created marax_monitor which does the following:

* Display and plot temperatures on a Nextion display
![Display](/misc/Display_explained.png)
* Display a shot timer (a reed switch has to be attached to the vibration pump)
![Shot timer](/misc/Display_shot_timer.jpg)
* Send data via MQTT to Node-RED and display data on it's dashboard 
![Node-RED dashboard](/misc/Node-RED_Dashboard.png)

Additionally I've added ArduinoOTA for being able to upload updates over the network.

The device is powered by an 18650 LiPo battery, that's why I've added a battery shield and a simple circuit for auto power off the monitor
after an hour of inactivity for saving energy. The battery lasts for about one week.

We're using marax_monitor for some weeks now and especially the shot timer is a feature which we do not want miss any more. I also like
to watch the temperature profiles which is not so trivial I've thought of before. 

## BOM

**Note:** I just used components which were available in my lab.

Required:
* ESP8266 development board
* Nextion Display, e.g. [Nextion NX4024T032](https://www.itead.cc/nextion-nx4024t032.html)
* Reed switch, e.g. [MC-38](https://www.aliexpress.com/item/32255861885.html)

Optional (for battery powering):
* Boost converter, e.g. WEMOS battery shield
* Battery, e.g. LiPo 18650

Optional (for auto power off functionality):
* PNP and NPN MOSFETs, resistors, diode

or 

* Soft power latch module, e.g. [Polulu Item #2808](https://www.pololu.com/product/2808)

Case (STL files for 3D printer):
* [3.2" display bezel by Nextion](https://nextion.tech/nextion-editor/#_section4)
* [Case](/hardware/marax_monitor_case.stl)

## Wiring

### MaraX

**Note:** You don't have to remove the complete housing of your espresso machine or empty the boiler. Just turn the machine 90 degrees backwards. 
Control unit and vibration pump can be easily reached by only removing the maintenance hatch on the bottom. 

* The two wires of the serial connection (D5 and D6) have to be connected to the RX and TX pins of the MaraX control unit (third and 
fourth pin if housing wall is on the left). For details please have a look on 
[calin's excellent post on reddit](https://www.reddit.com/r/espresso/comments/hft5zv/data_visualisation_lelit_marax_mod/)

* Reed switch has to be attached to the vibration pump (I used double sided tape). For details please have a look at 
[alexrus' great marax_timer project](https://github.com/alexrus/marax_timer)

### Monitor

**Note:** As already mentioned I used components laying around my lab and put them together quick and dirty. 

Device         			        | ESP8266 
--------------------------------| --------
RX pin from MaraX' control unit | D6
TX pin from MaraX' control unit | D5
1st wire from reed switch       | D3
2nd wire from reed switch       | GND
RX pin from Nextion display     | D1
TX pin from Nextion display     | D7

Optional (for auto power off and battery status):

Device         			          | ESP8266 
----------------------------------| --------
Gate of NPN MOSFET (power drive)  | D0
Push button (for power on/off)    | D2
Voltage divider for battery status| A0

The circuit for the auto power off functionality I've taken from [Michael Ruppe](https://github.com/michaelruppe/ArduinoSoftTouchPower) 
(thank you!). 

![Sketch](/hardware/marax_monitor_circuit_board_sketch.png)
![Prototype](/hardware/marax_monitor_circuit_prototype.jpg)
![Circuit board front](/hardware/marax_monitor_circuit_board_front.jpg)
![Circuit board back](/hardware/marax_monitor_circuit_board_back.jpg)
![Assembled](/hardware/marax_monitor_open.jpg)

## Software

### Used libraries for ESP8266 sketch:
* [arduino-timer](https://github.com/contrem/arduino-timer)
* [PubSubClient](https://pubsubclient.knolleary.net/) MQTT client (optional)
* [ArduinoOTA](https://github.com/jandrassy/ArduinoOTA) (optional)

### Nextion display
Project file for Nextion display (edited on Nextion Editor V1.61.2):
* [marax_monitor_nextion.HMI](/hardware/marax_monitor_nextion.HMI)

### Miscellaneous
* MaraX' control unit sends data via serial interface about twice per second. But marax_monitor reads and displays this data only every 5 seconds 
because that's enough 
* If you've implemented auto power off: the device will be powered off after an hour of inactivity. You can manually power off by long 
pressing the power off button. By short pressing the power off button the counter for auto power off will be reset.


