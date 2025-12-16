# 实现细节 (Implementation Details)

本文档概述了 `jinja.cpp` 的内部架构和设计决策。

## 架构

引擎遵循标准的 编译器/解释器 (Compiler/Interpreter) 管道流程：

1.  **词法分析器 (Lexer - `Lexer` 类)**:
    *   扫描输入字符串。
    *    tokenize Jinja 定界符 `{{ ... }}`, `{% ... %}`, `{# ... #}`。
    *   处理空白控制修饰符 (`-`, 例如 `{{-`)，通过跟踪状态并剥离文本 token 的前导/尾随空白。
    *   生成一个扁平的 `Token` 列表。

2.  **解析器 (Parser - `Parser` 类)**:
    *   递归下降解析器 (Recursive Descent Parser)。
    *   将有效的 token 转换为抽象语法树 (AST)。
    *   处理表达式的运算符优先级。
    *   支持：
        *   二元运算符 (`+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `and`, `or`, `in`, `not in`, `~`)。
        *   一元运算符 (`not`, `-`)。
        *   字面量 (字符串, 数字, 布尔值, 数组, 对象)。
        *   变量和属性访问 (`foo.bar`, `foo['bar']`)。
        *   函数调用和过滤器 (`foo | filter`)。
        *   控制结构 (`for`, `if`, `set`, `macro`).

3.  **抽象语法树 (AST - `Node` 层次结构)**:
    *   基类 `Node` 具有虚函数 `render(Context&, string& out)`。
    *   节点类型：`TextNode`, `PrintNode`, `ForStmt`, `IfNode`, `SetNode`, `MacroNode`。
    *   表达式 (`Expr` 层次结构) 计算结果为 `nlohmann::json` 值。

4.  **解释器 / 渲染器 (Interpreter / Renderer - `Template::render`)**:
    *   遍历根节点并调用 `render`。
    *   管理 `Context` (作用域, 变量)。

## 支持的特性 (Supported Features)

### 过滤器 (Filters)
*   **`tojson(indent=None)`**: 将变量序列化为 JSON 字符串。支持缩进。
*   **`safe`**: 将字符串标记为安全 (在此实现中为无操作，因为默认不强制 HTML 转义，但为了兼容性而支持)。*注意：通过透传隐式支持。*
*   **`string`**: 将值转换为其字符串表示形式。
*   **`length`**: 返回列表、字符串或对象的大小。
*   **`trim`**: 移除字符串首尾的空白字符。
*   **`items`**: 返回字典的 `[key, value]` 对列表 (用于遍历对象)。
*   **`capitalize`**: 将字符串的首字母大写，其余小写。
*   **`lower`**: 将字符串转换为小写。
*   **`upper`**: 将字符串转换为大写。
*   **`map(attribute=name)`**: 从列表中的每个元素提取特定属性 (例如 `users | map(attribute='name')`)。

### 全局函数 (Global Functions)
*   **`range([start], stop, [step])`**: 生成整数序列。
*   **`namespace(...)`**: 创建一个可变对象，用于在循环内部更新变量 (例如 `set ns.i = ns.i + 1`)。
*   **`strftime_now(format)`**: 返回按给定字符串格式化的当前时间。

### 测试Tests (`is ...`)
*   **`defined`**: 检查变量是否存在。
*   **`undefined`**: 检查变量是否未定义。
*   **`none`**: 检查变量是否为 null。
*   **`boolean`**: 检查变量是否为布尔值。
*   **`string`**: 检查变量是否为字符串。
*   **`number`**: 检查变量是否为数字。
*   **`sequence` / `iterable`**: 检查变量是否为列表或字符串。
*   **`mapping`**: 检查变量是否为对象/字典。
*   **`true` / `false`**: 检查布尔值。

## 关键实现特性

### 1. JSON 数据模型
我们使用 `nlohmann::json` 作为所有变量的统一数据类型。这简化了类型检查，并允许与基于 JSON 的 LLM API 轻松集成。

### 2. 自定义函数 / 过滤器分发
*   **过滤器 (Filters)**: 在 `FilterExpr` 中实现。标准的 Jinja2 过滤器如 `safe`, `tojson`, `trim`, `lower` 是硬编码的。
*   **函数 (Functions)**: `CallExpr` 处理全局函数 (`range`, `namespace`) 和用户注册的函数。
*   **用户钩子 (User Hooks)**: `Template::add_function` 允许用户将 C++ lambda 绑定到 Jinja 函数调用。

### 3. `tojson` 序列化
对 JSON 序列化的严格控制对于对话模板 (例如 Tool 定义) 至关重要。
我们在 `src/jinja.cpp` 中实现了一个自定义的递归序列化器 `to_json_string`，它：
*   支持与 Python 通用输出匹配的缩进。
*   **针对键进行排序**：按照特定顺序 (`type` -> `function` -> `name` -> ...) 对键进行排序，以匹配常见的 LLM 训练数据格式，确保高度一致性。

### 4. 空白控制
Lexer 部分模拟了 Jinja2 的 `lstrip_blocks` 和 `trim_blocks` 行为。手动实现的空白剥离逻辑 (`trim_prev`, `trim_next`) 确保生成的 prompt 不含多余的换行符，这可能会影响 LLM 的性能。

### 5. C++11 兼容性
为了支持广泛的部署环境：
*   结构化绑定 (Structured bindings) 被标准迭代器取代。
*   为 C++11 使用了 `std::make_unique` polyfill。

## 测试策略

*   **真实数据**: 我们使用 `tests/test_chat_template.json`，该文件是从官方 Python `transformers` 库针对通常支持的模型生成的。
*   **模糊匹配**: 对于动态内容 (如日期)，测试使用正则归一化来确跨时间和环境的通过一致性。
