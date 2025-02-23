#include "xxcnc/core/gcode/GCodeParser.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>

namespace xxcnc::core::gcode {

// GCodeLexer实现
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

    // 处理注释
    if (current == ';' || current == '(') {
        position_ = input_.length(); // 跳过注释后的所有内容
        return "";
    }

    // 处理字母+数字组合（如G01、X100等）
    if (std::isalpha(current)) {
        token += current;
        ++position_;

        // 读取数字部分
        while (position_ < input_.length() && 
               (std::isdigit(input_[position_]) || input_[position_] == '.' || 
                input_[position_] == '-' || input_[position_] == '+')) {
            token += input_[position_];
            ++position_;
        }
    }
    // 处理纯数字
    else if (std::isdigit(current) || current == '.' || current == '-' || current == '+') {
        while (position_ < input_.length() && 
               (std::isdigit(input_[position_]) || input_[position_] == '.' || 
                input_[position_] == '-' || input_[position_] == '+')) {
            token += input_[position_];
            ++position_;
        }
    }
    else {
        // 处理其他字符
        token += current;
        ++position_;
    }

    return token;
}

bool GCodeLexer::isEnd() const {
    return position_ >= input_.length();
}

// GCodeParser实现
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
    command.lineNumber = -1; // 默认行号

    std::string token;
    bool hasGCode = false;

    while (!(token = lexer_->nextToken()).empty()) {
        // 处理行号
        if (token[0] == 'N' && command.lineNumber == -1) {
            try {
                int lineNum = std::stoi(token.substr(1));
                if (lineNum < 0) {
                    throw ParserError("Negative line number not allowed: " + token);
                }
                command.lineNumber = lineNum;
                continue;
            } catch (const std::exception&) {
                throw ParserError("Invalid line number: " + token);
            }
        }

        // 处理G代码类型
        if (token[0] == 'G' && !hasGCode) {
            command.type = parseGCodeType(token);
            hasGCode = true;
            continue;
        }

        // 处理换刀命令
        if (token[0] == 'T') {
            command.type = GCodeType::TOOL_CHANGE;
            hasGCode = true;
            command.params.push_back(parseParam(token));
            continue;
        }

        // 处理参数
        if (std::isalpha(token[0])) {
            char paramLetter = token[0];
            // 检查是否是有效的参数字母
            if (paramLetter != 'X' && paramLetter != 'Y' && paramLetter != 'Z' && 
                paramLetter != 'I' && paramLetter != 'J' && paramLetter != 'K' && 
                paramLetter != 'F' && paramLetter != 'S' && paramLetter != 'P' && 
                paramLetter != 'T') {
                throw ParserError("Invalid parameter letter: " + std::string(1, paramLetter));
            }
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
            // 记录错误但继续解析
            std::cerr << "Error parsing line: " << e.what() << std::endl;
        }
    }

    return commands;
}

} // namespace xxcnc::core::gcode
