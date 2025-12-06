# Depth of Closure (DoC) Calculator & Sensitivity Analysis
### Sediment Transport Limits for Coastal Engineering

## 1. Abstract

This repository contains computational tools designed to calculate the **Depth of Closure (DoC)**—the theoretical depth at which sediment exchange between the nearshore and offshore becomes negligible. The software implements the most widely accepted empirical formulations from coastal engineering literature: **Birkemeier (1985)**, **Hallermeier (1981/1983)**, and **Houston (1995)**.

A distinguishing feature of this tool is its built-in **Sensitivity Analysis**. Coastal data is often subject to uncertainty; therefore, this calculator not only computes the DoC for the base case but automatically simulates five additional scenarios (Low/High Energy, Short/Long Period, and Worst Case). This allows engineers to understand the variability of the closure depth relative to wave climate fluctuations.

The tools provide specific recommendations for three distinct engineering applications:
1.  **Beach Nourishment** (Active Profile Toe).
2.  **Hard Structures** (Scour Protection foundations).
3.  **Dredged Material Disposal** (Stable Mound placement).

---

## 2. Theoretical Framework & Methodology

The software derives statistical wave parameters from annual climate data and applies them to empirical depth limits.

### 2.1 Wave Statistics Derivation

The inputs require the Annual Mean Significant Wave Height ($H_{s,mean}$) and the Annual Maximum Significant Wave Height ($H_{s,max}$). The software assumes a statistical distribution to derive the **Effective Wave Height ($H_e$)**, which represents the extreme tail of the distribution (approx. 0.137% exceedance) required for Inner DoC calculations.

**Standard Deviation Estimation ($\sigma$):**
$$\sigma \approx \frac{H_{s,max} - H_{s,mean}}{5.6}$$

**Effective Wave Height ($H_e$):**
$$H_e = H_{s,mean} + 5.6 \sigma$$

**Median Wave Height ($H_m$):**
Used for Outer DoC calculations:
$$H_m = H_{s,mean} - 0.3 \sigma$$

### 2.2 Empirical Depth Models

The calculator implements four distinct formulations to determine the depth of closure ($h_c$).

**1. Birkemeier (1985) - Field Adjusted Inner Limit:**
A modification of the Hallermeier equation based on extensive field measurements at the CERC Field Research Facility. It generally predicts a shallower depth, optimizing sand volume for nourishment.
$$h_c = 1.75 H_e - 57.9 \left( \frac{H_e^2}{g T_e^2} \right)$$

**2. Hallermeier (1981) - Analytical Inner Limit:**
Based on the critical Froude number for intense bed agitation. This provides a conservative (deeper) limit, recommended for structural foundations to prevent scour.
$$h_c = 2.28 H_e - 68.5 \left( \frac{H_e^2}{g T_e^2} \right)$$

**3. Houston (1995) - Simplified Parameterization:**
A rapid estimation based solely on the mean wave height.
$$h_c = 6.75 H_{s,mean}$$

**4. Hallermeier (1983) - Outer Limit (Sediment Entrainment):**
Defines the seaward limit of the "Shoal Zone" where typical waves can no longer agitate the specific sediment size ($D_{50}$). Used for siting dredged material disposal mounds.
$$h_c = 0.018 H_m T_e \sqrt{\frac{g}{(s-1) D_{50}}}$$
*Where $s$ is the specific gravity of the sediment (typically 2.65).*

---

## 3. Repository Contents & Compilation

This repository includes three implementation variations to suit different engineering workflows.

### 3.1 Python Script (`depth_of_closure.py`)
A rapid prototyping script ideal for quick checks, educational purposes, or integration into Jupyter notebooks.
* **Dependencies:** Python 3.x (Standard libraries: `math`, `sys`, `argparse`).
* **Execution:**
    ```bash
    python3 depth_of_closure.py
    ```

### 3.2 C++ CLI (`depth_of_closure_cli.cpp`)
A high-performance command-line tool designed for batch processing. It performs the full sensitivity analysis and exports a formatted text report.

* **Compilation (GCC/MinGW):**
    ```bash
    g++ -O3 -march=native -std=c++17 -Wall -Wextra \
    -static -static-libgcc -static-libstdc++ \
    -o depth_of_closure_cli depth_of_closure_cli.cpp
    ```
* **Usage (Interactive):** `./depth_of_closure_cli`
* **Usage (Arguments):**
    ```bash
    ./depth_of_closure_cli [Hs_Mean] [Te] [Hs_Max] [D50_mm] [SG]
    # Example:
    ./depth_of_closure_cli 2.25 9.0 7.7 0.50 2.65
    ```

### 3.3 C++ GUI (`depth_of_closure_gui.cpp`)
A standalone native Windows application using the Win32 API. It provides a visual interface for inputting wave parameters and viewing the generated "Design Decision Matrix."

* **Compilation (MinGW on Windows):**
    ```bash
    g++ -O3 -march=native -std=c++17 -municode \
    depth_of_closure_gui.cpp -o depth_of_closure_gui \
    -mwindows -static -static-libgcc -static-libstdc++
    ```
* **Features:**
    * Form-based inputs with validation.
    * Real-time generation of the Technical Report.
    * Automatic file export to `output.txt`.

---

## 4. Input Parameter Definitions

| Parameter | Symbol | Unit | Description |
| :--- | :---: | :---: | :--- |
| **Annual Mean Hs** | $H_{s,mean}$ | meters | The yearly average significant wave height. |
| **Mean Period** | $T_e$ | seconds | The mean wave period associated with the wave climate. |
| **Annual Max Hs** | $H_{s,max}$ | meters | The maximum significant wave height recorded in a typical year (proxy for the extreme tail). |
| **Sediment Size** | $D_{50}$ | mm | The median grain diameter of the native beach sediment. |
| **Specific Gravity** | $s$ | - | Relative density of sediment (Default Quartz = 2.65). |

---

## 5. Output Report Explanation

The software generates a detailed technical report. Key sections include:

1.  **Executive Summary:** Summarizes the input site conditions and calculated statistical parameters ($H_e, H_m, \sigma$).
2.  **Design Decision Matrix:** Recommends specific depth values based on the intended application:
    * **[A] Beach Nourishment:** Uses Birkemeier (Shallower).
    * **[B] Hard Structures:** Uses Hallermeier Inner (Deeper/Safer).
    * **[C] Disposal:** Uses Hallermeier Outer (Deepest/Most Stable).
3.  **Sensitivity Analysis Table:** A matrix displaying results for different wave scenarios.

---

## 6. Bibliographic References

1.  **Birkemeier, W.A. (1985).** "Field data on Seaward Limit of Profile Change." *Journal of Waterway, Port, Coastal and Ocean Engineering*, 111(3), 598-602.
2.  **Hallermeier, R.J. (1981).** "A profile zonation for seasonal sand beaches from wave climate." *Coastal Engineering*, 4, 253-277.
3.  **Hallermeier, R.J. (1983).** "Sand transport limits in coastal structure design." *Proceedings of Coastal Structures '83*, ASCE.
4.  **Houston, J.R. (1995).** "Beach Nourishment." *Coastal Engineering Technical Note*, CETN-II-32, USACE.
5.  **USACE (2002).** *Coastal Engineering Manual (CEM)*, Part III.

---

## 7. License & Disclaimer

**License:** Open Source (MIT or equivalent).


**Disclaimer:** This software is an engineering aid and does not replace detailed coastal process modeling or site-specific field studies. The authors assume no liability for design failures resulting from the use of these tools.
