Watering station 1.0
====================

Description:
------------

This piece of software is used to keep the pots wet in garden.

How to connect:
---------------
* Moisture sensor 1 - GPIO 33
* Moisture sensor 2 - GPIO 34
* Moisture sensor 3 - GPIO 35
* Relay 1 (electro valve) - GPIO 25
* Relay 2 (electro valve) - GPIO 26
* Relay 3 (electro valve) - GPIO 27
* Pump relay - GPIO 14
* Water level sensor - GPIO 13
* Lack of water LED - GPIO 23 (remember about current limitation (may be 3k3 resistor)

Project assumptions:
--------------------

* The water is stored in the barel and it must be pumped into the pots using a pump.
* The water level should be controled to avoid running the pump without water.
* The soil moisture should be conteolled using the ADC in the microcontroller - captive sensors should be used because they are more durable.
* Every pot shoud be wateret separately - because they can has different size - to avoid overwatering.
* When tank is empty and it should be refiled - there is notification by red led.
* The ESP32 microcontroler will be used, because it's cheap and in the future it can assure connectivity with network to record statistics, water confumption by pot etc.

Used things:
------------

* Captive soil moisture sensors,
* electromagnetic valves,
* 12V water pump (may be used the heapest 12V car washers),
* Magnetic (contactron) water level sensor.
* ESP32 as controller.

Version revision
----------------
29.07.2019 - Project start, create readme.
04.08.2019 - Refactoring and additional descritpion.
