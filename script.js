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
    statusElement.textContent = `Status : ${data.is_hanging}`;

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

    const temperatureElement = document.getElementById("temperature");
    temperatureElement.textContent = "Loading...";
    temperatureElement.textContent = `${data.temperature}`;

    const humidityElement = document.getElementById("humidity");
    humidityElement.textContent = "Loading...";
    humidityElement.textContent = `${data.humidity}`;

    const lightElement = document.getElementById("light");
    lightElement.textContent = "Loading...";
    lightElement.textContent = `${data.light}`;

    const rainElement = document.getElementById("rain");
    rainElement.textContent = "Loading...";
    rainElement.textContent = `${data.rain}`;
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
