from dotenv import load_dotenv
from litellm import completion
import os
from rich import print

load_dotenv()

api_key = os.environ["GROQ_API_KEY"]

def get_llm_response(message):
    response = completion(
        model="groq/llama3-8b-8192",
        api_key = api_key,
        messages=message,
        stream=False
    )
    return response

messages = [
    {
        "role":"system",
        "content":"You are a helpful AI assistance"
    },
    {
        "role":"user",
        "content":"When is my next meeting?"
    }
]

print(get_llm_response(messages))