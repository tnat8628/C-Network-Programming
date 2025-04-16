import os
from dotenv import load_dotenv
from smolagents import (
    CodeAgent,
    DuckDuckGoSearchTool,
    LiteLLMModel,
)

load_dotenv()
apikey=os.getenv("DEEPSEEK_API_KEY") 


llm_model = LiteLLMModel(
    model_id="deepseek/deepseek-chat",  # or another Groq model
    api_key=apikey,
    messages=[
        {"role": "system",
         "content": "You are a helpful AI assistant capable of using tools to perform tasks. "
         "When given a query, analyze it and use available tools to gather information. "
         "Return structured responses when possible."}
        ]
)
agent = CodeAgent(tools=[DuckDuckGoSearchTool()], 
                  model=llm_model,
                  verbosity_level=2,
                )
query = """
How long would it take for a puma to cross the united states from florida to california?
"""
agent.run(query)
