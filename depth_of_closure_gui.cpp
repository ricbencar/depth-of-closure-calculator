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
// 4. GUI IMPLEMENTATION:
//    Converted from CLI to Win32 API GUI. 
//    - Inputs are gathered via Edit Controls.
//    - Output is displayed in a read-only Edit Control and appended to 'output.txt'.
//    - Output formatting is identical to the CLI version.
//
// 5. COMPILATION INSTRUCTIONS:
//    g++ -O3 -std=c++17 -municode depth_of_closure_gui.cpp -o depth_of_closure_gui -mwindows -static -static-libgcc -static-libstdc++
//
// ======================================================================================

// ----------------------------------------------------------------------
// MINGW STATIC LINKING SHIM
// ----------------------------------------------------------------------
#if defined(_WIN32) && defined(__GNUC__)
    #include <cstdio>
    extern "C" {
        int (*__imp_fseeko64)(FILE*, long long, int) = reinterpret_cast<int(*)(FILE*, long long, int)>(&_fseeki64);
        long long (*__imp_ftello64)(FILE*) = reinterpret_cast<long long(*)(FILE*)>(&_ftelli64);
    }
#endif

#define _USE_MATH_DEFINES 
#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <fstream>

// ----------------------------------------------------------------------
// DATA STRUCTURES (Ported from CLI)
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
// LOGIC CLASS
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

    ScenarioResult solve_scenario(std::string name, Inputs in) {
        WaveStats ws = calculate_wave_stats(in);
        DocResults dr = calculate_depths(in, ws);
        return { name, in, ws, dr };
    }

public:
    Inputs defaults;

    DepthOfClosureCalculator() {
        defaults = {
            2.25, // Hs Mean
            9.0,  // Te Mean
            7.7,  // Hs Max
            0.50, // D50 mm
            2.65  // SG
        };
    }

    std::vector<ScenarioResult> run_sensitivity_analysis(Inputs base_in) {
        std::vector<ScenarioResult> report_data;
        
        // Base Case
        report_data.push_back(solve_scenario("Base Case", base_in));

        // Low Energy (-25%)
        Inputs low_en = base_in;
        low_en.hs_mean *= 0.75; low_en.hs_max *= 0.75;
        report_data.push_back(solve_scenario("Low Energy (-25%)", low_en));

        // High Energy (+25%)
        Inputs high_en = base_in;
        high_en.hs_mean *= 1.25; high_en.hs_max *= 1.25;
        report_data.push_back(solve_scenario("High Energy (+25%)", high_en));

        // Short Period (-25%)
        Inputs short_per = base_in;
        short_per.te_mean *= 0.75;
        report_data.push_back(solve_scenario("Short Period (-25%)", short_per));

        // Long Period (+25%)
        Inputs long_per = base_in;
        long_per.te_mean *= 1.25;
        report_data.push_back(solve_scenario("Long Period (+25%)", long_per));

        // Worst Case (H+, T-)
        Inputs worst = base_in;
        worst.hs_mean *= 1.25; worst.hs_max *= 1.25; worst.te_mean *= 0.75;
        report_data.push_back(solve_scenario("Worst Case (H+, T-)", worst));

        return report_data;
    }

    // Adapted from CLI: Returns std::string instead of printing to file immediately
    // EXACT MATCH to CLI Output Format
    std::string generate_report_string(const std::vector<ScenarioResult>& data) {
        std::stringstream ss;
        
        auto fmt = [](double val, int prec) {
            std::stringstream stream;
            stream << std::fixed << std::setprecision(prec) << val;
            return stream.str();
        };

        const auto& base = data[0]; 

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

        // Header identical to CLI
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

        return ss.str();
    }
};

// ----------------------------------------------------------------------
// GUI UTILITIES
// ----------------------------------------------------------------------

// Convert ASCII/UTF-8 std::string to std::wstring for Windows Controls
std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Convert \n to \r\n for Windows Edit Controls
std::wstring fix_newlines_for_edit_control(const std::wstring &text) {
    std::wstring out;
    out.reserve(text.size() + 100);
    for (wchar_t c : text) {
        if (c == L'\n') {
            out.push_back(L'\r');
        }
        out.push_back(c);
    }
    return out;
}

// Save Report Helper
void SaveReportToFile(const std::string& report_content) {
    std::ofstream file("output.txt", std::ios::app);
    if (file.is_open()) {
        file << report_content; 
        file.close();
    }
}

// ----------------------------------------------------------------------
// WIN32 GUI SPECIFICS
// ----------------------------------------------------------------------

#define IDC_EDIT_HS_MEAN 101
#define IDC_EDIT_TE_MEAN 102
#define IDC_EDIT_HS_MAX  103
#define IDC_EDIT_D50     104
#define IDC_EDIT_SG      105
#define IDC_BUTTON_COMPUTE 106
#define IDC_OUTPUT       107

HWND hEditHsMean, hEditTeMean, hEditHsMax, hEditD50, hEditSG, hOutput;
DepthOfClosureCalculator calculator;

// Store fonts globally to delete them later
HFONT hMonoFont = NULL;
HFONT hUIFont = NULL;

