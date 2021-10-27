#include <fstream>
#include <vector>
#include <string>
#include <iostream>

using std::string;
using std::vector;

/*
    非终结符个数；
    所有非终结符，空格分隔；
    终结符个数；
    所有终结符，空格分隔；
    规则个数；
    所有规则，每行一个规则，规则输入格式：左部，右部符号数，右部符号，空格分隔；
    开始符号。

  非终结符集：
      E T F
  终结符集：
    i +  *  (  )
  产生式集：
      0: E -> E + T
      1: E -> T
      2: E -> T * F
      3: T -> F
      4: F -> ( E )
      5: F -> i
  开始符号：
      E
*/

int main(int argc, char **argv)
{
    string filename;
    if (argc > 1)
        filename = argv[1];
    else
    {
        printf("you should follow a file name to be parsed!!!");
        return -1;
    }
    std::ifstream io(filename);

    int lexemeNum;
    vector<char> lexeme;
    int terminalNum;
    vector<int> terminal;
    int rulesNum;
    vector<string> rules;
    char lookAhead;
    io >> lexemeNum;

    for (int i = 0; i < lexemeNum; ++i)
    {
        char t;
        io >> t;
        lexeme.push_back(t);
    }
    io >> terminalNum;
    for (int i(0); i < terminalNum; ++i)
    {
        char t;
        io >> t;
        terminal.push_back(t);
    }

    io >> rulesNum;

    string c;
    getline(io, c);
    for (int i(0); i < rulesNum; ++i)
    {
        string t;
        getline(io, t);
        rules.push_back(t);
    }
    getline(io, c);
    io >> lookAhead;

    printf("非终结符：\n    ");
    for (auto i : lexeme)
    {
        printf("%c ", i);
    }
    printf("\b \b\n");

    printf("终结符集：\n    ");
    for (auto i : terminal)
    {
        printf("%c ", i);
    }
    printf("\b \b\n");

    printf("产生式集：\n    ");
    for (int i(0); i < rules.size(); ++i)
    {
        printf("%d: %s\n", i, rules[i].c_str());
        if (i < rules.size() - 1)
            printf("    ");
    }

    printf("开始符号：\n    ");
    printf("%c", lookAhead);
    return 0;
}