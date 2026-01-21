## NIST Software Licensing Statement

NIST-developed software is provided by NIST as a public service.
You may use, copy, and distribute copies of the software in any
medium, provided that you keep intact this entire notice. You may
improve, modify, and create derivative works of the software or
any portion of the software, and you may copy and distribute such
modifications or works. Modified works should carry a notice
stating that you changed the software and should note the date
and nature of any such change. Please explicitly acknowledge the
National Institute of Standards and Technology as the source of
the software.

NIST-developed software is expressly provided "AS IS." NIST MAKES
NO WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT, OR ARISING BY
OPERATION OF LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
NON-INFRINGEMENT, AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR
WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED
OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST DOES
NOT WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE
SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE
CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE
SOFTWARE.

You are solely responsible for determining the appropriateness of
using and distributing the software and you assume all risks
associated with its use, including but not limited to the risks
and costs of program errors, compliance with applicable laws,
damage to or loss of data, programs or equipment, and the
unavailability or interruption of operation. This software is not
intended to be used in any situation where a failure could cause
risk of injury or damage to property. The software developed by
NIST employees is not subject to copyright protection within the
United States.

## Disclaimers

**Disclaimer of Non-endorsement**:
Any references to commercial entities, products, services, or other non-governmental
organizations or individuals in this repository are provided solely for the information of
individuals using this document. These references are not intended to reflect the opinion of
NIST, the Department of Commerce or the United States, or its officers or employees. Such
references are not an official or personal endorsement of any product, person, or service, nor
are they intended to imply that the entities, materials, or equipment are necessarily the best
available for the purpose. Such references may not be quoted or reproduced for the purpose of
stating or implying an endorsement, recommendation, or approval of any product, person, or
service. 

**NIST Created/Developed Components**
- 3D Print Models for FoxNode - NIST License (See top section above)
- PCB schematics - NIST License (See top section above)

## Other software licenses used in this project
**Adafruit Libraries**
Adafruit_BusIO
License: MIT
Notes: Core I2C/SPI abstraction layer used by many Adafruit drivers.

Adafruit_GFX_Library
License: BSD 3-Clause
Notes: One of the few Adafruit libs that is BSD rather than MIT.

Adafruit_MAX1704X
License: MIT
Notes: Fuel gauge driver (MAX17043/44/48).

Adafruit_seesaw_Library
License: MIT
Notes: Used for Seesaw-based peripherals.

Adafruit_ST7735_and_ST7789_Library
License: MIT
Notes: Display driver for ST77xx LCDs.

Adafruit_Unified_Sensor
License: MIT
Notes: Sensor abstraction framework used across Adafruit sensor drivers.

**Arduino / Core / Networking**
ArduinoJson
License: MIT
Notes: Widely used JSON serialization library.

NTPClient
License: MIT
Notes: Common Arduino NTP client by Fabrice Weinberg.

SD
License: LGPL-2.1
Notes: Arduino core SD library, wraps SdFat functionality.
Important: LGPL has redistribution implications if statically linked.

**Sensor / Vendor Libraries**
BitBang_I2C
License: GPL-2.0 (typical)
Notes: Often derived from early AVR bit-banged I2C implementations.
Verify: License headers vary by fork.

bb_rtc
License: MIT
Notes: Frequently used lightweight RTC helper library.
Verify: Some variants are public domain or BSD.

GuL_T1_Humidity_HDC10XX
License: MIT
Notes: Community driver for TI HDC1000/1080/1010 sensors.

KXTJ3-1057
License: BSD 3-Clause or MIT
Notes: Accelerometer driver originally from Silicon Labs or SparkFun lineage.
Verify: License header inside .cpp/.h.

**STMicroelectronics (Cube / X-CUBE style)**
ClosedCube_LPS25HB
License: BSD 3-Clause
Notes: Pressure sensor driver based on ST application code.

ClosedCube_OPT3001
License: BSD 3-Clause
Notes: OPT3001 light sensor driver.

**PiSugar**
PiSugar Power Manager (daemon)
PiSugar Web UI
PiSugar Python Library
License: MIT

PiSugar 2 3D Printer Case
Licnese: GNU Version 3
