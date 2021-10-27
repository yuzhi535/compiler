/**
 * @file main.cc
 * @author Zhou YuXi (zhouyuxi.abc@gmail.com)
 * @brief lexical analyzer
 * @version 0.1
 * @date 2021-10-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <regex>

using std::ifstream;
using std::regex;
using std::regex_match;
using std::set;
using std::string;
using std::vector;

set<std::string> id;
set<std::string> literal;
set<std::string> keyword;
set<std::string> opt;
set<std::string> sep;
set<std::string> num;

bool isOperator(const std::string &str)
{
    const vector<std::string> operators{"<", ">", "<=", ">=", "!=", "*", "+", "-", "/", "="};
    for (const auto &op : operators)
        if (op == str)
            return true;
    return false;
}

// ,、;、:,{、}、[、]、(、)
bool isSeparator(const std::string &str)
{
    const vector<std::string> Separators{",", ";", ":", "{", "}", "[", "]", "(", ")"};
    for (const auto &separate : Separators)
        if (separate == str)
            return true;
    return false;
}

// [1-9][0-9]*|[0-9]\.[0-9]*
bool isDigit(const string &str)
{
    return regex_match(str, regex("[1-9][0-9]*|[0-9](\\.[0-9]*)?"));
}

bool isKeyword(const string &str)
{
    const vector<string> keywords{"main", "if", "then", "else", "while", "do", "repeat",
                                  "until", "for", "from", "to", "step", "switch", "of",
                                  "case", "default", "return", "integer", "real", "char",
                                  "bool", "and", "or", "not", "mod", "read", "write"};
    for (const auto &i : keywords)
        if (i == str)
            return true;
    return false;
}

// [a-zA-Z_][a-zA-Z0-9_]*
// [a-zA-Z][a-zA-Z0-9]*
bool isID(const std::string &str)
{
    return regex_match(str, regex("[a-zA-Z][a-zA-Z0-9]*"));
}

bool isComment(const std::string &str)
{
    return str == "/*" || str == "//";
}

bool isString(const std::string &str)
{
    return str[0] == '"' && str[str.size() - 1] == '"';
}

bool isBool(const std::string &str)
{
    return str == "true" || str == "false";
}

bool isLiteral(const std::string &str)
{
    return isString(str) || isBool(str);
}

bool isNotLegal(const std::string &str)
{
    return str == " " || str == "\n";
}

void printRoleOfToken(const std::string &token)
{
    if (isOperator(token))
    {
        opt.insert(token.c_str());
    }
    else if (isSeparator(token))
    {
        sep.insert(token);
        // std::cout << "(separator, " << token << ")" << std::endl;
    }
    else if (isKeyword(token))
    {
        keyword.insert(token);
    }
    else if (isLiteral(token))
    {
        literal.insert(token);
        // std::cout << "(literal, " << token << ")" << std::endl;
    }
    else if (isDigit(token))
    {
        num.insert(token);
    }
    else if (isID(token))
    {
        id.insert(token);
        // std::cout << "(identifier, " << token << ")" << std::endl;
    }
    else if (isComment(token))
    {

        // std::cout << "(comment, " << token << ")" << std::endl;
    }
    else
        throw std::runtime_error("Invalid token: " + token);
}

int main(int argc, char const *argv[])
{
    // 预先准备
    string filename;
    if (argc > 1)
        filename = argv[1];
    else
    {
        std::cerr << "you should add a path where the file is from!!!";
        return -1;
    }

    ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "cannot open the file!!!";
        return -1;
    }

    char ch;
    string buffer;

    bool miltiCm = false, singleCm = false; // 单行多行注释判断
    while (file >> std::noskipws >> ch)
    {
        if (singleCm || miltiCm)
        {
            if (singleCm && ch == '\n')
                singleCm = false;

            if (miltiCm && ch == '*')
            {
                file >> ch;
                if (ch == EOF)
                    break;

                if (ch == '/')
                    miltiCm = false;
            }
            continue;
        }

        if (ch == '/')
        {
            std::string comm(1, ch);
            file >> ch;
            if (ch == EOF)
            {
                printRoleOfToken(comm);
                break;
            }

            if (ch == '*')
            {
                miltiCm = true;
                comm += ch;
            }
            else if (ch == '/')
            {
                singleCm = true;
                comm += ch;
            }
            if (miltiCm || singleCm)
            {
                printRoleOfToken(comm);
                continue;
            }
        }

        if (isNotLegal(std::string(1, ch)))
        {
            if (!buffer.empty())
            {
                printRoleOfToken(buffer);
                buffer = "";
            }
            continue;
        }

        if (isOperator(std::string(1, ch)) && !isOperator(buffer))
        {
            if (!buffer.empty() && buffer[buffer.size() - 1] != '!') // 唯独!=违反之前的判断并且是运算符
            {
                printRoleOfToken(buffer);
                buffer = "";
            }
        }

        if (!isOperator(std::string(1, ch)) && isOperator(buffer))
        {
            printRoleOfToken(buffer);
            buffer = "";
        }

        if (isSeparator(std::string(1, ch)))
        {
            if (!buffer.empty())
            {
                printRoleOfToken(buffer);
                buffer = "";
            }
            if (isSeparator(std::string(1, ch)))
            {
                printRoleOfToken(std::string(1, ch));
                continue;
            }
        }
        buffer += ch;
    }
    file.close();
    std::cout << "<<<<<<<<<!>>>>>>>>>" << std::endl;
    for (auto i : id)
    {
        std::cout << "id -> " << i << std::endl;
    }

    for (auto i : literal)
    {
        std::cout << "id -> " << i << std::endl;
    }

    for (auto i : keyword)
    {
        std::cout << "keyword -> " << i << std::endl;
    }

    for (auto i : sep)
    {
        std::cout << "seperator -> " << i << std::endl;
    }

    for (auto i : opt)
    {
        std::cout << "operator -> " << i << std::endl;
    }

    for (auto i : num)
    {
        std::cout << "num -> " << i << std::endl;
    }
    return 0;
}
