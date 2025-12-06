// ======================================================================================
// PROGRAM DESCRIPTION & METHODOLOGY
// ======================================================================================
//
// 1. PURPOSE:
//    This software calculates the Depth of Closure (DoC), which is the theoretical 
//    depth at which the exchange of sediment between the nearshore and the 
//    offshore becomes negligible for engineering purposes.
//
//    It performs a SENSITIVITY ANALYSIS by calculating these depths across a range
//    of wave energy and period scenarios.
//
// 2. THEORETICAL FRAMEWORKS IMPLEMENTED:
//
//    a. Birkemeier (1985) - Field Adjusted Model (Inner DoC):
//       - Generally predicts shallower depths than Hallermeier.
//       - Best for Beach Nourishment Design.
//
//    b. Hallermeier (1981) - Analytical Model (Inner DoC):
//       - Based on critical Froude number for bed agitation.
//       - Best for Hard Structure foundations.
//
//    c. Houston (1995) - Simplified Parameterization:
//       - Approximation based on Mean Annual Wave Height.
//       - Best for preliminary desktop studies.
//
//    d. Hallermeier (1983) - Sediment Entrainment Model (Outer DoC):
//       - Defines the seaward limit of the "Shoal Zone".
//       - Dependent on sediment grain size (D50).
//       - Best for Dredged Material Disposal planning.
//
// 3. INPUT DATA LOGIC:
//    The script derives statistical wave parameters (Sigma, He, Hm) from:
//    - Annual Mean Hs
//    - Annual Max Hs (used as a proxy for the extreme tail of the distribution)
//    - Sediment D50
//
// 4. COMPILATION INSTRUCTIONS:
//
// Compilation (example using g++ on Windows/Linux):
//
// g++ -O3 -march=native -std=c++17 -Wall -Wextra -static -static-libgcc -static-libstdc++ 
// -o depth_of_closure_cli depth_of_closure_cli.cpp
//
// 5. EXECUTION EXAMPLES:
//
// 1. Interactive Mode (No arguments):
// depth_of_closure_cli.exe
//
// 2. Command Line Arguments Mode:
// Format: depth_of_closure_cli.exe [Hs_Mean] [Te_Mean] [Hs_Max] [D50_mm]
//
// Example (Hs=2.25, Te=9.0, Max=7.7, D50=0.5):
// depth_of_closure_cli.exe 2.25 9.0 7.7 0.5
//
// ======================================================================================

// ----------------------------------------------------------------------
// MINGW STATIC LINKING SHIM
// Fixes "undefined reference to __imp_fseeko64" errors in GCC 14/15+ on Windows
// ----------------------------------------------------------------------
#if defined(_WIN32) && defined(__GNUC__)
    #include <cstdio>
    extern "C" {
        // Force the linker to use the local _fseeki64 when it asks for the import stub
        int (*__imp_fseeko64)(FILE*, long long, int) = reinterpret_cast<int(*)(FILE*, long long, int)>(&_fseeki64);
        long long (*__imp_ftello64)(FILE*) = reinterpret_cast<long long(*)(FILE*)>(&_ftelli64);
    }
#endif
// ----------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>

// ----------------------------------------------------------------------
// DATA STRUCTURES
// ----------------------------------------------------------------------

struct Inputs {
    double hs_mean;
    double te_mean;
    double hs_max;     // Annual Maximum
    double d50_mm;     // Sediment diameter in mm
    double sg;         // Specific Gravity (default 2.65)
};

struct WaveStats {
    double sigma;      // Standard Deviation
    double he;         // Effective Wave Height (0.137% exceedance)
    double hm;         // Median Wave Height
};

struct DocResults {
    double birkemeier;
    double hallermeier_inner;
    double houston;
    double hallermeier_outer;
};

struct ScenarioResult {
    std::string name;
    Inputs input;
    WaveStats stats;
    DocResults results;
};

// ----------------------------------------------------------------------
// CLASS DEFINITION
// ----------------------------------------------------------------------

class DepthOfClosureCalculator {
private:
    const double g = 9.80665;

