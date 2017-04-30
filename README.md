# 8x8 Dot Matrix Word Clock
This is an AVR based mini word clock featuring:
* Real Time Clock (DS3231)
* LiPo battery with integrated USB charging circuit
* low battery sensing

The unit is composed of two stacked PCB and one 8x8 dot-matrix display module.

The Hardware folder is composed of Autodesk Eagle CAD files (schematics and PCB) and of Bill Of Material.
The Software folder is composed of Atmel Studio 7.0 project files, including libraries and hex file.

The I2C-master-lib is from user g4lvanix on Github ([github.com/g4lvanix/I2C-master-lib](http://github.com/g4lvanix/I2C-master-lib)). However, he does not include any licence in his repository therefore all rights for this library belongs to him.