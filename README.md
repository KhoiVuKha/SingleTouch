# SingleTouch
This project using Capacitive Sensor buttons to capture human touch (ADC values).
These ADC values are read by some chip's ADC Pins.
Each Sensor button is configured as a keyboard's key.
STM32C8T6 processes touch evens and send it to a Computer through USB connection.
So our product now acts as an USB Device (In another words It is 'a simple keyboard USB version', each sensor button is a key button).
With this product, we can play many basic games like: Piano (8 keys), Chicken invaders, Pacman...
Some Images:

# Schematic:
![image](https://user-images.githubusercontent.com/15206083/191793858-991b6fc5-03c2-4e1b-8dec-3eb722ad50eb.png)

# Product's real Image
Product is simply as a PCB board with main chip (STM32C8T6 - with intergrated USB interface) and some holes (Capacitive Sensor Buttons)

![image](https://user-images.githubusercontent.com/15206083/191791381-8f3cd388-ae28-434d-8c84-33452aff3ef8.png)

Connected with some wires to objects

![image](https://user-images.githubusercontent.com/15206083/191791429-287e9807-fd85-4b9f-a54e-7f58e9faf08d.png)

![image](https://user-images.githubusercontent.com/15206083/191792009-551b168a-3017-4578-824f-357b26c0fc8c.png)



# Techniques / Technologies

- Capacitive Sensor Button
A capacitive sensor is an electronic device that can detect solid or liquid targets without physical contact. To detect these targets, capacitive sensors emit an electrical field from the sensing end of the sensor. Any target that can disrupt this electrical field can be detected by a capacitive sensor

![image](https://user-images.githubusercontent.com/15206083/191790204-d4eeb30f-324d-4572-8a2d-8b44d85a0778.png)

*Capacitive sensing*
In electrical engineering, capacitive sensing is a technology, based on capacitive coupling, that can detect and measure anything that is conductive or has a dielectric different from air. [Wiki](https://en.wikipedia.org/wiki/Capacitive_sensing).

- Hardware design.
- C Programming Language.
- USB Device programming.
- Embedded C & STM32 MicroController: ADC, UART, USB, GPIO...
- Probability statistics theory.
