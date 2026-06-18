# ADC Linearity & Voltage Divider Design Guide

This guide explains the physical ADC non-linearity limitations of the CH32V003 microcontroller and provides practical engineering guidelines for designing external voltage divider circuits to achieve the highest possible measurement accuracy.

---

## 1. Core ADC Limitations of the CH32V003

The CH32V003 features a **10-bit Successive Approximation Register (SAR) ADC** that uses the system supply voltage ($V_{DD}$) as its reference voltage (since it lacks an external $V_{REF}$ pin). Like all SAR ADCs in budget microcontrollers, it exhibits non-linear behavior at the extreme ends of its input range:

### A. Ground-Level Dead Zone (0V to ~300mV)
At the bottom of the scale, internal analog switches and transistor threshold offsets prevent the ADC comparator from responding linearly. 
* Input voltages below approximately $150\text{mV} - 300\text{mV}$ may read as absolute zero or fluctuate unpredictably.

### B. Saturation Near VDD (VDD - 300mV to VDD)
As the analog input voltage approaches the $V_{DD}$ rail (the reference voltage), the internal sample-and-hold comparator's input stage saturates. 
* Readings close to the supply rail compress and flatline at `1023` early, even before the physical voltage reaches $V_{DD}$.

### C. Switched-Capacitor Loading (Impedance Error)
The ADC's input stage consists of a small sample-and-hold capacitor ($\approx 8\text{ pF}$). When a conversion starts, this capacitor connects to the input pin for a brief sampling window.
* If the source impedance (e.g., from a voltage divider) is too high, the internal capacitor cannot fully charge during the sampling window, resulting in readings that curve downwards at higher voltages.

---

## 2. Voltage Divider Design Guidelines

To achieve maximum linearity and accuracy when measuring external voltages, you must scale the input voltages to avoid the "dead zones" and ensure the source impedance allows the ADC's sampling capacitor to charge fully.

```
       V_DD |=========================================== (VDD Saturation Knee)
            |      \
            |       \   Optimum Operating Range (10% - 90%)
            |        \  Linear & Drift-Corrected
            |         \
            |          \
        GND |=========================================== (0V Non-linear Dead Zone)
```

### Guideline A: Keep Inputs Within the Sweet Spot (10% to 90%)
Design your voltage divider ratio so that the target measurement voltages map to the middle **10% to 90%** of the $V_{DD}$ range (or **15% to 85%** for maximum precision).

| System $V_{DD}$ | Target Optimum Range (10% to 90%) | Preferred Linearity Range (15% to 85%) |
| :--- | :--- | :--- |
| **3.3 V** | $0.3\text{ V}$ to $3.0\text{ V}$ | $0.5\text{ V}$ to $2.8\text{ V}$ |
| **5.0 V** | $0.5\text{ V}$ to $4.5\text{ V}$ | $0.75\text{ V}$ to $4.25\text{ V}$ |

### Guideline B: Keep Source Impedance Below $10\text{ k}\Omega$
The equivalent output resistance ($R_{eq}$) of the voltage divider connected to the ADC pin is:
$$R_{eq} = R_1 \parallel R_2 = \frac{R_1 \cdot R_2}{R_1 + R_2}$$

* **For Low-Impedance Dividers:** Keep $R_{eq} < 10\text{ k}\Omega$ (ideally $< 5\text{ k}\Omega$).
* **For High-Impedance Dividers (Low Power):** If you must use high-resistance dividers to prevent power loss, you **must** place a **$10\text{ nF}$ to $100\text{ nF}$ ceramic capacitor** parallel to $R_2$ close to the MCU pin. The external capacitor acts as a charge reservoir to instantly charge the internal $8\text{ pF}$ sampling capacitor when the ADC sampling switch closes.

---

## 3. Step-by-Step Design Example

### Scenario
We want to measure a $12\text{V}$ battery (which ranges from $10.0\text{V}$ discharged to $14.4\text{V}$ during charging) using a CH32V003 running at $3.3\text{V}$.

### Step 1: Scale the Max Voltage to the sweet spot
Our maximum voltage is $14.4\text{V}$. We want this to correspond to $2.8\text{V}$ on the ADC pin (to stay below the $3.0\text{V}$ non-linear knee).
$$\text{Division Ratio} = \frac{2.8\text{ V}}{14.4\text{ V}} \approx 0.1944$$

### Step 2: Choose standard resistor values
Let's select standard resistor values:
* **$R_1 = 39\text{ k}\Omega$**
* **$R_2 = 10\text{ k}\Omega$**

$$\text{Actual Division Ratio} = \frac{10\text{ k}\Omega}{39\text{ k}\Omega + 10\text{ k}\Omega} = 0.2041$$
* **Max reading (14.4V):** $14.4\text{ V} \times 0.2041 = 2.94\text{ V}$ (safely within the $< 3.0\text{V}$ linear window).
* **Min reading (10.0V):** $10.0\text{ V} \times 0.2041 = 2.04\text{ V}$ (comfortably in the middle of the range).

### Step 3: Verify Source Impedance
Calculate $R_{eq}$:
$$R_{eq} = \frac{39\text{ k}\Omega \times 10\text{ k}\Omega}{39\text{ k}\Omega + 10\text{ k}\Omega} \approx 7.96\text{ k}\Omega$$

Because $7.96\text{ k}\Omega < 10\text{ k}\Omega$, the source impedance is low enough to satisfy the ADC sampling requirements directly. If you scaled this up by a factor of 10 ($R_1 = 390\text{ k}\Omega$, $R_2 = 100\text{ k}\Omega$) to save power, you would add a $10\text{ nF}$ filter capacitor across $R_2$.

---

## 4. Built-in Firmware Corrections

The firmware on the co-processor automatically applies three corrections to maximize accuracy:
1. **Dynamic VDD Tracking:** The firmware continuously reads the internal $1.2\text{V}$ reference channel (see [main.c:L113-L123](../src/main.c#L113-L123)) to compute the actual value of $V_{DD}$. This corrects all calculations against power supply drift.
2. **Startup Self-Calibration:** An ADC self-calibration cycle is executed on boot to eliminate internal comparator offsets (see [main.c:L268-L277](../src/main.c#L268-L277)).
3. **EMA Noise Filter:** The Exponential Moving Average digital filter smoothens thermal and high-frequency electrical noise (see [main.c:L125-L139](../src/main.c#L125-L139)).

