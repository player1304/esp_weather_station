import csv
from flask import Flask, request, render_template
from datetime import datetime
from markupsafe import escape

app = Flask(__name__)

@app.route('/update_weather')
def update_weather():
    # test: update_weather?time=000&temp=10&air_humidity=20.0&air_pressure=1000.0&soil_humidity=30.0&rain=false&light_level=50&other_info=none
    time = escape(request.args.get('time'))
    temp = escape(request.args.get('temp'))
    air_humidity = escape(request.args.get('air_humidity'))
    air_pressure = escape(request.args.get('air_pressure'))
    soil_humidity = escape(request.args.get('soil_humidity'))
    rain = escape(request.args.get('rain'))
    light_level = escape(request.args.get('light_level'))
    other_info = escape(request.args.get('other_info'))


    with open('weather_data.csv', 'a', newline='') as file:
        writer = csv.writer(file)
        if file.tell() == 0:  # Check if file is empty
            writer.writerow(['time', 'temp', 'air_humidity', 'air_pressure','soil_humidity', 'rain', 'light_level', 'other_info'])  # Write header if file is empty
        writer.writerow([time, temp, air_humidity, air_pressure, soil_humidity, rain, light_level, other_info])

    return "0"


@app.route("/")
def show_weather():
    weather_data = []
    
    try:
        with open('weather_data.csv', 'r') as file:
            reader = csv.reader(file)
            header = next(reader)  # Skip header row
            rows = list(reader)
            last_30_rows = rows[-30:]  # Select only the last 30 rows
            for row in last_30_rows:
                # Convert the Unix epoch time to the desired format
                epoch_time = int(row[0])
                formatted_time = datetime.fromtimestamp(epoch_time).strftime("%Y-%m-%d %H:%M:%S")
                row[0] = formatted_time
                weather_data.append(row)
        return render_template('weather.html', data=weather_data)
    except FileNotFoundError:
        return "File not found"
    except:
        return "Error"

if __name__ == '__main__':
    app.run(debug=True)