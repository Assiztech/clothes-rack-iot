// script.js

async function fetchTemperature() {
    const response = await fetch("http://127.0.0.1:8000/status", {
        method: "GET",
    });
    const data = await response.json();
    return data;
}
async function updateTemperature() {
    const data = await fetchTemperature();
    console.log("Fetched data:", data);

    const statusElement = document.getElementById("is_hanging");
    statusElement.textContent = "Loading...";
    if (data.is_hanging == 1) {
        statusElement.style.cssText = `color : rgb(128, 255, 0)`;
        statusElement.textContent = `Status : Hanging`;
    } else {
        statusElement.style.cssText = `color : rgb(255, 255, 0)`;
        statusElement.textContent = `Status : Keeping`;
    }

    const envStatusElement = document.getElementById("env_status");
    envStatusElement.textContent = "Loading...";
    if (data.env_status == 1) {
        envStatusElement.style.cssText = `color : rgb(255, 0, 0)`;
        envStatusElement.textContent = `Weather : Rain`;
    } else if (data.env_status == 2) {
        envStatusElement.style.cssText = `color : rgb(255, 127, 0)`;
        envStatusElement.textContent = `Weather : Might Rain`;
    } else if (data.env_status == 3) {
        envStatusElement.style.cssText = `color : rgb(0, 255, 64) `;
        envStatusElement.textContent = `Weather : Sunny (Hot)`;
    } else {
        envStatusElement.style.cssText = `color : rgb(128, 255, 0)`;
        envStatusElement.textContent = `Weather : Sunny`;
    }

    const actionButton = document.getElementById("actionButton");
    if (data.is_hanging) {
        actionButton.textContent = "Collect Cloth In";
    } else {
        actionButton.textContent = "Hang Cloth Out";
    }

    const autoButton = document.getElementById("autoButton");
    if (data.is_auto) {
        autoButton.textContent = "System: Auto";
    } else {
        autoButton.textContent = "System: Manual";
    }

    let r, g, b;
    const temperatureElement = document.getElementById("temperature");
    temperatureElement.textContent = "Loading...";
    r = 0;
    g = 0;
    b = 30;
    const dataTemp = data.rain;
    r = (dataTemp - 20) * 24;
    g = (34 - dataTemp) * 36;
    if (dataTemp <= 20) {
        r = 0;
        g = 255;
        b = 30;
    }
    temperatureElement.style.cssText = `color : rgb(${r},${g},${b});`;
    temperatureElement.textContent = `${dataTemp}`;

    const humidityElement = document.getElementById("humidity");
    humidityElement.textContent = "Loading...";
    r = 0;
    g = 0;
    b = 30;
    const dataHum = data.humidity;
    r = (dataHum - 30) * 24;
    g = (82 - dataHum) * 36;
    if (dataHum <= 20) {
        r = 0;
        g = 255;
        b = 30;
    }
    humidityElement.style.cssText = `color : rgb(${r},${g},${b});`;
    humidityElement.textContent = `${dataHum}`;

    const lightElement = document.getElementById("light");
    lightElement.textContent = "Loading...";
    r = 0;
    g = 0;
    b = 30;
    const dataLight = data.light;
    g = (dataLight - 700) * 3.6;
    r = (1100 - dataLight) * 2.4;
    if (dataLight <= 200) {
        r = 0;
        g = 255;
        b = 30;
    }
    lightElement.style.cssText = `color : rgb(${r},${g},${b});`;
    lightElement.textContent = `${dataLight}`;

    const rainElement = document.getElementById("rain");
    rainElement.textContent = "Loading...";
    r = 0;
    g = 0;
    b = 30;
    const dataRain = data.rain;
    r = (dataRain - 100) * 0.24;
    g = (1700 - dataRain) * 0.36;
    if (dataRain <= 200) {
        r = 0;
        g = 255;
        b = 30;
    }
    rainElement.style.cssText = `color : rgb(${r},${g},${b});`;
    rainElement.textContent = `${dataRain}`;
}
async function forceCollect() {
    try {
        const response = await fetch("http://127.0.0.1:8000/force-collect", {
            method: "GET",
        });
        if (response.ok) {
            alert("Action executed successfully");
            updateTemperature();
        } else {
            throw new Error("Failed to execute action");
        }
    } catch (error) {
        console.error("Error:", error);
        alert("An error occurred");
    }
}

async function autoManual() {
    try {
        const response = await fetch("http://127.0.0.1:8000/auto-manual", {
            method: "GET",
        });
        if (response.ok) {
            alert("Action executed successfully");
            updateTemperature();
        } else {
            throw new Error("Failed to execute action");
        }
    } catch (error) {
        console.error("Error:", error);
        alert("An error occurred");
    }
}

document.getElementById("actionButton").addEventListener("click", forceCollect);
document.getElementById("autoButton").addEventListener("click", autoManual);
setInterval(updateTemperature, 5000);
// Initial update
updateTemperature();
