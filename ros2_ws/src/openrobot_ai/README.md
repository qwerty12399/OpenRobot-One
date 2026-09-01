# openrobot_ai

Planned boundary for converting text, and later speech, into whitelisted task
intents. The AI layer may request `search` or `stop`; it must never emit PWM or
raw serial commands. No runtime node or cloud dependency is implemented yet.
