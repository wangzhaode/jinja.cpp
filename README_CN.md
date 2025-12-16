# jinja.cpp (中文版)

![License](https://img.shields.io/badge/license-Apache%20License%202.0-green)
![Build Status](https://github.com/wangzhaode/jinja.cpp/actions/workflows/build.yml/badge.svg)
[![English Version](https://img.shields.io/badge/Language-English-green)](README.md)

一个轻量级、完全兼容 C++11 的 Jinja2 模板引擎实现，专为 **LLM 对话模板** (HuggingFace 风格) 设计。

它专注于支持现代大语言模型 (如 Llama 3, Qwen 2.5/3, DeepSeek 等) 所需的 Jinja2 语法子集，使得在 C++ 环境中进行推理集成变得无缝且高效。

## 特性

- **C++11 兼容**：确保在旧版编译器和嵌入式系统上的最大兼容性。
- **轻量级**：依赖极少 (仅依赖 `nlohmann/json`，已包含在项目中)。
- **专注 LLM**：原生支持 `messages`, `tools`, `add_generation_prompt` 以及特殊 token 的处理。
- **类型安全**：使用 `nlohmann::json` 进行上下文管理。
- **自定义函数**：支持轻松注入 C++ 函数 (如 `strftime_now`) 到模板中。
- **健壮性**：通过模糊匹配测试，与官方 Python `transformers` 输出进行对齐验证。

## 支持的模型

已基于以下模型的真实模板进行测试验证：
- **Llama 3 / 3.1 / 3.2** (Instruct & Vision)
- **Qwen 2.5** (Coder, Math, VL, Omni)
- **Qwen 3** (Instruct, Thinking, QwQ)
- **DeepSeek** (V3, R1)
- **Mistral**
- **Gemma**
- 更多...

## 构建指南

### 前置要求
- CMake 3.10+
- 支持 C++11 的编译器 (GCC, Clang, MSVC)

```bash
mkdir build
cd build
cmake ..
make
```

### 运行测试

本项目包含一个基于真实模型模板的全面测试套件。

```bash
./test_main
```

## 使用方法

### 基础渲染

```cpp
#include "jinja.hpp"
#include <iostream>

int main() {
    std::string template_str = "Hello {{ name }}!";
    jinja::Template tpl(template_str);

    nlohmann::json context;
    context["name"] = "World";

    std::string result = tpl.render(context);
    std::cout << result << std::endl; // 输出: Hello World!
    return 0;
}
```

### LLM 对话模板 (Chat Template)

```cpp
#include "jinja.hpp"

// 加载 tokenizer_config.json 中的 "chat_template" 字符串
std::string chat_template_str = "...";
jinja::Template tpl(chat_template_str);

nlohmann::json messages = nlohmann::json::array({
    {{"role", "user"}, {"content", "你好！"}}
});

// 应用模板
std::string prompt = tpl.apply_chat_template(
    messages,
    true, // add_generation_prompt
    nlohmann::json::array() // tools
);
```

### 自定义函数

你可以注册自定义 C++ 函数，供模板内部调用。

```cpp
tpl.add_function("strftime_now", [](const std::vector<nlohmann::json>& args) {
    // 返回当前时间字符串
    return "2025-12-16";
});
```

## 文档

关于具体的实现细节，请参阅 [doc/implementation_details_CN.md](doc/implementation_details_CN.md)。

## 许可证

Apache License 2.0。 详见 [LICENSE](LICENSE) 文件。
