import requests
import time

BLYNK_AUTH = "YNQQI_-l3MTdCqHkvU8xNkqwt9CyU-Ao"

# Track previous energy for spike detection
prev_energy = 0

# -------- FUNCTIONS -------- #

def get_data(pin):
    url = f"https://blynk.cloud/external/api/get?token={BLYNK_AUTH}&{pin}"
    try:
        return float(requests.get(url).text)
    except:
        return 0

def send_control(value):
    url = f"https://blynk.cloud/external/api/update?token={BLYNK_AUTH}&V15={value}"
    requests.get(url)

def send_ai_status(msg):
    url = f"https://blynk.cloud/external/api/update?token={BLYNK_AUTH}&V11={msg}"
    requests.get(url)

print("🤖 AI System Started...")

# -------- MAIN LOOP -------- #

while True:
    power = get_data("V2")   # Power (W)
    energy = get_data("V3")  # Energy (kWh)
    demo_mode = int(get_data("V17"))  # 1 = DEMO, 0 = REAL

    print(f"Power: {power} W | Energy: {energy} kWh | Demo Mode: {demo_mode}")

    # -------- MODE-BASED THRESHOLDS -------- #
    if demo_mode == 1:
        energy_limit = 50        # scaled
        spike_limit = 0.5        # scaled spike
        power_limit = 2000
    else:
        energy_limit = 1.5       # real
        spike_limit = 0.05       # real spike
        power_limit = 2000

    # -------- SPIKE DETECTION -------- #
    usage_rate = energy - prev_energy
    prev_energy = energy

    # -------- AI DECISION -------- #
    if usage_rate > spike_limit:
        send_ai_status("AI: Sudden spike detected ⚠")
        send_control(0)

    elif energy > energy_limit:
        send_ai_status("AI: High consumption predicted ⚠")
        send_control(0)

    elif power > power_limit:
        send_ai_status("AI: Heavy load detected ⚡")
        send_control(0)

    else:
        send_ai_status("AI: System efficient ✅")
        send_control(1)

    time.sleep(5)