    // Helper: Calculates He, Hm, and Sigma based on input statistics
    WaveStats calculate_wave_stats(const Inputs& in) {
        WaveStats ws;
        
        // 1. Estimation of Standard Deviation (Sigma)
        // Assumption: Hs_max represents the extreme tail (approx Mean + 5.6 Sigma)
        double diff = in.hs_max - in.hs_mean;
        ws.sigma = (diff > 0) ? (diff / 5.6) : 0.0;

        // 2. Calculation of Yearly Median Wave Height (Hm)
        // Defined as Mean - 0.3 * Sigma
        ws.hm = in.hs_mean - (0.3 * ws.sigma);

        // 3. Calculation of Effective Wave Height (He)
        // Method A: Statistical Approx (Recommended)
        if (ws.sigma > 0) {
            ws.he = in.hs_mean + 5.6 * ws.sigma;
        } 
        // Method B: Annual Max Proxy
        else if (in.hs_max > 0) {
            ws.he = in.hs_max;
        }
        // Method C: Generic Fallback
        else {
            ws.he = 4.0 * in.hs_mean;
        }

        return ws;
    }

    // Core Calculation Logic
    DocResults calculate_depths(const Inputs& in, const WaveStats& ws) {
        DocResults res;
        double d50_m = in.d50_mm / 1000.0; // Convert to meters

        // 1. Birkemeier (1985) - Field Adjusted Inner Limit
        double term1_b = 1.75 * ws.he;
        double term2_b = 57.9 * (std::pow(ws.he, 2) / (g * std::pow(in.te_mean, 2)));
        res.birkemeier = term1_b - term2_b;

        // 2. Hallermeier (1981) - Analytical Inner Limit
        double term1_h = 2.28 * ws.he;
        double term2_h = 68.5 * (std::pow(ws.he, 2) / (g * std::pow(in.te_mean, 2)));
        res.hallermeier_inner = term1_h - term2_h;

        // 3. Houston (1995) - Simplified
        res.houston = 6.75 * in.hs_mean;

        // 4. Hallermeier (1983) - Outer Depth of Closure
        double sediment_factor = std::sqrt(g / (d50_m * (in.sg - 1.0)));
        res.hallermeier_outer = 0.018 * ws.hm * in.te_mean * sediment_factor;

        return res;
    }

public:
    Inputs defaults;

    DepthOfClosureCalculator() {
        // Default values similar to Python script
        defaults = {
            2.25, // Hs Mean
            9.0,  // Te Mean
            7.7,  // Hs Max
            0.50, // D50 mm
            2.65  // SG
        };
    }

    // Runs a single scenario calculation
    ScenarioResult solve_scenario(std::string name, Inputs in) {
        WaveStats ws = calculate_wave_stats(in);
        DocResults dr = calculate_depths(in, ws);
        return { name, in, ws, dr };
    }

    // Runs the full sensitivity analysis suite
    std::vector<ScenarioResult> run_sensitivity_analysis(Inputs base_in) {
        std::vector<ScenarioResult> report_data;
        
        // Scenario 1: Base Case
        report_data.push_back(solve_scenario("Base Case", base_in));

        // Scenario 2: Low Energy (-25%)
        Inputs low_en = base_in;
        low_en.hs_mean *= 0.75;
        low_en.hs_max *= 0.75;
        report_data.push_back(solve_scenario("Low Energy (-25%)", low_en));

        // Scenario 3: High Energy (+25%)
        Inputs high_en = base_in;
        high_en.hs_mean *= 1.25;
        high_en.hs_max *= 1.25;
        report_data.push_back(solve_scenario("High Energy (+25%)", high_en));

        // Scenario 4: Short Period (-25%)
        Inputs short_per = base_in;
        short_per.te_mean *= 0.75;
        report_data.push_back(solve_scenario("Short Period (-25%)", short_per));

        // Scenario 5: Long Period (+25%)
        Inputs long_per = base_in;
        long_per.te_mean *= 1.25;
        report_data.push_back(solve_scenario("Long Period (+25%)", long_per));

        // Scenario 6: Worst Case (H+, T-)
        Inputs worst = base_in;
        worst.hs_mean *= 1.25;
        worst.hs_max *= 1.25;
        worst.te_mean *= 0.75;
        report_data.push_back(solve_scenario("Worst Case (H+, T-)", worst));

        return report_data;
    }

