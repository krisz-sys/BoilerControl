How it works?<br><br>
The ESP8266 runs a web server that displays various data.
The microcontroller turns on the boiler if:

The slider is triggered by the user

The time schedule is active

The user can set the time schedule via the web interface.
The microcontroller reads the value of a DHT11 temperature sensor from port D7 (GPIO13), and the boiler command is handled by port D5 (GPIO14).
The temperature reading is for informational purposes only.

You can access the web server by entering the IP address into your browser: http://192.168.1.223/ . <br>
Additionally, you need to configure the WiFi SSID and password.

You can see the circuit schematic below:

<img width="1253" height="697" alt="Screenshot 2026-07-16 235613" src="https://github.com/user-attachments/assets/3b4976e9-78b3-4aa3-b85b-c5ef3a1875ac" />

Here is the PCB layout:

<img width="583" height="695" alt="Screenshot 2026-07-17 001455" src="https://github.com/user-attachments/assets/ed5aec58-a259-44bf-83ae-e122ae7536b5" />
<br><br>

You can find the AutoCAD Eagle design files as HeaterController_PCB.

If the web server is live, you should see the following view. The most important elements are the slider, the temperature value, and the time schedule table:
<br>
<img width="500" height="360" alt="Screenshot 2026-07-17 001816" src="https://github.com/user-attachments/assets/365f372d-a229-4e6f-976f-df566754408b" />
