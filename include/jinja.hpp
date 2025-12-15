#pragma once
#include <string>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace jinja {

// Alias for convenience
using json = nlohmann::json;
using UserFunction = std::function<json(const std::vector<json>&)>;

/**
 * @brief A lightweight, C++11 compatible Jinja2 template renderer.
 *
 * Designed specifically for LLM chat templates (HuggingFace style).
 * It supports a subset of Jinja2 syntax used in modern models like
 * Qwen 3, Llama 3 and Other LLMs.
 */
class Template {
public:
    /**
     * @brief Construct and compile a Jinja template.
     *
     * @param template_str The Jinja2 template string (e.g., from `tokenizer_config.json`).
     * @param default_context Optional global variables (e.g., `bos_token`, `eos_token`, fixed `tools`).
     *                        These variables are accessible in the template but can be overridden
     *                        by request-specific context in `render()`.
     *
     * @throws std::runtime_error If the template syntax is invalid (e.g., mismatched tags).
     */
    Template(const std::string& template_str, const json& default_context = json::object());

    /**
     * @brief Destructor. Required for PImpl with unique_ptr.
     */
    ~Template();

    // Move semantics (allowed)
    Template(Template&&) noexcept;
    Template& operator=(Template&&) noexcept;

    // Copy semantics (deleted to avoid expensive AST copying)
    Template(const Template&) = delete;
    Template& operator=(const Template&) = delete;

    /**
     * @brief Core rendering function.
     *
     * Renders the template using the provided context.
     * Thread-Safety: This function is const and thread-safe.
     * Multiple threads can call render() on the same Template instance simultaneously.
     *
     * @param context The JSON context containing variables like 'messages', 'tools', etc.
     * @return std::string The formatted string (prompt).
     */
    std::string render(const json& context) const;

    /**
     * @brief Register a custom function to be used in the template.
     */
    void add_function(const std::string& name, UserFunction func);

    /**
     * @brief Helper function mimicking HuggingFace's `apply_chat_template`.
     *
     * Automatically assembles the context object and calls render().
     *
     * @param messages The chat history (list of objects with 'role' and 'content').
     * @param add_generation_prompt Whether to append the start token for the assistant's response.
     * @param tools Optional list of available tools/functions.
     * @param extra_context Any additional variables required by the template
     *                      (e.g., 'date_string', 'documents', custom flags).
     * @return std::string The formatted prompt.
     */
    std::string apply_chat_template(
        const json& messages,
        bool add_generation_prompt = true,
        const json& tools = json::array(),
        const json& extra_context = json::object()
    ) const;

private:
    // PImpl idiom to hide the AST and Interpreter implementation details.
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace jinja
