/**
 * @file main.cpp
 * @author Zhou YuXi (周誉喜) (zhouyuxi.abc@gmail.com)
 * @brief 实现First集、Follow集和Select集的计算
 * @version 0.1
 * @date 2021-10-28
 * @note c++11
 * @copyright Copyright (c) 2021
 * 
 */

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <stack>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>

using std::cin;
using std::cout;
using std::fstream;
using std::ifstream;
using std::make_pair;
using std::map;
using std::multimap;
using std::ostream;
using std::set;
using std::string;
using std::vector;

using NONTERMINAL = set<string>;
using TERMINAL = set<string>;
using RULE = multimap<string, vector<string>>;
using INITIAL = string;
using FIRSTSET = map<string, set<string>>;
using FOLLOWSET = map<string, set<string>>;
using SELLECTSET = map<string, set<string>>;

class Grammar
{
    friend ostream &operator<<(ostream &os, const Grammar &g)
    {
        os << "非终结符集:\n  ";
        for (auto i : g.nonTerminal)
            os << i << ' ';
        os << '\n';
        os << "终结符集合:\n  ";
        for (auto i : g.terminal)
            os << i << ' ';
        os << '\n';
        os << "产生式集:\n";

        int index(0);
        for (const auto &i : g.rules)
        {
            os << "  " << index++ << ": " << i.first << " -> ";
            for (auto j : i.second)
            {
                os << j << " ";
            }
            os << '\n';
        }
        os << "\n";
        os << "开始符号:\n  ";
        os << g.startSymbol;
        os << "\n\n";
        return os;
    }

private:
    int nonTerminalNum;      // 非终结符个数
    NONTERMINAL nonTerminal; // 非终结符
    int terminalNum;         // 终结符个数
    TERMINAL terminal;       //终结符
    int rulesNum;            //规则个数
    RULE rules;              //规则
    string startSymbol;      //开始符号
    FIRSTSET firstset;       //first集合
    FOLLOWSET followset;     //follow集合
    SELLECTSET selectset;    //select集合

    // 识别字符串中的符号  开始必须不是空白符号
    vector<string> recognizeSymbols(string &s)
    {
        vector<string> outputs;
        int start(0), i;
        for (i = 0; i < s.size(); ++i)
        {
            if (s[i] != ' ')
                continue;
            else
            {
                outputs.push_back(s.substr(start, i - start));
                while (s[i++] == ' ')
                    ;
                --i;
                start = i;
            }
        }
        if (s[i - 1] != ' ' && i == s.size())
            outputs.push_back(s.substr(start, i - start));
        return outputs;
    }

public:
    explicit Grammar(const string &filename)
    {
        ifstream in(filename);
        if (!in.is_open())
        {
            std::cerr << "不能打开文件，文件不存在?" << std::endl;
            exit(-1);
        }

        in >> nonTerminalNum;
        for (int i = 0; i < nonTerminalNum; ++i)
        {
            string t;
            in >> t;
            nonTerminal.insert(t);
        }
        string c;
        getline(in, c);
        in.clear();
        in >> terminalNum;
        for (int i(0); i < terminalNum; ++i)
        {
            string t;
            in >> t;
            terminal.insert(t);
        }
        in.clear();
        in >> rulesNum;
        getline(in, c);

        for (int i(0); i < rulesNum; ++i)
        {
            string t;
            getline(in, t);
            auto k = t.find_first_of('>');
            string s = t.substr(k + 2);
            auto symbols = recognizeSymbols(s);
            rules.insert(make_pair(t.substr(0, k - 2), symbols));
        }
        getline(in, c);
        in >> startSymbol;
        in.close();
    }

    // 求一个非终结符的First集
    set<string> firstSet(const string &symbol)
    {
        auto back = firstset.find(symbol);
        if (back != firstset.end())
            return firstset[symbol];
        set<string> myFirstSet;
        // 防止非法输入
        // if (terminal.find(symbol) != terminal.end())
        // {
        //     std::cerr << "这个函数不是针对token的" << std::endl;
        //     return myFirstSet;
        // }

        // if (nonTerminal.find(symbol) == nonTerminal.end())
        // {
        //     std::cerr << "没有这个符号!" << std::endl;
        //     return myFirstSet;
        // }

        auto val = rules.equal_range(symbol);

        for (auto &i = val.first; i != val.second; ++i)
        {
            string ch = i->second.at(0);
            if (terminal.find(ch) != terminal.end())
                myFirstSet.insert(ch);
            else if (ch == "ε")
                myFirstSet.insert(ch);
            else
            { // 非终结符
                if (ch == symbol)
                {
                    std::cerr << "直接左递归!!!，无法生成First集" << std::endl;
                }
                auto re = this->firstSet(ch);
                for (auto j : re)
                    myFirstSet.insert(j);
            }
        }
        // firstset.insert(make_pair(symbol, myFirstSet));
        firstset[symbol] = myFirstSet;
        return myFirstSet;
    }

