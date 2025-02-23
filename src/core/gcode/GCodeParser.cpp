#include "xxcnc/core/gcode/GCodeParser.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>

namespace xxcnc {
namespace core {
namespace gcode {

// GCodeLexer implementation
void GCodeLexer::setInput(const std::string& input) {
    input_ = input;
    position_ = 0;
}

void GCodeLexer::skipWhitespace() {
    while (position_ < input_.length() && std::isspace(input_[position_])) {
        ++position_;
    }
}

std::string GCodeLexer::nextToken() {
    skipWhitespace();
    
    if (position_ >= input_.length()) {
        return "";
    }

    std::string token;
    char current = input_[position_];

    // Handle comments
    if (current == ';' || current == '(') {
        position_ = input_.length(); // Skip all content after comment
        return "";
    }

    // Handle letter+number combinations (like G01, X100, etc.)
    if (std::isalpha(current)) {
        token += current;
        ++position_;

        // Read numeric part
        while (position_ < input_.length() && 
               (std::isdigit(input_[position_]) || input_[position_] == '.' || 
                input_[position_] == '-' || input_[position_] == '+')) {
            token += input_[position_];
            ++position_;
        }
    }
    // Handle pure numbers
    else if (std::isdigit(current) || current == '.' || current == '-' || current == '+') {
        while (position_ < input_.length() && 
               (std::isdigit(input_[position_]) || input_[position_] == '.' || 
                input_[position_] == '-' || input_[position_] == '+')) {
            token += input_[position_];
            ++position_;
        }
    }
    else {
        // Handle other characters
        token += current;
        ++position_;
    }

    return token;
}

bool GCodeLexer::isEnd() const {
    return position_ >= input_.length();
}

// GCodeParser implementation
GCodeParser::GCodeParser() : lexer_(std::make_unique<GCodeLexer>()) {}

GCodeType GCodeParser::parseGCodeType(const std::string& token) const {
    if (token.empty() || token[0] != 'G') {
        throw ParserError("Invalid G-code type: " + token);
    }

    try {
        int code = std::stoi(token.substr(1));
        switch (code) {
            case 0: return GCodeType::RAPID_MOVE;
            case 1: return GCodeType::LINEAR_MOVE;
            case 2: return GCodeType::CW_ARC;
            case 3: return GCodeType::CCW_ARC;
            case 4: return GCodeType::DWELL;
            case 28: return GCodeType::HOME;
            default:
                throw ParserError("Unsupported G-code: " + token);
        }
    } catch (const std::exception&) {
        throw ParserError("Failed to parse G-code type: " + token);
    }
}

GCodeParam GCodeParser::parseParam(const std::string& token) const {
    if (token.empty() || !std::isalpha(token[0])) {
        throw ParserError("Invalid parameter format: " + token);
    }

    GCodeParam param;
    param.letter = token[0];
    
    try {
        param.value = std::stod(token.substr(1));
    } catch (const std::exception&) {
        throw ParserError("Failed to parse parameter value: " + token);
    }

    return param;
}

GCodeCommand GCodeParser::parseLine(const std::string& line) {
    lexer_->setInput(line);
    GCodeCommand command;
    command.lineNumber = -1; // Default line number

    std::string token;
    bool hasGCode = false;

    while (!(token = lexer_->nextToken()).empty()) {
        // Handle line number
        if (token[0] == 'N' && command.lineNumber == -1) {
            try {
                command.lineNumber = std::stoi(token.substr(1));
                continue;
            } catch (const std::exception&) {
                throw ParserError("Invalid line number: " + token);
            }
        }

        // Handle G-code type
        if (token[0] == 'G' && !hasGCode) {
            command.type = parseGCodeType(token);
            hasGCode = true;
            continue;
        }

        // Handle tool change command
        if (token[0] == 'T') {
            command.type = GCodeType::TOOL_CHANGE;
            hasGCode = true;
            command.params.push_back(parseParam(token));
            continue;
        }

        // Handle parameters
        if (std::isalpha(token[0])) {
            command.params.push_back(parseParam(token));
        }
    }

    if (!hasGCode && command.params.empty()) {
        throw ParserError("Empty or invalid G-code line");
    }

    return command;
}

std::vector<GCodeCommand> GCodeParser::parseFile(const std::string& filename) {
    std::vector<GCodeCommand> commands;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw ParserError("Failed to open file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        try {
            commands.push_back(parseLine(line));
        } catch (const ParserError& e) {
            // Log error but continue parsing
            std::cerr << "Error parsing line: " << e.what() << std::endl;
        }
    }

    return commands;
}

} // namespace gcode
} // namespace core
} // namespace xxcnc
