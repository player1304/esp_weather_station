## A trivial attempt at embedded programming - a weather station?

I stumbled into, and was tempted by, an V2EX thread several weeks ago about embedded programming and the ESP8266 module. This is a trivial attempt at making a weather station.

The project utilizes one ESP8266 devboard and one ESP32 devboard to send the temperature, relative humidity, air pressure, soil moisture, etc. to a home server, which runs a simple Flask app (on Debian (via Gunicorn) to receive GET requests and display the data on demand. 

### What I've gained
- Some experience in:
	- Arduino / embedded programming
	- Soldering (due to two sensors only available in semi-completed form: BMP085 (air pressure) and GY302 (light level))
	- Flask / Python
- A lot of fun

### Known issues
- The solar panel solution doesn't really work as-is, due to the power bank shutting off automatically (presumabaly as power outflow is too small).
- I couldn't get the ESP8266 to properly enter deep sleep and wake up, so it draws full power (albeit not a lot) constantly right now.
- Despite taking precaution to escape the GET requests, I am pretty sure the Flask app is not safe. The server is only accessible via Tailscale externally, so this is not a huge problem.