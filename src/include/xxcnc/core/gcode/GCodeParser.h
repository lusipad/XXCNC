#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace xxcnc {
namespace core {
namespace gcode {

// G code instruction type
enum class GCodeType {
    RAPID_MOVE = 0,        // G00
    LINEAR_MOVE = 1,       // G01
    CW_ARC = 2,           // G02
    CCW_ARC = 3,          // G03
    DWELL = 4,            // G04
    HOME = 28,            // G28
    TOOL_CHANGE = 6       // T
};

// G code parameter
struct GCodeParam {
    char letter;           // Parameter letter (X, Y, Z, F, etc.)
    double value;         // Parameter value
    
    GCodeParam(char l = '\0', double v = 0.0) : letter(l), value(v) {}
};

// G code command
struct GCodeCommand {
    GCodeType type;                    // Command type
    std::vector<GCodeParam> params;    // Parameter list
    int lineNumber;                    // Line number
    
    GCodeCommand() : type(GCodeType::RAPID_MOVE), lineNumber(0) {}
};

// Lexer error
class LexerError : public std::runtime_error {
public:
    explicit LexerError(const std::string& message) : std::runtime_error(message) {}
};

// Parser error
class ParserError : public std::runtime_error {
public:
    explicit ParserError(const std::string& message) : std::runtime_error(message) {}
};

// G code lexer
class GCodeLexer {
public:
    GCodeLexer() = default;
    ~GCodeLexer() = default;
    
    // Tokenize input
    std::vector<std::string> tokenize(const std::string& line);
    
    // Set input text
    void setInput(const std::string& input);
    
    // Get next token
    std::string nextToken();
    
    // Check if end is reached
    bool isEnd() const;
    
private:
    std::string input_;
    size_t position_ = 0;
    
    // Skip whitespace characters
    void skipWhitespace();
    
    // Helper functions
    bool isWhitespace(char c) const;
    bool isDigit(char c) const;
    bool isLetter(char c) const;
};

// G code parser
class GCodeParser {
public:
    GCodeParser();
    ~GCodeParser() = default;
    
    // Parse single line of G code
    GCodeCommand parseLine(const std::string& line);
    
    // Parse G code file
    std::vector<GCodeCommand> parseFile(const std::string& filename);
    
private:
    std::unique_ptr<GCodeLexer> lexer_;
    
    // Parse parameter
    GCodeParam parseParam(const std::string& token) const;
    
    // Parse G code type
    GCodeType parseGCodeType(const std::string& token) const;
};

} // namespace gcode
} // namespace core
} // namespace xxcnc