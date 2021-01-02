# marax_monitor
Shows temperature data and pump activity of Lelit's MaraX (PL62X) espresso machine with an ESP8266 on a 3.2" Nextion display

## Introduction
[Lelit's MaraX (PL62X)](https://marax.lelit.com/) espresso machine is a quite interesting device because of it's "coffee mode", i.e. 
temperature is regulated by measuring the HX temperature instead of the steam temperature. The control unit of the machine provides 
a serial interface which allows us to read all relevant data.

There are already many projects out there which read and display the machine's data, but I want to draw your attention to following 
two projects from where I got a lot of ideas and information (thank you!):

* [calin's excellent post on reddit](https://www.reddit.com/r/espresso/comments/hft5zv/data_visualisation_lelit_marax_mod/)
* [alexrus' great marax_timer project](https://github.com/alexrus/marax_timer)

Since a Nextion display was laying around in my lab, I created marax_monitor which does following:

* display and plot temperatures on a Nextion display
![Display](/misc/Display_explained.png)
* shot timer by attaching a reed switch to the vibration pump
![Shot timer](/misc/Display_shot_timer.jpg)
* send data via MQTT to Node-RED and display data on it's dashboard 
![Node-RED dashboard](/misc/Node-RED Dashboard.png)

## BOM

**Note:** I just used components which were laying around in my lab

Required:
* ESP8266 development board
* Nextion Display, e.g. [Nextion NX4024T032](https://www.itead.cc/nextion-nx4024t032.html)
* Reed switch, e.g. [MC-38](https://www.aliexpress.com/item/32255861885.html)

Optional (if battery powered):
* Boost converter, e.g. WEMOS battery shield
* Battery, e.g. LiPo 18650

Optional (if auto power off functionality):
* PNP and NPN mosfets, resistors, diode

or 

* Soft power latch module, e.g. [Polulu Item #2808](https://www.pololu.com/product/2808)

Case:
* [3.2" display bezel by Nextion](https://nextion.tech/nextion-editor/#_section4)
* [Case](/hardware/MaraX_monitor_case.stl)

## Wiring

### MaraX

**Note:** You don't have to remove the complete housing of your espresso machine. Control unit and vibration pump can be easily 
reached by only removing the maintenance hatch on the bottom.

* The two wires of the serial connection (D5 and D6) has to be connected to the RX and TX pins of the MaraX control unit (third and 
fourth pin counted if housing wall is on the left. For details please have a look on [calin's excellent post on reddit](https://www.reddit.com/r/espresso/comments/hft5zv/data_visualisation_lelit_marax_mod/)

* Reed switch has to be attached to the vibration pump (I used double sided tape). For details please have a look at [alexrus' great marax_timer project](https://github.com/alexrus/marax_timer)

### Monitor

![ElectronicsGitHub Logo](/hardware/marax_monitor_circuit_drawing.png)


## Software
