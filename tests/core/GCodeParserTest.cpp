#include <gtest/gtest.h>
#include "xxcnc/core/gcode/GCodeParser.h"

namespace xxcnc::core::gcode::test {

class GCodeParserTest : public ::testing::Test {
protected:
    GCodeParser parser;
};

// 词法分析器测试
TEST_F(GCodeParserTest, LexerBasicTokenization) {
    GCodeLexer lexer;
    
    // 测试基本G代码指令
    lexer.setInput("G01 X100 Y200");
    EXPECT_EQ(lexer.nextToken(), "G01");
    EXPECT_EQ(lexer.nextToken(), "X100");
    EXPECT_EQ(lexer.nextToken(), "Y200");
    EXPECT_TRUE(lexer.nextToken().empty());

    // 测试带注释的代码
    lexer.setInput("G00 X0 ; Move to origin");
    EXPECT_EQ(lexer.nextToken(), "G00");
    EXPECT_EQ(lexer.nextToken(), "X0");
    EXPECT_TRUE(lexer.nextToken().empty());

    // 测试带括号注释的代码
    lexer.setInput("G01 X100 (Linear move) Y200");
    EXPECT_EQ(lexer.nextToken(), "G01");
    EXPECT_EQ(lexer.nextToken(), "X100");
    EXPECT_TRUE(lexer.nextToken().empty());
}

// 语法分析器测试
TEST_F(GCodeParserTest, ParseBasicGCodes) {
    // 测试快速定位指令
    auto cmd = parser.parseLine("G00 X100 Y200 Z50");
    EXPECT_EQ(cmd.type, GCodeType::RAPID_MOVE);
    EXPECT_EQ(cmd.params.size(), 3);
    
    // 验证参数
    auto findParam = [&](char letter) -> double {
        auto it = std::find_if(cmd.params.begin(), cmd.params.end(),
            [letter](const GCodeParam& p) { return p.letter == letter; });
        return it != cmd.params.end() ? it->value : 0.0;
    };
    
    EXPECT_DOUBLE_EQ(findParam('X'), 100.0);
    EXPECT_DOUBLE_EQ(findParam('Y'), 200.0);
    EXPECT_DOUBLE_EQ(findParam('Z'), 50.0);
}

// 测试直线移动指令
TEST_F(GCodeParserTest, ParseLinearMove) {
    auto cmd = parser.parseLine("G01 X100 Y200 F1000");
    EXPECT_EQ(cmd.type, GCodeType::LINEAR_MOVE);
    EXPECT_EQ(cmd.params.size(), 3);
    
    auto findParam = [&](char letter) -> double {
        auto it = std::find_if(cmd.params.begin(), cmd.params.end(),
            [letter](const GCodeParam& p) { return p.letter == letter; });
        return it != cmd.params.end() ? it->value : 0.0;
    };
    
    EXPECT_DOUBLE_EQ(findParam('X'), 100.0);
    EXPECT_DOUBLE_EQ(findParam('Y'), 200.0);
    EXPECT_DOUBLE_EQ(findParam('F'), 1000.0);
}

// 测试圆弧移动指令
TEST_F(GCodeParserTest, ParseArcMove) {
    auto cmd = parser.parseLine("G02 X100 Y100 I50 J50 F500");
    EXPECT_EQ(cmd.type, GCodeType::CW_ARC);
    EXPECT_EQ(cmd.params.size(), 5);
    
    auto findParam = [&](char letter) -> double {
        auto it = std::find_if(cmd.params.begin(), cmd.params.end(),
            [letter](const GCodeParam& p) { return p.letter == letter; });
        return it != cmd.params.end() ? it->value : 0.0;
    };
    
    EXPECT_DOUBLE_EQ(findParam('X'), 100.0);
    EXPECT_DOUBLE_EQ(findParam('Y'), 100.0);
    EXPECT_DOUBLE_EQ(findParam('I'), 50.0);
    EXPECT_DOUBLE_EQ(findParam('J'), 50.0);
    EXPECT_DOUBLE_EQ(findParam('F'), 500.0);
}

// 测试带行号的指令
TEST_F(GCodeParserTest, ParseWithLineNumbers) {
    auto cmd = parser.parseLine("N10 G01 X100 Y200");
    EXPECT_EQ(cmd.lineNumber, 10);
    EXPECT_EQ(cmd.type, GCodeType::LINEAR_MOVE);
}

// 测试换刀指令
TEST_F(GCodeParserTest, ParseToolChange) {
    auto cmd = parser.parseLine("T1");
    EXPECT_EQ(cmd.type, GCodeType::TOOL_CHANGE);
    EXPECT_EQ(cmd.params.size(), 1);
    EXPECT_DOUBLE_EQ(cmd.params[0].value, 1.0);
}

// 错误处理测试
TEST_F(GCodeParserTest, ErrorHandling) {
    // 测试无效的G代码
    EXPECT_THROW(parser.parseLine("G99"), ParserError);
    
    // 测试无效的参数
    EXPECT_THROW(parser.parseLine("G01 X100 Q200"), ParserError);
    
    // 测试缺少参数值
    EXPECT_THROW(parser.parseLine("G01 X"), ParserError);
    
    // 测试无效的行号
    EXPECT_THROW(parser.parseLine("N-1 G01 X100"), ParserError);
}

} // namespace xxcnc::core::gcode::test
