#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace xxcnc::core::gcode {

// G代码指令类型
enum class GCodeType {
    RAPID_MOVE = 0,        // G00
    LINEAR_MOVE = 1,       // G01
    CW_ARC = 2,           // G02
    CCW_ARC = 3,          // G03
    DWELL = 4,            // G04
    HOME = 28,            // G28
    TOOL_CHANGE = 6       // T
};

// G代码参数
struct GCodeParam {
    char letter;           // 参数字母 (X, Y, Z, F等)
    double value;         // 参数值
    
    GCodeParam(char l = '\0', double v = 0.0) : letter(l), value(v) {}
};

// G代码命令
struct GCodeCommand {
    GCodeType type;                    // 命令类型
    std::vector<GCodeParam> params;    // 参数列表
    int lineNumber;                    // 行号
    
    GCodeCommand() : type(GCodeType::RAPID_MOVE), lineNumber(0) {}
};

// 词法分析器错误
class LexerError : public std::runtime_error {
public:
    explicit LexerError(const std::string& message) : std::runtime_error(message) {}
};

// 解析器错误
class ParserError : public std::runtime_error {
public:
    explicit ParserError(const std::string& message) : std::runtime_error(message) {}
};

// G代码词法分析器
class GCodeLexer {
public:
    GCodeLexer() = default;
    ~GCodeLexer() = default;
    
    // 对输入进行词法分析
    std::vector<std::string> tokenize(const std::string& line);
    
    // 设置输入文本
    void setInput(const std::string& input);
    
    // 获取下一个标记
    std::string nextToken();
    
    // 检查是否到达结尾
    bool isEnd() const;
    
private:
    std::string input_;
    size_t position_ = 0;
    
    // 跳过空白字符
    void skipWhitespace();
    
    // 辅助函数
    bool isWhitespace(char c) const;
    bool isDigit(char c) const;
    bool isLetter(char c) const;
};

// G代码解析器
class GCodeParser {
public:
    GCodeParser();
    ~GCodeParser() = default;
    
    // 解析单行G代码
    GCodeCommand parseLine(const std::string& line);
    
    // 解析G代码文件
    std::vector<GCodeCommand> parseFile(const std::string& filename);
    
private:
    std::unique_ptr<GCodeLexer> lexer_;
    
    // 解析参数
    GCodeParam parseParam(const std::string& token) const;
    
    // 解析G代码类型
    GCodeType parseGCodeType(const std::string& token) const;
};

} // namespace xxcnc::core::gcode