void CreateControls(HWND hwnd) {
    // Font Size (20) for User Interface
    hUIFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    int y = 20;
    int lblW = 220; // Label Width
    int editX = 240;
    int editW = 100; // Edit Box Width
    int step = 40;   // Vertical Spacing

    auto fmt = [](double val) {
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(2) << val;
        return ss.str();
    };

    auto CreateLabel = [&](const wchar_t* text, int x, int y) {
        HWND h = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, lblW, 25, hwnd, NULL, NULL, NULL);
        SendMessageW(h, WM_SETFONT, (WPARAM)hUIFont, TRUE);
    };

    auto CreateInput = [&](const wchar_t* def, int id, int y) -> HWND {
        HWND h = CreateWindowW(L"EDIT", def, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 
                              editX, y, editW, 25, hwnd, (HMENU)(INT_PTR)id, NULL, NULL);
        SendMessageW(h, WM_SETFONT, (WPARAM)hUIFont, TRUE);
        return h;
    };

    // 1. Hs Mean
    CreateLabel(L"Annual Mean Hs (m):", 10, y);
    hEditHsMean = CreateInput(fmt(calculator.defaults.hs_mean).c_str(), IDC_EDIT_HS_MEAN, y);
    
    y += step;
    // 2. Te Mean
    CreateLabel(L"Mean Te (s):", 10, y);
    hEditTeMean = CreateInput(fmt(calculator.defaults.te_mean).c_str(), IDC_EDIT_TE_MEAN, y);

    y += step;
    // 3. Hs Max
    CreateLabel(L"Annual Max Hs (m):", 10, y);
    hEditHsMax = CreateInput(fmt(calculator.defaults.hs_max).c_str(), IDC_EDIT_HS_MAX, y);

    y += step;
    // 4. D50
    CreateLabel(L"Sediment D50 (mm):", 10, y);
    hEditD50 = CreateInput(fmt(calculator.defaults.d50_mm).c_str(), IDC_EDIT_D50, y);

    y += step;
    // 5. SG
    CreateLabel(L"Specific Gravity:", 10, y);
    hEditSG = CreateInput(fmt(calculator.defaults.sg).c_str(), IDC_EDIT_SG, y);

    y += 45;
    // Compute Button
    HWND hBtn = CreateWindowW(L"BUTTON", L"Calculate Depth of Closure", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                 10, y, 330, 40, hwnd, (HMENU)IDC_BUTTON_COMPUTE, NULL, NULL);
    SendMessageW(hBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);

    // Output Box
    // Positioned to the right of inputs
    hOutput = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER |
                           ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY | WS_HSCROLL | ES_AUTOHSCROLL,
                           400, 10, 760, 720, hwnd, (HMENU)IDC_OUTPUT, NULL, NULL);

    // Monospace Font for Report alignment (Courier New)
    hMonoFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Courier New");
    SendMessageW(hOutput, WM_SETFONT, (WPARAM)hMonoFont, TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateControls(hwnd);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_COMPUTE) {
            wchar_t buffer[64];
            Inputs inputs = calculator.defaults; 

            // Retrieve Inputs
            GetWindowTextW(hEditHsMean, buffer, 63); inputs.hs_mean = _wtof(buffer);
            GetWindowTextW(hEditTeMean, buffer, 63); inputs.te_mean = _wtof(buffer);
            GetWindowTextW(hEditHsMax, buffer, 63); inputs.hs_max = _wtof(buffer);
            GetWindowTextW(hEditD50, buffer, 63); inputs.d50_mm = _wtof(buffer);
            GetWindowTextW(hEditSG, buffer, 63); inputs.sg = _wtof(buffer);

            // Basic Validation
            if (inputs.hs_mean <= 0 || inputs.te_mean <= 0 || inputs.hs_max <= 0 || inputs.d50_mm <= 0) {
                 MessageBoxW(hwnd, L"Please enter positive values for all parameters.", L"Input Error", MB_ICONERROR | MB_OK);
                 break;
            }

            try {
                // Run Analysis
                auto results = calculator.run_sensitivity_analysis(inputs);
                
                // Generate Report (Exact CLI format)
                std::string report = calculator.generate_report_string(results);
                
                // Save to file (Append)
                SaveReportToFile(report); 

                // Display in GUI
                std::wstring wReport = StringToWString(report);
                std::wstring guiText = fix_newlines_for_edit_control(wReport);
                SetWindowTextW(hOutput, guiText.c_str());

            } catch (const std::exception& e) {
                const char* what = e.what();
                std::string errMsgStr(what);
                std::wstring errMsg = StringToWString(errMsgStr);
                MessageBoxW(hwnd, errMsg.c_str(), L"Calculation Error", MB_ICONERROR | MB_OK);
            }
        }
        break;

    case WM_DESTROY:
        if (hMonoFont) DeleteObject(hMonoFont);
        if (hUIFont) DeleteObject(hUIFont);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ----------------------------------------------------------------------
// MAIN ENTRY POINT
// ----------------------------------------------------------------------

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"DocCalcWindow";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Depth of Closure Calculator",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}