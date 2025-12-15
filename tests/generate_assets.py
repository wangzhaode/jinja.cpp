import os
import json
import datetime
import shutil

try:
    from modelscope import AutoTokenizer
except ImportError:
    from transformers import AutoTokenizer

OUTPUT_FILE = "test_chat_template.json"

MODELS = [
    "Qwen/Qwen2.5-3B-Instruct",
    "Qwen/Qwen2.5-VL-3B-Instruct",
    "Qwen/Qwen2.5-Omni-3B",
    "Qwen/Qwen2.5-7B-Instruct-1M",
    "Qwen/Qwen2.5-Math-7B-Instruct",
    # "Qwen/Qwen2.5-Coder-7B-Instruct",
    "Qwen/QwQ-32B",
    "Qwen/Qwen3-4B",
    "Qwen/Qwen3-4B-Instruct-2507",
    "Qwen/Qwen3-4B-Thinking-2507",
    "Qwen/Qwen3-VL-4B-Instruct",
    "Qwen/Qwen3-VL-4B-Thinking",
    "Qwen/Qwen3Guard-Gen-4B",
    # "Qwen/Qwen3Guard-Stream-4B",
    "Qwen/Qwen3-Coder-30B-A3B-Instruct",
    "Qwen/Qwen3-Omni-30B-A3B-Instruct",
    "Qwen/Qwen3-Omni-30B-A3B-Thinking",
    "deepseek-ai/DeepSeek-R1-Distill-Qwen-7B",
    "deepseek-ai/DeepSeek-V3.2",
    "deepseek-ai/DeepSeek-R1",
    "ZhipuAI/GLM-4.5V",
    "ZhipuAI/GLM-4.6V",
    "01ai/Yi-VL-6B",
    "01ai/Yi-1.5-6B-Chat",
    "HuggingFaceTB/SmolLM-135M-Instruct",
    "HuggingFaceTB/SmolVLM-256M-Instruct",
    "HuggingFaceTB/SmolLM2-135M-Instruct",
    # "HuggingFaceTB/SmolVLM2-256M-Video-Instruct",
    "HuggingFaceTB/SmolLM3-3B",
    "google/gemma-3-4b-it",
    "google/gemma-3n-E4B-it",
    "mistralai/Ministral-3-3B-Instruct-2512",
    "LLM-Research/llama-2-7b",
    "LLM-Research/Meta-Llama-3-8B-Instruct",
    "LLM-Research/Llama-3.2-3B-Instruct",
    "LLM-Research/Phi-3.5-mini-instruct",
    "LLM-Research/Phi-3.5-vision-instruct",
    "LLM-Research/phi-4",
    "LLM-Research/Phi-4-mini-reasoning",
    "LLM-Research/MobileLLM-125M"
]