    void generate_report_file(const std::vector<ScenarioResult>& data, std::string filepath = "output.txt") {
        std::stringstream ss;
        
        // Helper lambdas for formatting
        auto fmt = [](double val, int prec) {
            std::stringstream stream;
            stream << std::fixed << std::setprecision(prec) << val;
            return stream.str();
        };

        const auto& base = data[0]; // First element is always base case

        ss << "================================================================================" << "\n";
        ss << "TECHNICAL REPORT: DEPTH OF CLOSURE (DoC) SENSITIVITY ANALYSIS" << "\n";
        ss << "================================================================================" << "\n\n";

        // 1. EXECUTIVE SUMMARY
        ss << "1. EXECUTIVE SUMMARY OF BASE CASE" << "\n";
        ss << "---------------------------------" << "\n";
        ss << "   Site Conditions:" << "\n";
        ss << "   - Annual Mean Hs.........: " << fmt(base.input.hs_mean, 2) << " m" << "\n";
        ss << "   - Annual Mean Te.........: " << fmt(base.input.te_mean, 2) << " s" << "\n";
        ss << "   - Annual Max Hs..........: " << fmt(base.input.hs_max, 2) << " m" << "\n";
        ss << "   - Sediment D50...........: " << fmt(base.input.d50_mm, 2) << " mm" << "\n\n";

        ss << "   Calculated Intermediate Parameters:" << "\n";
        ss << "   - Estimated Sigma (Std Dev)..: " << fmt(base.stats.sigma, 3) << " m" << "\n";
        ss << "   - Effective Wave Height (He).: " << fmt(base.stats.he, 2) << " m (Used for Inner Limit)" << "\n";
        ss << "   - Median Wave Height (Hm)....: " << fmt(base.stats.hm, 2) << " m (Used for Outer Limit)" << "\n\n";

        // 2. DESIGN DECISION MATRIX
        ss << "2. DESIGN DECISION MATRIX (BASE CASE)" << "\n";
        ss << "-------------------------------------" << "\n";
        ss << "   Select the Closure Depth based on your engineering application:" << "\n\n";
        
        ss << "   [A] FOR BEACH NOURISHMENT (Active Profile Toe)" << "\n";
        ss << "       >> ADOPT: " << fmt(base.results.birkemeier, 2) << " meters (Birkemeier 1985)" << "\n";
        ss << "       Reasoning: Field-calibrated to the limit of significant profile change." << "\n";
        ss << "       Optimizes sand volume." << "\n\n";

        ss << "   [B] FOR HARD STRUCTURES (Scour Protection)" << "\n";
        ss << "       >> ADOPT: " << fmt(base.results.hallermeier_inner, 2) << " meters (Hallermeier 1981)" << "\n";
        ss << "       Reasoning: Analytical limit of intense bed agitation. Provides a safety" << "\n";
        ss << "       factor (deeper than Birkemeier)." << "\n\n";

        ss << "   [C] FOR DREDGED MATERIAL DISPOSAL (Stable Mounds)" << "\n";
        ss << "       >> ADOPT: " << fmt(base.results.hallermeier_outer, 2) << " meters (Hallermeier 1983)" << "\n";
        ss << "       Reasoning: Theoretical limit of incipient sediment motion. Material placed" << "\n";
        ss << "       seaward of this line will be stable." << "\n\n";

        // 3. SENSITIVITY ANALYSIS TABLE
        ss << "3. SENSITIVITY ANALYSIS TABLE (ALL SCENARIOS)" << "\n";
        ss << "---------------------------------------------" << "\n";
        ss << "All depths in meters." << "\n\n";

        // Table Header
        ss << std::left << std::setw(22) << "Scenario" 
           << std::right << std::setw(8) << "Hs_Avg" 
           << std::setw(8) << "Te" 
           << std::setw(8) << "He" 
           << std::setw(8) << "Hm" 
           << std::setw(12) << "Birkemeier" 
           << std::setw(12) << "Hall_Inner" 
           << std::setw(10) << "Houston" 
           << std::setw(12) << "Hall_Outer" << "\n";
        
        ss << std::string(100, '-') << "\n";

        // Table Rows
        for (const auto& row : data) {
            ss << std::left << std::setw(22) << row.name
               << std::right << std::setw(8) << fmt(row.input.hs_mean, 2)
               << std::setw(8) << fmt(row.input.te_mean, 1)
               << std::setw(8) << fmt(row.stats.he, 2)
               << std::setw(8) << fmt(row.stats.hm, 2)
               << std::setw(12) << fmt(row.results.birkemeier, 2)
               << std::setw(12) << fmt(row.results.hallermeier_inner, 2)
               << std::setw(10) << fmt(row.results.houston, 2)
               << std::setw(12) << fmt(row.results.hallermeier_outer, 2) << "\n";
        }
        
        ss << "\n" << std::string(100, '-') << "\n";
        ss << "Reference Codes:\n";
        ss << "- Birkemeier: Field Adjusted Inner Limit (Best for Fill)\n";
        ss << "- Hall_Inner: Analytical Inner Limit (Best for Structures)\n";
        ss << "- Hall_Outer: Incipient Motion Limit (Best for Disposal)\n";
        ss << "================================================================================" << "\n";

        std::string report_content = ss.str();

        // Write to file
        std::ofstream outfile(filepath);
        if (outfile.is_open()) {
            outfile << report_content;
            outfile.close();
            std::cout << "\nSuccess! Detailed technical report exported to '" << filepath << "'." << std::endl;
            std::cout << std::string(60, '-') << "\n";
            std::cout << report_content << std::endl;
        } else {
            std::cerr << "\nError writing file." << std::endl;
        }
    }
};

