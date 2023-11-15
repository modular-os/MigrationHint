#!/bin/bash

# Activate virtual environment
# conda activate openai

# Testing
OPENAI_API_KEY=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 \
curl https://api.openai.com/v1/chat/completions \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $OPENAI_API_KEY" \
  -d '{
     "model": "gpt-3.5-turbo",
     "messages": [{"role": "user", "content": "Say this is a test!"}],
     "temperature": 0.7
   }'
