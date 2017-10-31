# IIP-ATV-Light

Here are the Arduino sketch and other files to get working with the IIP ATV Light app.

----------------------------------------------------------------------------------------------------------------------------

It uses this "fake" HM-10 module https://www.ebay.com/itm/HM-10-CC2540-CC2541-BLE-Bluetooth-4-0-Serial-Wireless-Module-for-Arduino-iBeacon/162508058657?epid=847416381&hash=item25d63c3021:g:zsYAAOSwxu5ZECqF
If a real one is used the sketch needs modifying.

The baud should be set to 19200 "AT+BAUD5"
This can be done by using a FTDI and the onbord DEBUG header.
To direct RX and TX of tje HM-10 to the DEBUG header, fit 4 jumpers on the HM-10 and TTL SELECT header, all 4 to the right.
The board is then powered by the FTDI.
DON'T CONNECT POWER TO THE BOARD WHILE A FTDI IS CONNECTED IF THE FTDI IS SET TO 3.3V!!!

A list of AT commands for the fake HM-10 can be downloaded here http://img.banggood.com/file/products/20150104013200BLE-CC41-A_AT%20Command.pdf

----------------------------------------------------------------------------------------------------------------------------

For the power regulator, this fits the board design https://www.ebay.com/itm/1PCS-Mini-3A-DC-DC-Converter-Step-Down-Module-Adjustable-3V-5V-16V-For-RC-T/301924577805?hash=item464c1b720d:g:td0AAOSwKIpV~iDx

Remember to adjust the voltage to 5V before installing on the board!
The board will tolerate 5-27V.

----------------------------------------------------------------------------------------------------------------------------

Before you power up the board you need to fit 5 Jumpers
J3, J6, J7 and two on HM10 SELECT - NANO (both on the left side)
First time you power on the board after uploading the sketch, pull the "Factory default" (A5) pin high while powering on.
This will update the EEPROM with all the settings at it will also name the HM-10(fake) module to "IIP ATV Light 2.0" so the APP can find it.
After this disconnect and reconnect power.


