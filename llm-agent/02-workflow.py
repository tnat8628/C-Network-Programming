from dotenv import load_dotenv
from langchain_litellm import ChatLiteLLM
from langchain.agents import initialize_agent, AgentType
from langchain.tools import tool

import os
from rich import print

load_dotenv()

api_key = os.environ["GROQ_API_KEY"]

llm = ChatLiteLLM(
    model="groq/llama3-8b-8192",
    api_key=api_key,
)

@tool
def dummy_tool(query: str) -> str:
    """A dummy tool that just echoes the input"""
    return f"I received: {query}"



#Initialize the agent
agent = initialize_agent(
    tools=[dummy_tool],
    llm=llm,
    agent=AgentType.ZERO_SHOT_REACT_DESCRIPTION,
    verbose=True
)

# Run the agent with a sample query
print(agent.run("When is my next meeting?"))