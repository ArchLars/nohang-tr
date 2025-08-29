#!/usr/bin/env bash
set -euo pipefail

# Setup a local dev helper for OpenAI coding assistance.
# Requirements: python3, venv. Set OPENAI_API_KEY in your environment.

if ! command -v python3 >/dev/null; then
  echo "python3 is required" >&2
  exit 1
fi

python3 -m venv .venv
. .venv/bin/activate
pip install --upgrade pip
pip install openai

# Create a simple .env template
cat > .env <<'EOF'
# Export before running maintain_codex.sh
# export OPENAI_API_KEY=sk-...
# Optional: export OPENAI_BASE_URL=https://api.openai.com/v1
EOF

echo "OK. Activate with: source .venv/bin/activate"
echo "Set OPENAI_API_KEY, then run scripts/maintain_codex.sh 'your prompt'"