// ==============================================================================
// MAIN EXECUTION BLOCK
// ==============================================================================
int main(int argc, char* argv[]) {
    DepthOfClosureCalculator calc;

    // Helper to get param with default
    auto get_param = [](std::string prompt, double default_val) -> double {
        std::cout << prompt << " [Default: " << default_val << "]: ";
        std::string input_str;
        std::getline(std::cin, input_str);
        if (input_str.empty()) {
            return default_val;
        }
        try {
            return std::stod(input_str);
        } catch (...) {
            std::cout << "  >> Invalid input. Using default: " << default_val << "\n";
            return default_val;
        }
    };

    Inputs user_inputs = calc.defaults;

    // Check if command line arguments are provided
    if (argc >= 5) {
        // Parse CLI arguments
        try {
            user_inputs.hs_mean = std::stod(argv[1]);
            user_inputs.te_mean = std::stod(argv[2]);
            user_inputs.hs_max  = std::stod(argv[3]);
            user_inputs.d50_mm  = std::stod(argv[4]);
            
            // Optional: SG
            if (argc >= 6) {
                user_inputs.sg = std::stod(argv[5]);
            }

        } catch (const std::exception& e) {
            std::cerr << "Error parsing command line arguments: " << e.what() << std::endl;
            std::cerr << "Usage: " << argv[0] << " [Hs_mean] [Te] [Hs_max] [D50_mm] (optional: SG)" << std::endl;
            return 1;
        }
    } else {
        // Interactive Mode
        std::cout << "\n" << std::string(110, '=') << "\n";
        std::cout << "                DEPTH OF CLOSURE CALCULATOR & SENSITIVITY ANALYSIS\n";
        std::cout << std::string(110, '=') << "\n";
        
        std::cout << "\nPlease enter the wave climate parameters for the project site:" << std::endl;
        
        user_inputs.hs_mean = get_param("Annual Mean Hs (m)", calc.defaults.hs_mean);
        user_inputs.te_mean = get_param("Mean Te (s)", calc.defaults.te_mean);
        user_inputs.hs_max  = get_param("Annual Max Hs (Proxy for He) (m)", calc.defaults.hs_max);
        user_inputs.d50_mm  = get_param("Sediment D50 (mm)", calc.defaults.d50_mm);
    }

    std::cout << "\nCalculating scenarios and generating report..." << std::endl;

    try {
        auto results = calc.run_sensitivity_analysis(user_inputs);
        calc.generate_report_file(results, "output.txt");
    } catch (const std::exception& e) {
        std::cout << "\n Calculation Error: " << e.what() << std::endl;
    }

    return 0;
}