    // 展示所有first集
    void allFirstSet()
    {
        std::cout << "First Set:" << std::endl;
        for (const auto &i : nonTerminal)
        {
            std::cout << "  First(" << i << "):  ";
            auto result = firstSet(i);
            for (const auto &i : result)
            {
                std::cout << i << ' ';
            }
            std::cout << std::endl;
        }
    }

    // 求给定非终结符的follow集
    set<string> followSet(const string &symbol, set<string> lastSymbol)
    {
        set<string> myFollowSet;
        if (symbol == startSymbol)
            myFollowSet.insert("$");
        for (auto i = rules.begin(); i != rules.end(); ++i)
        {
            auto k = i->second;
            auto index = find(k.begin(), k.end(), symbol);
            if (index != k.end())
            {
                if (index + 1 == k.end())
                {
                    if (i->first != symbol) //如果是求同一个非终结符的follow集
                    {
                        if (lastSymbol.find(i->first) == lastSymbol.end())
                        {
                            lastSymbol.insert(symbol);
                            auto re = followSet(i->first, lastSymbol);
                            for (auto j : re)
                                myFollowSet.insert(j);
                        }
                    }
                }
                else
                {
                    // 验证是否非终结符!!!
                    auto ch = *(index + 1);
                    if (terminal.find(ch) != terminal.end())
                    {
                        myFollowSet.insert(ch);
                    }
                    else
                    {
                        auto re = firstSet(*(index + 1));
                        for (auto j : re)
                            myFollowSet.insert(j);
                        if (re.find("ε") != re.end())
                        {
                            myFollowSet.erase("ε");
                            if (ch != symbol && lastSymbol.find(ch) == lastSymbol.end())
                            {
                                lastSymbol.insert(symbol);
                                auto anotherFollow = followSet(ch, lastSymbol);
                                for (auto i : anotherFollow)
                                    myFollowSet.insert(i);
                            }
                        }
                    }
                }
            }
        }
        followset[symbol] = myFollowSet;
        return myFollowSet;
    }

    void allFollowSet()
    {
        std::cout << "Follow Set:" << std::endl;
        for (const auto &i : nonTerminal)
        {
            std::cout << "  Follow(" << i << "):  ";
            auto result = followSet(i, set<string>());
            for (const auto &i : result)
            {
                std::cout << i << ' ';
            }
            std::cout << std::endl;
        }
    }

    // 找一个产生式的select集
    set<string> selectSet(const string &symbol, const vector<string> &production)
    {
        auto t = production.at(0);
        set<string> outputs;
        if (terminal.find(t) != terminal.end())
            outputs.insert(t);
        else
        { // 排除是终结符的可能性
            if (t == "ε")
                outputs.insert("ε");
            else
            {
                const auto &re = firstset.at(t);
                for (const auto &i : re)
                    outputs.insert(i);
            }
            if (outputs.find("ε") != outputs.end())
            {
                outputs.erase("ε");
                auto secondRe = followset[symbol];
                for (const auto &j : secondRe)
                    outputs.insert(j);
            }
        }
        return outputs;
    }

    void allSelectSet()
    {
        std::cout << "Select Set:" << std::endl;
        for (auto &i : rules)
        {
            cout << "  Select( " << i.first << " -> ";
            for (auto &j : i.second)
                cout << j << ' ';
            cout << "): ";
            for (auto &j : selectSet(i.first, i.second))
            {
                cout << j << ' ';
            }
            cout << std::endl;
        }
    }

    ~Grammar() = default;
};

int main(int argc, char const *argv[])
{
    string filename;
    if (argc != 2)
    {
        std::cerr << "请输入文件名" << std::endl;
        return -1;
    }
    filename = argv[1];
    Grammar g(filename);

    cout << g;

    g.allFirstSet();
    cout << "\n";
    g.allFollowSet();
    cout << "\n";
    g.allSelectSet();
    cout << "\n";
    return 0;
}