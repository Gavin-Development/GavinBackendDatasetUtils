//
// Created by scot on 21/03/2022.
//
#include <DataLoader.hpp>


TokenSequence::TokenSequence() = default;

TokenSequence::TokenSequence(std::vector<uid_letter_token> tokens) {
    this->tokens = std::move(tokens);
}

TokenSequence::TokenSequence(const std::string& text) {
    this->tokens.reserve(text.size());
    for (char c : text) {
        this->tokens.push_back(uid_letter_token{.type=TT_CHAR, .letter=c});
    }
}

TokenSequence::~TokenSequence() {
    tokens.clear();
}

std::size_t TokenSequence::find(const char &c) {
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type == TT_CHAR && tokens[i].letter == c) {
            return i;
        }
    }
    return tokens.size();
}


void TokenSequence::replace_with_uid(TokenSequence &replace_chars, int uid) {
    std::size_t first_letter_pos = -1;
    std::size_t last_letter_pos = -1;
    while (((first_letter_pos = this->find(replace_chars[0].letter)) != this->tokens.size()) &&
           ((last_letter_pos = this->find(replace_chars[replace_chars.size() - 1].letter)) != this->tokens.size())) {
        if (first_letter_pos != last_letter_pos && first_letter_pos >= 0
            && last_letter_pos >= 0) {
            auto length = last_letter_pos - first_letter_pos + 1;
            auto token = uid_letter_token{.type=TT_UID, .uid=uid};
            for (std::size_t i = 0; i < length; i++) {
                this->tokens.erase(this->tokens.begin() + (int)first_letter_pos);
            }
        }
    }
}

