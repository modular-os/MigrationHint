import re
import openai
import requests
import json
import time

AGENT = "http://20.14.22.180:8080"


openai.api_key = "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"

Prompt_0 = "As an experienced code summarizer specializing in the Linux kernel, your task is to process a set of Markdown-formatted texts. Specifically, you are required to replace all occurrences of `<filled-by-ai>` within the section titled \"- **功能描述**\" with professional descriptions of the specific functions. Additionally, you should replace all instances of `<filled-by-ai>` within the section titled \"- **核心逻辑**\" with detailed step-by-step descriptions of the function's logic. Please format each step using Markdown format by prefixing it with \"-\". Note that you do not need to process the tag `<filled-by-CodeAnalysis>`. Each round of chat, you should only process the most recent messages and ignore any previous conversation history. Your responses should be provided in Chinese."

Prompt_1 = "As an experienced code summarizer specializing in the Linux kernel, your task is to read the a set of kernel source code and summarize its description and detail under the markdown-formatted texts. Specifically, you are required to summarize professional descriptions of the specific functions within the section titled \"- **功能描述**\". Additionally, you should also analyze detailed step-by-step descriptions of the function's logic within the section titled \"- **核心逻辑**\". Please format each step using Markdown format by prefixing it with \"-\". Each round of chat, you should only process the most recent messages and ignore any previous conversation history. Your responses should be provided in Chinese."

Prompt_1 = "As an experienced code summarizer specializing in the Linux kernel, your task is to read the a set of kernel source code and summarize its description and detail under the markdown-formatted texts. Specifically, you are required to summarize professional descriptions of the specific functions within the section titled \"- **功能描述**\". Additionally, you should also analyze detailed step-by-step descriptions of the function's logic within the section titled \"- **核心逻辑**\". Please format each step using Markdown format by prefixing it with \"-\". Each round of chat, you should only process the most recent messages and ignore any previous conversation history. Your responses should be provided in Chinese."

Prompt_2 = "As an experienced code summarizer specializing in the Linux kernel, your task is to read the a set of kernel source code and summarize its description. Specifically, you are required to briefly summarize professional descriptions of the specific functions. **Please only output the required description without any other information**. Each round of chat, you should only process the most recent messages and ignore any previous conversation history. Your responses should be provided in Chinese."

messages = [
    {"role": "system", "content": Prompt_2},
]

pattern_basic_ability = re.compile(r'\s\s\s`(.*?)`\n\s+- 类型: `(.*?)`\n\s+- 定义路径: `(.*?)`\n\s+- 简介：`<Filled-By-AI>`\n(?:\s+- 外部类型细节：\n\s+- 外部类型名称: `(.*?)`\n\s+- 位置: `(.*?)`\n\s+- 是否为指针: `(.*?)`\n)?')

def process_func_framework():
    with open('./ksm_framework.md', 'r') as file:
        framework_content = file.read()
    text = framework_content

    # 使用正则表达式拆分文本
    sections = re.split(r"\n\n```c\n", text)
    # 填充每个部分的<filled-by-ai>内容
    filled_sections = []
    cnt = 0
    size = len(sections)
    for section in sections:
        # filled_section = section.replace("<filled-by-ai>", "")
        filled_section = section
        cnt += 1
        print(f"Processing {cnt}/{size}...")
        print(section)

        messages.append(
            {"role": "user", "content": section},
        )
        
        # print(messages)
        chat = openai.ChatCompletion.create(
            model="gpt-4", messages=messages
        )
        
        reply = chat.choices[0].message.content
        # print(reply)

        # 提取生成的文本
        filled_text = chat.choices[0].message.content.strip()
        filled_sections.append(filled_text)
        if (cnt % 5 == 0):
            filled_text = ""
            filled_text = "\n---\n\n\n```c\n".join(filled_sections)
            with open('./result.txt', 'w') as file:
                file.write(filled_text)

            

    # 重新组合文本
    filled_text = "\n---\n\n\n```c\n".join(filled_sections)

    # 打印填充后的文本
    print(filled_text)

    with open('./result.txt', 'w') as file:
        file.write(filled_text)


def query_openai(model, messages, agent_token="8af89643-2e56-41b0-a464-b98cb3753124"):
    headers = {
        "Authorization" : f"Bearer {agent_token}",
        "Content-Type" : "application/json"
    }
    data = {
        "model": model,
        "messages": messages,
    }
    resp = requests.post(AGENT, data=json.dumps(data), headers=headers)
    return resp.text

def process_func_dep_details(original_file_path, post_processed_file):
    with open(original_file_path, 'r') as file:
        file_content = file.read()

    matches = pattern_basic_ability.findall(file_content)

    size = len(matches)
    cnt = 0
    for match in matches:
        cnt += 1
        entry_name = match[0]
        entry_type = match[1]
        entry_path = match[2]
        query_string = f"Name: {entry_name}\nType: {entry_type}\nPath: {entry_path}\n";

        if match[4] != "":
            query_string = f"Name: struct {match[3]}\nType: {entry_type}\nPath: {match[4]}\n";

        print(f"{cnt}/{size}: {query_string}\n")

        messages.append(
            {"role": "user", "content": query_string},
        )

        chat = openai.ChatCompletion.create(
            model="gpt-4", messages=messages
        )
        
        ai_result = chat.choices[0].message.content

        print(ai_result)

        original_text = ""
        new_text = ""

        if match[4] == "":
            original_text = f"   `{match[0]}`\n   - 类型: `{match[1]}`\n   - 定义路径: `{match[2]}`\n   - 简介：`<Filled-By-AI>`\n"

            new_text = f"   `{match[0]}`\n   - 类型: `{match[1]}`\n   - 定义路径: `{match[2]}`\n   - 简介：`{ai_result}`\n"
        else:
            original_text = f"   `{match[0]}`\n   - 类型: `{match[1]}`\n   - 定义路径: `{match[2]}`\n   - 简介：`<Filled-By-AI>`\n   - 外部类型细节：\n      - 外部类型名称: `{match[3]}`\n         - 位置: `{match[4]}`\n         - 是否为指针: `{match[5]}`\n"

            new_text = f"   `{match[0]}`\n   - 类型: `{match[1]}`\n   - 定义路径: `{match[2]}`\n   - 简介：`{ai_result}`\n   - 外部类型细节：\n      - 外部类型名称: `{match[3]}`\n         - 位置: `{match[4]}`\n         - 是否为指针: `{match[5]}`\n"

        # write to post_processed_file: replace original_text with new_text
        print(original_text)
        file_content = file_content.replace(original_text, new_text)

        with open(post_processed_file, 'w', encoding='utf-8') as post_processed:
                post_processed.write(file_content)

        time.sleep(20)


    
if __name__ == "__main__":
    # result = query_openai("gpt-4", messages)
    # print(result)

    # chat = openai.ChatCompletion.create(
    #     model="gpt-4", messages=messages
    # )
        
    # reply = chat.choices[0].message.content
    # print(reply)


    # process_func_dep_details("/home/tz/MigrationHint/log/download/ksm_result.md")
    original_file_path = "/home/tz/MigrationHint/log/download/ksm_result.md"
    new_file_path = "new_ksm_result.md"

    process_func_dep_details(original_file_path, new_file_path)