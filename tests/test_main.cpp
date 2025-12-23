#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <iomanip>
#include <sstream>
#include <regex>
// #define JINJA_DEBUG
#include "jinja.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string RED     = "\033[31m";
    const std::string GREEN   = "\033[32m";
    const std::string YELLOW  = "\033[33m";
    const std::string BLUE    = "\033[34m";
    const std::string CYAN    = "\033[36m";
    const std::string BOLD    = "\033[1m";
    const std::string GREY    = "\033[90m";
}

std::string load_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << Color::RED << "❌ Failed to open file: " << path << Color::RESET << std::endl;
        std::exit(1);
    }
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string visualize(const std::string& input) {
    std::string out;
    for (char c : input) {
        if (c == '\n') {
            out += Color::GREY + "\\n" + Color::RESET + "\n";
        } else if (c == '\r') {
            out += Color::GREY + "\\r" + Color::RESET;
        } else if (c == '\t') {
            out += "\\t";
        } else {
            out += c;
        }
    }
    return out;
}

std::string normalize_date(const std::string& input) {
    // Pattern 1: dd Mon YYYY (e.g., 26 Jul 2024 or 06 Dec 2025)
    std::regex pattern1(R"(\b\d{1,2} [A-Z][a-z]+ \d{4}\b)");
    // Pattern 2: YYYY-MM-DD (e.g., 2025-12-16)
    std::regex pattern2(R"(\b\d{4}-\d{2}-\d{2}\b)");

    std::string res = std::regex_replace(input, pattern1, "{{DATE}}");
    res = std::regex_replace(res, pattern2, "{{DATE}}");
    return res;
}

int main(int argc, char** argv) {
    auto start_total = std::chrono::high_resolution_clock::now();
    std::string json_path = "../tests/test_chat_template.json";
    std::string model_filter = "";
    if (argc > 1) {
        json_path = argv[1];
    }
    if (argc > 2) {
        model_filter = argv[2];
    }

    std::cout << "📂 Loading: " << json_path << std::endl;

    json all_data;
    try {
        all_data = json::parse(load_file(json_path));
    } catch (const std::exception& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return 1;
    }

    int total_models = 0;
    int total_cases = 0;
    int total_passed = 0;
    int total_failed = 0;
    std::vector<std::string> failed_models;

    for (auto it = all_data.begin(); it != all_data.end(); ++it) {
        std::string model_id = it.key();
        if (!model_filter.empty() && model_id.find(model_filter) == std::string::npos) {
            continue;
        }
        json& model_data = it.value();
        total_models++;

        std::cout << "\n" << Color::BLUE << Color::BOLD << "┏━━ Model: " << model_id << Color::RESET << std::endl;

        if (model_data.find("template") == model_data.end()) {
            std::cout << Color::RED << "┗━━ [SKIP] No template found." << Color::RESET << std::endl;
            continue;
        }

        json default_context = json::object();
        if (model_data.contains("special_tokens")) {
            default_context = model_data["special_tokens"];
        }
        default_context["strftime_now"] = true; // Ensure 'is defined' check passes

        std::string template_str = model_data["template"];
        std::unique_ptr<jinja::Template> chat_template;

        try {
            chat_template = std::unique_ptr<jinja::Template>(new jinja::Template(template_str, default_context));
        } catch (const std::exception& e) {
            std::cout << Color::RED << "┗━━ [ERROR] Template parse failed: " << e.what() << Color::RESET << std::endl;
            continue;
        }

        if (model_data.find("cases") == model_data.end() || !model_data["cases"].is_array()) {
            std::cout << Color::YELLOW << "┗━━ [WARN] No cases found." << Color::RESET << std::endl;
            continue;
        }

        const auto& cases = model_data["cases"];
        int model_case_count = 0;
        int model_fail_count = 0;

        for (const auto& test_case : cases) {
            model_case_count++;
            total_cases++;

            std::string desc = test_case.value("description", "unnamed");


            json messages = test_case.count("messages") ? test_case["messages"] : json::array();
            bool add_gen = test_case.value("add_generation_prompt", false);
            json tools = test_case.count("tools") ? test_case["tools"] : json::array();
            json extra = test_case.count("extra_context") ? test_case["extra_context"] : json::object();

            std::string result;
            bool runtime_error = false;
            std::string error_msg;

            auto start_case = std::chrono::high_resolution_clock::now();
            try {
                result = chat_template->apply_chat_template(messages, add_gen, tools, extra);
            } catch (const std::exception& e) {
                runtime_error = true;
                error_msg = e.what();
            }
            auto end_case = std::chrono::high_resolution_clock::now();
            auto duration_case = std::chrono::duration_cast<std::chrono::microseconds>(end_case - start_case).count();

            std::cout << "  ├─ " << std::left << std::setw(50) << desc << " ";
            std::cout << Color::GREY << "(" << std::fixed << std::setprecision(2) << duration_case / 1000.0 << "ms) " << Color::RESET;

            if (runtime_error) {
                std::cout << Color::RED << "[ERROR]" << Color::RESET << std::endl;
                std::cout << "     └─ Reason: " << error_msg << std::endl;
                total_failed++;
                model_fail_count++;
                continue;
            }

            if (test_case.find("expected") == test_case.end()) {
                std::cout << Color::YELLOW << "[SKIP]" << Color::RESET << std::endl;
                continue;
            }

            std::string expected = test_case["expected"];

            // Fuzzy date comparison
            std::string expected_norm = normalize_date(expected);
            std::string result_norm = normalize_date(result);

            if (result_norm == expected_norm) {
                std::cout << Color::GREEN << "[PASS]" << Color::RESET << std::endl;
                total_passed++;
            } else {
                std::cout << Color::RED << "[FAIL]" << Color::RESET << std::endl;
                total_failed++;
                model_fail_count++;

                std::cout << Color::GREY << "     ┌── Expected ──────────────────────────────────" << Color::RESET << std::endl;
                std::string vis_exp = visualize(expected);
                std::cout << "     │ " << vis_exp << std::endl;
                std::cout << Color::GREY << "     ├── Actual ────────────────────────────────────" << Color::RESET << std::endl;
                std::string vis_act = visualize(result);
                std::cout << "     │ " << vis_act << std::endl;
                std::cout << Color::GREY << "     └──────────────────────────────────────────────" << Color::RESET << std::endl;
            }
        }
        if (model_fail_count > 0) {
            failed_models.push_back(model_id);
        }
    }

    std::cout << "\n";
    std::cout << "==================================================" << std::endl;
    std::cout << "               TEST SUMMARY                       " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << " Models Tested : " << total_models << std::endl;
    std::cout << " Total Cases   : " << total_cases << std::endl;
    std::cout << Color::GREEN << " Passed        : " << total_passed << Color::RESET << std::endl;

    auto end_total = std::chrono::high_resolution_clock::now();
    auto duration_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_total - start_total).count();

    if (total_failed > 0) {
        std::cout << Color::RED   << " Failed        : " << total_failed << Color::RESET << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << " Total Time    : " << duration_total << "ms" << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << " Failed Models:" << std::endl;
        for (const auto& m : failed_models) {
            std::cout << Color::RED << "  - " << m << Color::RESET << std::endl;
        }
        return 1;
    } else {
        std::cout << Color::GREEN << " Failed        : 0" << Color::RESET << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << " Total Time    : " << duration_total << "ms" << std::endl;
        std::cout << "\n✨ All tests passed! ✨" << std::endl;
        return 0;
    }
}