def get_scenarios():
    current_date = datetime.datetime.now().strftime("%Y-%m-%d")

    sample_tools = [
        {
            "type": "function",
            "function": {
                "name": "get_weather",
                "description": "Get current weather",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "location": {"type": "string"},
                        "unit": {"type": "string", "enum": ["c", "f"]}
                    },
                    "required": ["location"]
                }
            }
        },
        {
            "type": "function",
            "function": {
                "name": "search",
                "description": "Search web",
                "parameters": {
                    "type": "object",
                    "properties": {"query": {"type": "string"}},
                    "required": ["query"]
                }
            }
        }
    ]

    return [
        {
            "desc": "basic_user",
            "messages": [{"role": "user", "content": "Hi"}]
        },
        {
            "desc": "system_user_assistant",
            "messages": [
                {"role": "system", "content": "You are a helper."},
                {"role": "user", "content": "Who are you?"},
                {"role": "assistant", "content": "I am AI."}
            ]
        },
        {
            "desc": "consecutive_users",
            "messages": [
                {"role": "user", "content": "Part 1"},
                {"role": "user", "content": "Part 2"}
            ]
        },
        {
            "desc": "gen_prompt_true",
            "messages": [{"role": "user", "content": "Hello"}],
            "add_generation_prompt": True
        },
        {
            "desc": "gen_prompt_false",
            "messages": [{"role": "user", "content": "Hello"}],
            "add_generation_prompt": False
        },
        {
            "desc": "disable_thinking",
            "messages": [
                {
                    "role": "user",
                    "content": "Hello"
                }
            ],
            "extra_context": {
                "enable_thinking": False
            },
            "add_generation_prompt": True,
            "expected": "<|im_start|>user\nHello<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n"
        },
        {
            "desc": "tools_provided_no_call",
            "messages": [{"role": "user", "content": "Hi"}],
            "tools": sample_tools,
            "add_generation_prompt": True
        },
        {
            "desc": "assistant_tool_call_history",
            "messages": [
                {"role": "user", "content": "Weather in NY?"},
                {
                    "role": "assistant",
                    "tool_calls": [
                        {
                            "id": "call_123",
                            "type": "function",
                            "function": {"name": "get_weather", "arguments": "{\"location\":\"NY\"}"}
                        }
                    ]
                }
            ],
            "tools": sample_tools,
            "add_generation_prompt": False
        },
        {
            "desc": "tool_response_execution",
            "messages": [
                {"role": "user", "content": "Weather in NY?"},
                {
                    "role": "assistant",
                    "content": "",
                    "tool_calls": [
                        {
                            "id": "call_123",
                            "type": "function",
                            "function": {"name": "get_weather", "arguments": "{\"location\":\"NY\"}"}
                        }
                    ]
                },
                {
                    "role": "tool",
                    "tool_call_id": "call_123",
                    "name": "get_weather",
                    "content": "{\"temp\": 20}"
                }
            ],
            "tools": sample_tools,
            "add_generation_prompt": True
        },
        {
            "desc": "parallel_tool_calls",
            "messages": [
                {"role": "user", "content": "Weather in NY and SF?"},
                {
                    "role": "assistant",
                    "tool_calls": [
                        {
                            "id": "call_1",
                            "type": "function",
                            "function": {"name": "get_weather", "arguments": "{\"location\":\"NY\"}"}
                        },
                        {
                            "id": "call_2",
                            "type": "function",
                            "function": {"name": "get_weather", "arguments": "{\"location\":\"SF\"}"}
                        }
                    ]
                },
                {
                    "role": "tool",
                    "tool_call_id": "call_1",
                    "name": "get_weather",
                    "content": "20C"
                },
                {
                    "role": "tool",
                    "tool_call_id": "call_2",
                    "name": "get_weather",
                    "content": "15C"
                }
            ],
            "tools": sample_tools,
            "add_generation_prompt": True
        },
        {
            "desc": "reasoning_content",
            "messages": [
                {"role": "user", "content": "Solve"},
                {
                    "role": "assistant",
                    "content": "42",
                    "reasoning_content": "Thinking process..."
                }
            ]
        },
        {
            "desc": "date_injection_sim",
            "messages": [
                {"role": "system", "content": f"Current Date: {current_date}"},
                {"role": "user", "content": "Date?"}
            ]
        }
    ]

def main():
    if os.path.dirname(OUTPUT_FILE):
        os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)

    all_models_data = {}
    scenarios = get_scenarios()

    for model_id in MODELS:
        print(f"Processing: {model_id}")

        try:
            tokenizer = AutoTokenizer.from_pretrained(model_id, trust_remote_code=True)
        except Exception as e:
            print(f"  Failed to load {model_id}: {e}")
            continue

        template = tokenizer.chat_template
        if not template and hasattr(tokenizer, "default_chat_template"):
            template = tokenizer.default_chat_template

        if not template:
            print(f"  No template found for {model_id}")
            continue

        special_tokens = {
            "bos_token": tokenizer.bos_token if tokenizer.bos_token else "",
            "eos_token": tokenizer.eos_token if tokenizer.eos_token else "",
            "unk_token": tokenizer.unk_token if tokenizer.unk_token else "",
            "pad_token": tokenizer.pad_token if tokenizer.pad_token else "",
        }

        model_entry = {
            "template": template,
            "special_tokens": special_tokens,
            "cases": []
        }

        for sc in scenarios:
            try:
                expected = tokenizer.apply_chat_template(
                    sc["messages"],
                    tools=sc.get("tools"),
                    tokenize=False,
                    add_generation_prompt=sc.get("add_generation_prompt", False)
                )

                case_data = {
                    "description": sc["desc"],
                    "messages": sc["messages"],
                    "add_generation_prompt": sc.get("add_generation_prompt", False),
                    "expected": expected
                }

                if "tools" in sc:
                    case_data["tools"] = sc["tools"]

                model_entry["cases"].append(case_data)
            except Exception:
                pass

        all_models_data[model_id] = model_entry

    with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
        json.dump(all_models_data, f, indent=2, ensure_ascii=False)

    print(f"\nDone. Saved to {OUTPUT_FILE}")

if __name__ == "__main__":
    main()