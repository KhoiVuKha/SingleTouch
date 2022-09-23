# SingleTouch
- This project using Capacitive Sensor buttons to capture human touch (ADC values).
- These ADC values are read by some chip's ADC Pins.
- Each Sensor button is configured as a keyboard's key.
- STM32C8T6 processes touch evens and send it to a Computer through USB connection.
- So our product now acts as an USB Device (In another words It is 'a simple keyboard USB version', each sensor button is a key button).
- With this product, we can play many basic games like: Piano (8 keys), Chicken invaders, Pacman...
Some Images:

# Schematic & PCB:
## Schematic
![image](https://user-images.githubusercontent.com/15206083/191793858-991b6fc5-03c2-4e1b-8dec-3eb722ad50eb.png)

## PCB
Top Layer | Bottom Layer
--|--
![image](https://user-images.githubusercontent.com/15206083/191795135-0056b6c9-4115-4904-a94d-adacafb0a85c.png) | ![image](https://user-images.githubusercontent.com/15206083/191795251-68b556c2-ee14-4841-9566-f089307ac6b0.png)

# Product's real Image
- Product is simply as a PCB board with main chip (STM32C8T6 - with intergrated USB interface) and some holes (Capacitive Sensor Buttons)

| Green PCB (v1.0)  | Red PCB (V1.1) |
| ------------- | ------------- |
| ![image](https://user-images.githubusercontent.com/15206083/191791381-8f3cd388-ae28-434d-8c84-33452aff3ef8.png) | ![b635873c06c9c2979bd8](https://user-images.githubusercontent.com/15206083/191878442-0774e454-0d13-4ca8-b609-1e212e33e420.jpg) |

- Game Pad

![TAY_CAM](https://user-images.githubusercontent.com/15206083/191878358-3bdd1f02-b2be-4ece-9ec5-f03351eaf6da.png)


- Connected with wires connected to objects

| #  | # |
| ------------- | ------------- |
| ![image](https://user-images.githubusercontent.com/15206083/191791429-287e9807-fd85-4b9f-a54e-7f58e9faf08d.png)  | ![image](https://user-images.githubusercontent.com/15206083/191792009-551b168a-3017-4578-824f-357b26c0fc8c.png)  |
| <p align="center"> <img src="https://user-images.githubusercontent.com/15206083/191795613-f763d5ec-8d02-42cf-a596-9d6437c14243.png"> </p>  | <p align="center"> <img src="https://user-images.githubusercontent.com/15206083/191795675-1882faab-9d1d-4a3a-bbf9-f945da65c3e4.png"> </p>  |
| <p align="center"> <img src="https://user-images.githubusercontent.com/15206083/191795715-903ab9e5-3868-4691-98a2-b311f222874f.png"> </p>  | <p align="center"> <img src="https://user-images.githubusercontent.com/15206083/191795764-af761b14-1af6-4c64-93b3-b19d71477abe.png"> </p>  |

# Techniques / Technologies
- Capacitive Sensor Button

  A capacitive sensor is an electronic device that can detect solid or liquid targets without physical contact. To detect these targets, capacitive sensors emit an electrical field from the sensing end of the sensor. Any target that can disrupt this electrical field can be detected by a capacitive sensor

<p align="center">
  <img src="https://user-images.githubusercontent.com/15206083/191790204-d4eeb30f-324d-4572-8a2d-8b44d85a0778.png">
</p>

- Capacitive sensing

  In electrical engineering, capacitive sensing is a technology, based on capacitive coupling, that can detect and measure anything that is conductive or has a dielectric different from air. Refer to [Wiki](https://en.wikipedia.org/wiki/Capacitive_sensing).

- Hardware design.
- C Programming Language.
- USB Device programming.
- Embedded C & STM32 MicroController: ADC, UART, USB, GPIO...
- Probability statistics theory.
