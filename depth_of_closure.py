import numpy as np
import pandas as pd
import sys

"""
================================================================================
                DEPTH OF CLOSURE (DoC) CALCULATOR
================================================================================
DESCRIPTION:
    This script calculates the Depth of Closure (DoC), which is the theoretical 
    depth at which the exchange of sediment between the nearshore and the 
    offshore becomes negligible for engineering purposes.

    It implements multiple theoretical and empirical models to provide a range 
    of estimates (sensitivity analysis), distinguishing between:
      1. INNER Depth of Closure (Active Littoral Zone Limit)
      2. OUTER Depth of Closure (Shoal/Sediment Motion Limit)

THEORETICAL FRAMEWORKS IMPLEMENTED:

    1. Hallermeier (1981) - Analytical Model (Inner DoC):
       - Based on the critical Froude number quantifying intense bed agitation.
       - Defines the seaward limit of the active surf/littoral zone.
       - Primary Input: Effective Wave Height (He) - exceeded 12 hours/year.

    2. Birkemeier (1985) - Field Adjusted Model (Inner DoC):
       - An adjustment to Hallermeier's model based on high-resolution profile 
         data from the USACE Field Research Facility (Duck, NC).
       - Generally predicts shallower depths than Hallermeier, often considered
         more realistic for beach nourishment design.

    3. Hallermeier (1983) - Sediment Entrainment Model (Outer DoC):
       - Defines the seaward limit of the "Shoal Zone".
       - Represents the depth where waves can initiate sediment motion 
         (threshold of motion) but cannot induce significant profile change.
       - Highly dependent on sediment grain size (D50).

    4. Houston (1995) - Simplified Parameterization:
       - Approximates DoC using only the Mean Annual Wave Height (Hs_mean).
       - Useful for preliminary studies where extreme statistics are unavailable.

DATA SOURCES & REFERENCES:
    - Hallermeier, R.J. (1981). "A profile zonation for seasonal sand beaches from wave climate."
    - Birkemeier, W.A. (1985). "Field data on seaward limit of profile change."
    - Houston, J.R. (1995). "Beach-fill volume required to produce specified dry beach width."
    - USACE Coastal Engineering Technical Notes (CETN).

================================================================================
"""

class WaveClimate:
    """
    Handles statistical wave parameterization to derive the
    Effective Wave Height (He) and Median Wave Height (Hm).
    """
    def __init__(self, hs_mean, te_mean, hs_max_annual=None, hs_std=None):
        self.hs_mean = float(hs_mean)
        self.te_mean = float(te_mean)
        self.hs_max = float(hs_max_annual) if hs_max_annual is not None else None
        
        # ----------------------------------------------------------------------
        # 1. ESTIMATION OF STANDARD DEVIATION (SIGMA)
        # ----------------------------------------------------------------------
        # If sigma is not provided, we reverse-engineer it from Hs_max.
        # Assumption: Hs_max represents the extreme tail (approx Mean + 5.6 Sigma)
        # ----------------------------------------------------------------------
        if hs_std is not None:
            self.sigma = float(hs_std)
        elif self.hs_max is not None:
            diff = self.hs_max - self.hs_mean
            self.sigma = max(0.0, diff / 5.6)
        else:
            self.sigma = 0.0

        # ----------------------------------------------------------------------
        # 2. CALCULATION OF YEARLY MEDIAN WAVE HEIGHT (Hm)
        # ----------------------------------------------------------------------
        # Required for Hallermeier (1983) Outer Limit.
        # Defined as: Mean - 0.3 * Sigma.
        # ----------------------------------------------------------------------
        self.hm = self.hs_mean - (0.3 * self.sigma)
        
        # ----------------------------------------------------------------------
        # 3. CALCULATION OF EFFECTIVE WAVE HEIGHT (He)
        # ----------------------------------------------------------------------
        # Height exceeded 12 hours/year (0.137%). Used for Inner Limit.
        self.he = self._calculate_he()

    def _calculate_he(self):
        """Estimates He (H_0.137%) based on USACE guidelines."""
        # Method A: Statistical Approx (Recommended)
        if self.sigma > 0:
            return self.hs_mean + 5.6 * self.sigma
        
        # Method B: Annual Max Proxy
        if self.hs_max:
            return self.hs_max
            
        # Method C: Generic Fallback
        return 4.0 * self.hs_mean


