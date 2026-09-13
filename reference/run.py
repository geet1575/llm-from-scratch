import torch
from transformers import AutoTokenizer, AutoModelForCausalLM
import argparse
from pathlib import Path
from tokenizers import Tokenizer
from tokenizers.pre_tokenizers import PreTokenizer, Sequence


parser = argparse.ArgumentParser()
parser.add_argument("--model-dir", type=Path, help="Load model and tokenizer from this local directory (so that you don't need to access the internet every time)")
parser.add_argument("--input-file", help="Path to the file containing the message")
parser.add_argument("--output-file", help="Path to the file where the response will be saved")
parser.add_argument("--print-templated-text", action="store_true", help="Print the templated text to the output file, so you need to specify the output file path as well, otherwise it will throw an error")
parser.add_argument("--print-split-pretokenized-text", action="store_true", help="Print the text after normalization, split token extraction and the Split pre tokenization. You will need to specify an output file path as well")

args = parser.parse_args()
if args.model_dir is not None:
    args.model_dir = args.model_dir.expanduser().resolve()
    if not args.model_dir.is_dir():
        parser.error(f"--model-dir is not a directory: {args.model_dir}")

model_name = str(args.model_dir) if args.model_dir is not None else "Qwen/Qwen3.5-2B"
local_files_only = args.model_dir is not None

tokenizer = AutoTokenizer.from_pretrained(model_name, local_files_only=local_files_only)
model = AutoModelForCausalLM.from_pretrained(
    model_name,
    torch_dtype="auto",
    device_map="auto",
    local_files_only=local_files_only,
)

if args.input_file:
    with open(args.input_file, "r") as f:
        content = f.read()
else:
    content = "who are you?"


messages = [ {"role": "user", "content": content} ]

text = tokenizer.apply_chat_template(
    messages,
    tokenize=False,
    add_generation_prompt=True,
)

if args.print_templated_text:
    with open(args.output_file, "w") as f:
        f.write(text)
    exit(0)

if args.print_split_pretokenized_text: # split pretokenizer test
    class PrintSplits:
        def __init__(self, outfile):
            self.outfile = outfile
        def pre_tokenize(self, pretok):
            for piece, _, tokens in pretok.get_splits():
                token_id = tokens[0].id if tokens else -1
                print(f"---\n{piece}\nID: {token_id}", file=self.outfile)

    qwenlike_tokenizer = Tokenizer.from_str(tokenizer.backend_tokenizer.to_str())
    with open(args.output_file, "w") as outfile:
        qwenlike_tokenizer.pre_tokenizer = Sequence([qwenlike_tokenizer.pre_tokenizer[0],
                                                    PreTokenizer.custom(PrintSplits(outfile))])
        qwenlike_tokenizer.encode(text, add_special_tokens=False)
    exit(0)

inputs = tokenizer(text, return_tensors="pt").to(model.device)

with torch.no_grad():
    output_ids = model.generate(
        **inputs,
        max_new_tokens=64,
    )

new_tokens = output_ids[0][inputs["input_ids"].shape[1]:]
response = tokenizer.decode(new_tokens, skip_special_tokens=True)

print(response)
