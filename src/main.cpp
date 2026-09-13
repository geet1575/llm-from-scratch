#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<cctype>
#include<cassert>

#define IS_LETTER(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define IS_NUMBER(c) ((c >= '0' && c <= '9'))
#define IS_NEWLINE(c) ((c == '\n' || c == '\r'))
#define IS_WHITESPACE(c) ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))

int main(int argc, char* argv[]) {
    std::string input_file = {};
    std::string output_file = {};
    bool arg_print_templated_text = false;
    bool arg_print_split_pretokenized_text = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input-file" && i + 1 < argc) {
            input_file = argv[++i];
        } else if (arg == "--output-file" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--print-templated-text") {
            arg_print_templated_text = true;
        } else if (arg == "--print-split-pretokenized-text") {
            arg_print_split_pretokenized_text = true;
        }
    }

    std::string content;
    if(input_file.empty()){ // if no input file is provided we default to the content being "who are you?"
        content = "who are you?";
    } else {
        std::ifstream infile(input_file);
        if (infile.is_open()) {
            std::getline(infile, content, '\0'); // read file into content
            infile.close();
        } 
    }
    
    // trim leading and ending whitespace - the jinja file does so too
    auto first = content.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) content.clear();
    else{
        auto last = content.find_last_not_of(" \t\n\r");
        content = content.substr(first, last - first + 1);
    }
    std::string template_prefix = "<|im_start|>user\n";
    std::string template_suffix = "<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
    std::string templated_text = template_prefix + content + template_suffix;

    if(arg_print_templated_text){
        std::ofstream outfile(output_file,std::ios::trunc);
        if(outfile.is_open()){
            outfile << templated_text;
            outfile.close();
        }
    }
 
    struct TokenOrString{
        int token_no {-1};
        std::string text {};
    };
    
    TokenOrString relevant_tokens[] = {{248045, "<|im_start|>"}, {248046, "<|im_end|>"}, 
                                        {248068, "<think>"}, {248069, "</think>"}};

    std::vector<TokenOrString> list_after_extraction = {{-1, templated_text}}; 

    for(auto tok : relevant_tokens){
        std::vector<TokenOrString> list_after_pass = {};
        for(auto e: list_after_extraction){
            if(e.token_no != -1){ // special token
                list_after_pass.push_back(e);
                continue;
            }
            // e is text
            // we want to find all occurrences of tok.text in e.text
            size_t start = 0; 
            size_t pos = 0;
            while(true){
                pos = e.text.find(tok.text, start);
                if(pos == std::string::npos){
                    if(start < e.text.size()) list_after_pass.push_back({-1, e.text.substr(start)}); // adding the rest - if there's something there
                    break;
                }
                if(pos > start) list_after_pass.push_back({-1, e.text.substr(start, pos-start)});
                list_after_pass.push_back(tok);
                start = pos + tok.text.size();
            }
        }
        list_after_extraction = list_after_pass;
    }

    std::vector<TokenOrString> list_after_regex_split = {};
    for(auto tok: list_after_extraction){
        if(tok.token_no != -1){ // special token
            list_after_regex_split.push_back(tok);
            continue;
        }
        size_t start = 0; size_t pos = 0;
        while(start < tok.text.size()){
            // Rule 1 match checked
            // checking for 2 char matching first - 's, 't, 'm, 'd
            auto ch = tok.text.substr(start, 2);

            // for case insensitivity do to_lower on each char
            std::string lc;
            for(auto c : ch) lc += std::tolower(c);
            
            if(lc == "'s" || lc == "'t" || lc == "'m" || lc == "'d"){
                list_after_regex_split.push_back({-1, ch});
                start += ch.size(); 
                continue;
            }

            // 3 char matching - 're, 've , 'll
            ch = tok.text.substr(start,3);
            
            // case insensitivity
            lc.clear();
            for(auto c : ch) lc += std::tolower(c);
            
            if(lc == "'re" || lc == "'ve" || lc == "'ll"){
                list_after_regex_split.push_back({-1, ch});
                start += ch.size(); 
                continue;
            }
            
            // Rule 2 match checked
            ch.clear();
            pos = start;
            
            // checking for possibly one char which isnt a letter, number or newline
            if(!IS_LETTER(tok.text[pos]) && !IS_NUMBER(tok.text[pos]) && !IS_NEWLINE(tok.text[pos])) ch.push_back(tok.text[pos++]);
            
            // at least one letter
            if(pos < tok.text.size() && IS_LETTER(tok.text[pos])){
                ch.push_back(tok.text[pos++]);
                // we take as many letters as possible now
                while(pos < tok.text.size() && IS_LETTER(tok.text[pos])) ch.push_back(tok.text[pos++]);
                list_after_regex_split.push_back({-1, ch});
                start += ch.size();
                continue;
            }

            // Rule 3 match checked
            ch.clear();
            pos = start;

            // checking for single number
            if(IS_NUMBER(tok.text[pos])){
                ch.push_back(tok.text[pos]);
                list_after_regex_split.push_back({-1, ch});
                start += ch.size();
                continue;
            }

            // Rule 4 match checked
            ch.clear();
            pos = start;

            // we check for possibly one space
            if(tok.text[pos] == ' ') ch.push_back(tok.text[pos++]);
            
            // look for at least one char which isnt whitespace letter or number
            if(pos < tok.text.size() && !IS_WHITESPACE(tok.text[pos]) && !IS_LETTER(tok.text[pos]) && !IS_NUMBER(tok.text[pos])){
                ch.push_back(tok.text[pos++]);
                // now we match as many chars which arent whitespace letters or numbers
                while(pos < tok.text.size() && !IS_WHITESPACE(tok.text[pos]) && !IS_LETTER(tok.text[pos]) && !IS_NUMBER(tok.text[pos]))ch.push_back(tok.text[pos++]);
                // now look for as many new lines as possible (0 newlines is fine)
                while(pos < tok.text.size() && IS_NEWLINE(tok.text[pos])) ch.push_back(tok.text[pos++]);

                list_after_regex_split.push_back({-1, ch});
                start += ch.size();
                continue;
            }

            // Rule 5 match checked
            ch.clear();
            pos = start;
            size_t last_seen_newline = tok.text.size() + 1;

            // look for as many whitespace chars as possible - since newline is also whitespace we need to keep track of the last new line we saw
            while(pos < tok.text.size() && IS_WHITESPACE(tok.text[pos])) {
                if(IS_NEWLINE(tok.text[pos])) last_seen_newline = pos;
                ch.push_back(tok.text[pos++]);
            }

            if(last_seen_newline != tok.text.size() + 1){
                ch = tok.text.substr(start, last_seen_newline-start+1); // 
                list_after_regex_split.push_back({-1, ch});
                start += ch.size();
                continue;
            }


            // Rule 6 match checked
            ch.clear();
            pos = start;
            size_t next = pos + 1;

            // one whitespace at least - and cannot be followed by a non whitespace character
            if((pos < tok.text.size() && IS_WHITESPACE(tok.text[pos])) && !(next < tok.text.size() && !IS_WHITESPACE(tok.text[next]))){
                ch.push_back(tok.text[pos++]);
                // now add as many ws chars as possible while ensuring next char is whitespace or doesnt exist
                while(pos < tok.text.size() && IS_WHITESPACE(tok.text[pos])){
                    next = pos + 1;
                    if(next < tok.text.size() && !IS_WHITESPACE(tok.text[next])) break; // next char exists and is not whitespace
                    ch.push_back(tok.text[pos++]);
                }

                list_after_regex_split.push_back({-1, ch});
                start += ch.size();
                continue;
            }

            // Rule 7 match checked
            ch.clear();
            pos = start;

            // at least one whitespace
            if(pos < tok.text.size() && IS_WHITESPACE(tok.text[pos])){
                ch.push_back(tok.text[pos++]);
                // as many as possible
                while(pos < tok.text.size() && IS_WHITESPACE(tok.text[pos])) ch.push_back(tok.text[pos++]);
                
                list_after_regex_split.push_back({-1, ch});
                start += ch.size();
                continue;
            }

            // at least one rule shouldve been matched - or the input has something weird
            assert(false && "No regex rule matched in Split pre-tokenizer");
        }
    }

    if (arg_print_split_pretokenized_text) {
        std::ofstream outfile(output_file,std::ios::trunc);
        if(outfile.is_open()){
            for (const auto& tok : list_after_regex_split) {
                outfile << "---\n" << tok.text<< "\nID: " << tok.token_no << '\n';
            }
            outfile.close();
        }
    }

    return 0;
}