class DepthOfClosureCalculator:
    """
    Calculation engine implementing the specific coastal engineering formulas.
    """
    def __init__(self, wave_climate, d50_mm, sg=2.65):
        self.wc = wave_climate
        self.d50_m = d50_mm / 1000.0  # Convert mm to meters
        self.sg = sg
        self.g = 9.80665

    def birkemeier_1985(self):
        """
        METHOD: Birkemeier (1985) - Field Adjusted Inner Limit
        USE CASE: Beach Nourishment Design.
        """
        term1 = 1.75 * self.wc.he
        term2 = 57.9 * (self.wc.he**2 / (self.g * self.wc.te_mean**2))
        return term1 - term2

    def hallermeier_1981_inner(self):
        """
        METHOD: Hallermeier (1981) - Analytical Inner Limit
        USE CASE: Hard Structure Foundations (Conservative).
        """
        term1 = 2.28 * self.wc.he
        term2 = 68.5 * (self.wc.he**2 / (self.g * self.wc.te_mean**2))
        return term1 - term2

    def houston_1995(self):
        """
        METHOD: Houston (1995) - Simplified
        USE CASE: Preliminary Desktop Studies.
        """
        return 6.75 * self.wc.hs_mean

    def hallermeier_1983_outer(self):
        """
        METHOD: Hallermeier (1983) - Outer Depth of Closure
        USE CASE: Dredged Material Disposal / Shoal Limit.
        NOTE: Sensitive to Grain Size (1/sqrt(D50)).
        """
        # Sediment mobility factor
        sediment_factor = np.sqrt(self.g / (self.d50_m * (self.sg - 1)))
        
        # Uses Yearly Median Wave Height (Hm)
        return 0.018 * self.wc.hm * self.wc.te_mean * sediment_factor

    def get_results(self):
        return {
            "Birkemeier (Active)": self.birkemeier_1985(),
            "Hallermeier (Inner)": self.hallermeier_1981_inner(),
            "Houston (Mean)": self.houston_1995(),
            "Hallermeier (Outer)": self.hallermeier_1983_outer()
        }

def read_input_float(message, default):
    """Helper function to get user input with default values."""
    try:
        user_input = input(f"{message} [Default: {default}]: ").strip()
        if not user_input:
            return default
        return float(user_input)
    except ValueError:
        print(f"  >> Invalid input. Using default: {default}")
        return default

def generate_report_text(final_df, base_res, base_wc, d50_mm):
    """Generates the comprehensive technical report string."""

    text = f"""
================================================================================
TECHNICAL REPORT: DEPTH OF CLOSURE (DoC) SENSITIVITY ANALYSIS
================================================================================

1. EXECUTIVE SUMMARY OF BASE CASE
---------------------------------
   Site Conditions:
   - Annual Mean Hs.........: {base_wc.hs_mean:.2f} m
   - Annual Mean Te.........: {base_wc.te_mean:.2f} s
   - Annual Max Hs..........: {base_wc.hs_max:.2f} m
   - Sediment D50...........: {d50_mm:.2f} mm

   Calculated Intermediate Parameters:
   - Estimated Sigma (Std Dev)..: {base_wc.sigma:.3f} m
   - Effective Wave Height (He).: {base_wc.he:.2f} m (Used for Inner Limit)
   - Median Wave Height (Hm)....: {base_wc.hm:.2f} m (Used for Outer Limit)

2. DESIGN DECISION MATRIX (BASE CASE)
-------------------------------------
   Select the Closure Depth based on your engineering application:

   [A] FOR BEACH NOURISHMENT (Active Profile Toe)
       >> ADOPT: {base_res['Birkemeier (Active)']:.2f} meters (Birkemeier 1985)
       Reasoning: Field-calibrated to the limit of significant profile change.
       Optimizes sand volume.

   [B] FOR HARD STRUCTURES (Scour Protection)
       >> ADOPT: {base_res['Hallermeier (Inner)']:.2f} meters (Hallermeier 1981)
       Reasoning: Analytical limit of intense bed agitation. Provides a safety
       factor (deeper than Birkemeier).

   [C] FOR DREDGED MATERIAL DISPOSAL (Stable Mounds)
       >> ADOPT: {base_res['Hallermeier (Outer)']:.2f} meters (Hallermeier 1983)
       Reasoning: Theoretical limit of incipient sediment motion. Material placed
       seaward of this line will be stable.

3. SENSITIVITY ANALYSIS TABLE (ALL SCENARIOS)
---------------------------------------------
The following table tests the sensitivity of the DoC to changes in wave energy
and period. All depth values are in meters.

{final_df.to_string(index=False, float_format=lambda x: "{:.2f}".format(x))}

--------------------------------------------------------------------------------
Reference Codes:
- Birkemeier: Field Adjusted Inner Limit (Best for Fill)
- Hall_Inner: Analytical Inner Limit (Best for Structures)
- Hall_Outer: Incipient Motion Limit (Best for Disposal)
================================================================================
"""
    return text

