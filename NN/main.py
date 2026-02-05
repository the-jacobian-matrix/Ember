#doing my bio homework

import matplotlib.pyplot as plt

# Data
temperature = [20, 30, 40, 50]
rate = [0.22, 0.29, 0.32, 0.0]

# Plot
plt.plot(temperature, rate, marker='o')
plt.title("Enzyme Activity vs Temperature")
plt.xlabel("Temperature (°C)")
plt.ylabel("Rate (min⁻¹)")
plt.grid(True)
plt.show()
