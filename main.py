from flask import Flask, request, jsonify
import json
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

def save_data(data):
    with open('data.json', 'w') as file:
        json.dump(data, file)

def load_data():
    try:
        with open('data.json', 'r') as file:
            data = json.load(file)
        return data
    except Exception as e:
        raise Exception("Open Json error")

@app.route("/update-sensor", methods=["GET"])
def update_sensor():
    temperature = float(request.args.get('temperature'))
    humidity = float(request.args.get('humidity'))
    light = float(request.args.get('light'))
    rain = float(request.args.get('rain'))
    is_hanging = int(request.args.get('is_hanging'))

    data = load_data()
    data["temperature"] = temperature
    data["humidity"] = humidity
    data["light"] = light
    data["rain"] = rain
    data["is_hanging"] = is_hanging

    save_data(data)  # Save the data to a JSON file
    return jsonify({"message": "Data received and saved successfully"})

@app.route("/status", methods=["GET"])
def get_status():
    data = load_data()
    return jsonify({
        "is_hanging": data.get("is_hanging"),
        "force_collect": data.get("force_collect"),
        "temperature": data.get("temperature"),
        "humidity": data.get("humidity"),
        "light": data.get("light"),
        "rain": data.get("rain"),
        "is_auto": data.get("is_auto"),
    })

@app.route("/force-collect", methods=["GET"])
def force_collect():
    data = load_data()
    if not data.get("force_collect", 0):
        data["force_collect"] = 1
        data["is_auto"] = 0
        save_data(data)

    return jsonify({"message": ""})

@app.route("/auto-manual", methods=["GET"])
def auto_manual():
    data = load_data()
    if data.get("is_auto") == 0:
        data["is_auto"] = 1
        save_data(data)
    else:
        data["is_auto"] = 0
        save_data(data)

    return jsonify({"message": ""})

@app.route("/update-force-collect", methods=["GET"])
def update_force_collect():
    data = load_data()
    data["force_collect"] = 0
    save_data(data)

    return jsonify({"message": ""})

if __name__ == "__main__":
    data = {
        "temperature": 0,
        "humidity": 0,
        "light": 0,
        "rain": 0,
        "is_hanging" : 0,
        "force_collect" : 0,
        "is_auto" : 0,
    }
    save_data(data)
    app.run(debug=True, host='0.0.0.0', port=8000)
