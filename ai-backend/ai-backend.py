import openai
# from tiktoken import Tokenizer

# should be replaced by your API key
openai.api_key = ""

messages = [
    {"role": "system", "content": "What can you do as an AI."},
]

# tokenizer = Tokenizer()
# def count_tokens(message):
#     # 使用tokenizer计算令牌数
#     tokens = tokenizer.count_tokens(message)
#     return tokens

while True:
    message = input("User : ")
    if message:
        messages.append(
            {"role": "user", "content": message},
        )
        chat = openai.ChatCompletion.create(
            model="gpt-4", messages=messages
        )
    
    reply = chat.choices[0].message.content
    print(f"ChatGPT: {reply}")
    messages.append({"role": "assistant", "content": reply})