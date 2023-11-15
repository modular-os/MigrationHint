import re
import openai

openai.api_key = ""

with open('./ksm_framework.md', 'r') as file:
    framework_content = file.read()

text = framework_content

# 使用正则表达式拆分文本
sections = re.split(r"\n\n```c\n", text)

messages = [
    {"role": "system", "content": "As an experienced code summarizer specializing in the Linux kernel, your task is to process a set of Markdown-formatted texts. Specifically, you are required to replace all occurrences of `<filled-by-ai>` within the section titled \"- **功能描述**\" with professional descriptions of the specific functions. Additionally, you should replace all instances of `<filled-by-ai>` within the section titled \"- **核心逻辑**\" with detailed step-by-step descriptions of the function's logic. Please format each step using Markdown format by prefixing it with \"-\". Note that you do not need to process the tag `<filled-by-CodeAnalysis>`. Each round of chat, you should only process the most recent messages and ignore any previous conversation history. Your responses should be provided in Chinese."},
]

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

# from tiktoken import Tokenizer

# tokenizer = Tokenizer()
# def count_tokens(message):
#     # 使用tokenizer计算令牌数
#     tokens = tokenizer.count_tokens(message)
#     return tokens