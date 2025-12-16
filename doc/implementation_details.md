# Implementation Details

This document provides an overview of the internal architecture and design decisions of `jinja.cpp`.

## Architecture

The engine follows a standard compiler/interpreter pipeline:

1.  **Lexer (`Lexer` class)**:
    *   Scans the input string.
    *   Tokenizes Jinja delimiters `{{ ... }}`, `{% ... %}`, `{# ... #}`.
    *   Handles whitespace control modifiers (`-`, like `{{-`) by tracking state and stripping preceding/succeeding whitespace from text tokens.
    *   Produces a flat list of `Token`s.

2.  **Parser (`Parser` class)**:
    *   Recursive Descent Parser.
    *   Converts the valid tokens into an Abstract Syntax Tree (AST).
    *   Handles operator precedence for expressions.
    *   Supports:
        *   Binary operators (`+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `and`, `or`, `in`, `not in`, `~`).
        *   Unary operators (`not`, `-`).
        *   Literals (String, Number, Boolean, Array, Object).
        *   Variables and Attribute Access (`foo.bar`, `foo['bar']`).
        *   Function Calls and Filters (`foo | filter`).
        *   Control Structures (`for`, `if`, `set`, `macro`).

3.  **AST (`Node` hierarchy)**:
    *   Base `Node` class with virtual `render(Context&, string& out)` method.
    *   Nodes: `TextNode`, `PrintNode`, `ForStmt`, `IfNode`, `SetNode`, `MacroNode`.
    *   Expressions (`Expr` hierarchy) evaluate to `nlohmann::json` values.

4.  **Interpreter / Renderer (`Template::render`)**:
    *   Iterates through root nodes and calls `render`.
    *   Manages `Context` (scopes, variables).

## Supported Features

### Filters
*   **`tojson(indent=None)`**: Serializes a variable to JSON string. Supports indentation.
*   **`safe`**: Marks a string as safe (no-op in this implementation as HTML escaping is not enforced by default, but supported for compatibility). *Note: Implicitly supported by pass-through.*
*   **`string`**: Converts a value to its string representation.
*   **`length`**: Returns the size of a list, string, or object.
*   **`trim`**: Removes leading and trailing whitespace from a string.
*   **`items`**: Returns a list of `[key, value]` pairs from a dictionary (useful for iterating over objects).
*   **`capitalize`**: Capitalizes the first character of a string and lowercases the rest.
*   **`lower`**: Converts a string to lowercase.
*   **`upper`**: Converts a string to uppercase.
*   **`map(attribute=name)`**: Extracts a specific attribute from each element in a list (e.g., `users | map(attribute='name')`).

### Global Functions
*   **`range([start], stop, [step])`**: Generates a sequence of integers.
*   **`namespace(...)`**: Creates a mutable object, useful for updating variables inside loops (e.g., `set ns.i = ns.i + 1`).
*   **`strftime_now(format)`**: Returns the current time formatted according to the given string.

### Tests (`is ...`)
*   **`defined`**: Checks if a variable exists.
*   **`undefined`**: Checks if a variable is not defined.
*   **`none`**: Checks if a variable is null.
*   **`boolean`**: Checks if a variable is a boolean.
*   **`string`**: Checks if a variable is a string.
*   **`number`**: Checks if a variable is a number.
*   **`sequence` / `iterable`**: Checks if a variable is a list or string.
*   **`mapping`**: Checks if a variable is an object/dictionary.
*   **`true` / `false`**: Checks boolean value.

## Key Implementation Features

### 1. JSON Data Model
We utilize `nlohmann::json` as the unified data type for all variables. This simplifies type checking and allows easy integration with JSON-based LLM APIs.

### 2. Custom Function / Filter Dispatch
*   **Filters**: Implemented in `FilterExpr`. Standard Jinja2 filters like `safe`, `tojson`, `trim`, `lower` are hardcoded.
*   **Functions**: `CallExpr` handles global functions (`range`, `namespace`) and user-registered functions.
*   **User Hooks**: `Template::add_function` allows users to bind C++ lambdas to Jinja function calls.

### 3. `tojson` Serialization
Strict control over JSON serialization is critical for chat templates (e.g., Tool definitions).
We implemented a custom recursive serializer `to_json_string` (in `src/jinja.cpp`) that:
*   Supports indentation matching Python's generic output.
*   **Sorts keys** in a specific order (`type` -> `function` -> `name` -> ...) to match common LLM training data formats, ensuring high consistency.

### 4. Whitespace Control
Jinja2's `lstrip_blocks` and `trim_blocks` behavior is partially emulated in the Lexer. The manual whitespace stripping logic (`trim_prev`, `trim_next`) ensures that the generated prompt doesn't contain excess newlines, which can affect LLM performance.

### 5. C++11 Compatibility
To support a wide range of deployment environments:
*   Structure bindings were replaced with standard iterators.
*   `std::make_unique` polyfill used for C++11.

## Testing Strategy

*   **Real Data**: We use `tests/test_chat_template.json` generated from the official Python `transformers` library on typically supported models.
*   **Fuzzy Matching**: For dynamic content (like dates), tests use regex normalization to ensure pass consistency across time and environments.
