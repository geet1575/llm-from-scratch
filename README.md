# Qwen 3.5 From Scratch

This repo contains my code for replicating Qwen 3.5-2B from scratch. 
I've documented the process here - 

## Instructions for cloning the repo and downloading the model weights and files
Run the following block: 
```
git clone https://github.com/geet1575/llm-from-scratch.git
cd llm-from-scratch/
pip install -U huggingface_hub

hf download Qwen/Qwen3.5-2B \
  --local-dir ./.cache/Qwen3.5-2B
```

## Instructions for running the template test (test 1)
Assuming you are in the `llm-from-scratch` directory and you've downloaded the model files to `./.cache/Qwen3.5-2B`, run
```
bash tests/test_01_templated_text.sh ./.cache/Qwen3.5-2B
```
If you've downloaded the model files elsewhere, run
```
bash tests/test_01_templated_text.sh <path to model files>
```

## Instructions for running the split pretokenizer test (test 2)
Assuming you are in the `llm-from-scratch` directory and you've downloaded the model files to `./.cache/Qwen3.5-2B`, run
```
bash tests/test_02_split_pretokenized_text.sh ./.cache/Qwen3.5-2B
```
If you've downloaded the model files elsewhere, run
```
bash tests/test_02_split_pretokenized_text.sh <path to model files>
```

## Instructions for running the files manually
Assuming you are in the `llm-from-scratch` directory run `python3 reference/run.py --help` to see all the arguments. 
The arguments and their semantics are the same for the `main.cpp` file (except for `--help`), so you will need to run `make` and then `./build/main (arguments)`