# --- MAIN EXECUTION FUNCTION ---
def run_sensitivity_analysis():
    print("\n" + "="*110)
    print(f"{'DEPTH OF CLOSURE CALCULATOR & SENSITIVITY ANALYSIS':^110}")
    print("="*110)
    
    # --------------------------------------------------------------------------
    # 1. USER INPUTS
    # --------------------------------------------------------------------------
    print("\nPlease enter the wave climate parameters for the project site:")
    hs_mean_base = read_input_float("Annual Mean Hs (m)", 2.25)
    te_base = read_input_float("Mean Te (s)", 9.0)
    hs_max_base = read_input_float("Annual Max Hs (Proxy for He) (m)", 7.7)
    d50_mm = read_input_float("Sediment D50 (mm)", 0.50)
    
    print("\nCalculating scenarios and generating report...")

    # --------------------------------------------------------------------------
    # 2. SCENARIO DEFINITION
    # --------------------------------------------------------------------------
    scenarios = [
        {"Name": "Base Case",           "Hs_mean": hs_mean_base, "Te": te_base, "Hs_max": hs_max_base},
        {"Name": "Low Energy (-25%)",   "Hs_mean": hs_mean_base * 0.75, "Te": te_base, "Hs_max": hs_max_base * 0.75},
        {"Name": "High Energy (+25%)",  "Hs_mean": hs_mean_base * 1.25, "Te": te_base, "Hs_max": hs_max_base * 1.25},
        {"Name": "Short Period (-25%)", "Hs_mean": hs_mean_base, "Te": te_base * 0.75, "Hs_max": hs_max_base},
        {"Name": "Long Period (+25%)",  "Hs_mean": hs_mean_base, "Te": te_base * 1.25, "Hs_max": hs_max_base},
        {"Name": "Worst Case (H+, T+)", "Hs_mean": hs_mean_base * 1.25, "Te": te_base * 1.25, "Hs_max": hs_max_base * 1.25},
    ]
    
    results_list = []
    
    # --------------------------------------------------------------------------
    # 3. CALCULATION LOOP
    # --------------------------------------------------------------------------
    for sc in scenarios:
        # Initialize Wave Climate
        climate = WaveClimate(
            hs_mean=sc["Hs_mean"],
            te_mean=sc["Te"],
            hs_max_annual=sc["Hs_max"]
        )
        
        # Initialize Calculator
        calc = DepthOfClosureCalculator(climate, d50_mm=d50_mm)
        res = calc.get_results()
        
        # Collect All Intermediate & Final Data
        row = {
            "Scenario": sc["Name"],
            "Hs_mean": sc["Hs_mean"],
            "Te": sc["Te"],
            "He(Eff)": climate.he,   # Intermediate Param
            "Hm(Med)": climate.hm,   # Intermediate Param
            "Birkemeier": res["Birkemeier (Active)"],
            "Hall_Inner": res["Hallermeier (Inner)"],
            "Houston": res["Houston (Mean)"],
            "Hall_Outer": res["Hallermeier (Outer)"]
        }
        results_list.append(row)

    # --------------------------------------------------------------------------
    # 4. FORMATTING & EXPORT
    # --------------------------------------------------------------------------
    df = pd.DataFrame(results_list)
    
    # Re-calculate Base Case specifically for the verbose summary
    base_wc = WaveClimate(hs_mean_base, te_base, hs_max_base)
    base_calc = DepthOfClosureCalculator(base_wc, d50_mm)
    base_res = base_calc.get_results()
    
    final_content = generate_report_text(df, base_res, base_wc, d50_mm)

    try:
        filename = "output.txt"
        with open(filename, "w", encoding="utf-8") as f:
            f.write(final_content)
        print(f"\nSuccess! Detailed technical report exported to '{filename}'.")
        print("-" * 60)
        print(final_content) # Print to console as well
    except Exception as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    run_sensitivity_analysis()