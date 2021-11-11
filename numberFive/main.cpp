/**
 * @file main.cpp
 * @author 周誉喜 (zhouyuxi@stu.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2021-11-11
 * @remark c++11
 * @copyright Copyright (c) 2021
 * 
 */

/*
一个一个比较select集
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
#include <stack>
#include <queue>

using std::cin;
using std::cout;
using std::fstream;
using std::ifstream;
using std::make_pair;
using std::map;
using std::multimap;
using std::ostream;
using std::pair;
using std::queue;
using std::set;
using std::stack;
using std::string;
using std::vector;

using NONTERMINAL = set<string>;
using TERMINAL = set<string>;
using RULE = multimap<string, vector<string>>;
using FIRSTSET = map<string, set<string>>;
using FOLLOWSET = map<string, set<string>>;
using SELECTSET = multimap<string, pair<int, string>>;

class Grammar
{
    friend ostream &operator<<(ostream &os, const Grammar &g)
    {
        os << "CFG\n非终结符集:\n  ";
        for (const auto &i : g.nonTerminal)
            os << i << ' ';
        os << '\n';
        os << "终结符集:\n  ";
        for (const auto &i : g.terminal)
            os << i << ' ';
        os << '\n';
        os << "产生式集:\n";

        int index(0);
        for (const auto &i : g.rules)
        {
            os << "  " << index++ << ": " << i.first << " -> ";
            for (const auto &j : i.second)
            {
                os << j << " ";
            }
            os << '\n';
        }
        os << "\n";
        os << "开始符号:\n  ";
        os << g.startSymbol;
        os << "\n";
        return os;
    }

private:
    int nonTerminalNum;              // 非终结符个数
    NONTERMINAL nonTerminal;         // 非终结符
    int terminalNum;                 // 终结符个数
    TERMINAL terminal;               //终结符
    int rulesNum;                    //规则个数
    RULE rules;                      //规则
    string startSymbol;              //开始符号
    FIRSTSET firstSet;               //first集合
    FOLLOWSET followSet;             //follow集合
    SELECTSET selectSet;             //select集合
    string str;                      //待预测字符串
    map<string, map<string, int>> m; // 预测分析表
    vector<string> allRules;

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

    // 求一个非终结符的First集
    set<string> getFirstSet(const string &symbol)
    {
        auto back = firstSet.find(symbol);
        if (back != firstSet.end())
            return firstSet[symbol];
        set<string> myFirstSet;

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
                auto re = this->getFirstSet(ch);
                for (const auto &j : re)
                    myFirstSet.insert(j);
            }
        }
        // firstSet.insert(make_pair(symbol, myFirstSet));
        firstSet[symbol] = myFirstSet;
        return myFirstSet;
    }

    // 求给定非终结符的follow集
    set<string> getFollowSet(const string &symbol, set<string> lastSymbol)
    {
        set<string> myFollowSet;
        if (symbol == startSymbol)
            myFollowSet.insert("$");
        for (auto &rule : rules)
        {
            auto k = rule.second;
            auto index = find(k.begin(), k.end(), symbol);
            if (index != k.end())
            {
                if (index + 1 == k.end())
                {
                    if (rule.first != symbol) //如果是求同一个非终结符的follow集
                    {
                        if (lastSymbol.find(rule.first) == lastSymbol.end())
                        {
                            lastSymbol.insert(symbol);
                            auto re = getFollowSet(rule.first, lastSymbol);
                            for (const auto &j : re)
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
                        auto re = getFirstSet(*(index + 1));
                        for (const auto &j : re)
                            myFollowSet.insert(j);
                        if (re.find("ε") != re.end())
                        {
                            myFollowSet.erase("ε");
                            if (ch != symbol && lastSymbol.find(ch) == lastSymbol.end())
                            {
                                lastSymbol.insert(symbol);
                                auto anotherFollow = getFollowSet(ch, lastSymbol);
                                for (const auto &i : anotherFollow)
                                    myFollowSet.insert(i);
                            }
                        }
                    }
                }
            }
        }
        followSet[symbol] = myFollowSet;
        return myFollowSet;
    }

    // 找一个产生式的select集
    void getSelectSet(const string &symbol, const vector<string> &production, int index /*表示第几个规则*/)
    {
        auto t = production.at(0);
        if (terminal.find(t) != terminal.end())
        {
            selectSet.insert(make_pair(symbol, make_pair(index, t)));
        }
        else
        { // 排除是终结符的可能性
            bool flag(false);
            if (t != "ε")
            {
                const auto &re = firstSet.at(t);
                for (const auto &i : re)
                    if (i != "ε")
                        selectSet.insert(make_pair(symbol, make_pair(index, i)));
                    else
                        flag = true;
            }
            else
                flag = true;
            if (flag)
            {
                auto secondResult = followSet[symbol];
                for (const auto &j : secondResult)
                    selectSet.insert(make_pair(symbol, make_pair(index, j)));
            }
        }
    }

    void printARow()
    {
        cout << "-----+-----+-----+-----+-----+-----+-----" << '\n';
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

            // 自c++11以来键值对顺序不会改变，就按插入的顺序来
            allRules.push_back(s);
            rules.insert(make_pair(t.substr(0, k - 2), symbols));
        }
        getline(in, c);
        in >> startSymbol;
        getline(in, c);
        in >> str;
        in.close();
    }

    // 展示所有first集
    void allFirstSet()
    {
        std::cout << "First Set:" << std::endl;
        for (const auto &i : nonTerminal)
        {
            std::cout << "  First(" << i << "):  ";
            auto result = getFirstSet(i);
            for (const auto &item : result)
            {
                std::cout << item << ' ';
            }
            std::cout << std::endl;
        }
    }

    void allFollowSet()
    {
        std::cout << "Follow Set:" << std::endl;
        for (const auto &i : nonTerminal)
        {
            std::cout << "  Follow(" << i << "):  ";
            auto result = getFollowSet(i, set<string>());
            for (const auto &item : result)
            {
                std::cout << item << ' ';
            }
            std::cout << std::endl;
        }
    }

    void getAllFirstSet()
    {
        for (const auto &i : nonTerminal)
            getFirstSet(i);
    }

    void getAllFollowSet()
    {
        for (const auto &i : nonTerminal)
            getFollowSet(i, set<string>());
    }

    void makeSelectSet()
    {
        int index(0);
        for (auto &i : rules)
        {
            getSelectSet(i.first, i.second, index);
            ++index;
        }
        // cout << "hello world\n";
        // for (const auto &i : selectSet)
        // {
        //     printf("%s %d %s\n", i.first.c_str(), i.second.first, i.second.second.c_str());
        // }
    }

    void printTable()
    {
        getAllFirstSet();
        getAllFollowSet();
        makeSelectSet();

        cout << '\t';
        map<string, int> col2int;
        vector<string> col;

        for (const auto &i : this->terminal)
        {
            cout << i << '\t';
            col.push_back(i);
        }
        //      add #
        cout << '#' << '\n';
        col.push_back("#");

        this->printARow();
        // cout << '\t';
        for (const auto &i : nonTerminal)
        {
            for (const auto &i : this->terminal)
            {
                col2int[i] = -1;
            }
            //      add #
            col2int["#"] = -1;
            cout << i << '\t';
            for (int j(0); j < col2int.size(); ++j)
            {
                auto re = this->selectSet.equal_range(i);
                for (auto k = re.first; k != re.second; ++k)
                {
                    col2int[k->second.second] = k->second.first;
                }
            }
            for (const auto j : col)
            {
                if (col2int[j] >= 0)
                {
                    m[i][j] = col2int[j];
                    cout << "p" << col2int[j] << '\t';
                }
                else
                {
                    m[i][j] = -1;
                    cout << "\t";
                }
            }
            cout << "\n";
        }
    }

    // 预测分析具体字符串
    void parser()
    {
        cout << this->str << '#' << "分析过程" << '\n';
        stack<string> inStack;
        cout << "初始化：#入栈，S入栈；" << '\n';
        inStack.push("#");
        inStack.push(startSymbol);
        // 补终结符
        if (str[str.size() - 1] != '#')
            str += '#';

        stack<string> outStack;
        // 把后面入栈
        for (auto i = str.rbegin(); i != str.rend(); ++i)
            outStack.push(string(1, *i));
        int index(1);
        while (!inStack.empty())
        {
            auto ch = inStack.top();
            inStack.pop();
            if (index < 10)
                cout << "0";
            auto c = outStack.top();
            outStack.pop();
            cout << index << ":出栈X=" << ch << "输入c=" << c << ", ";
            if (ch == c)
            {
                if (ch == "#")
                {
                    cout << "匹配，成功。\n";
                    return;
                }
                cout << "匹配，输入指针后移；\n";
            }
            else if (m[ch][c] >= 0)
            {
                string s = allRules[m[ch][c]];
                if (s == "ε")
                {
                    cout << "查表, M(X, c)=" << ch << "->" << s << "\n";
                    outStack.push(c);
                }
                else
                {
                    cout << "查表, M(X, c)=" << ch << "->" << s << ", 产生式右部逆序入栈；"
                         << "\n";

                    auto symbols = recognizeSymbols(s);
                    for (auto k = symbols.rbegin(); k != symbols.rend(); ++k)
                        inStack.push(*k);
                    outStack.push(c);
                }
            }
            else
            {
                cout << "错误!\n";
                return;
            }
            index += 1;
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

    cout << g << std::endl;

    g.printTable();
    g.parser();
    return 0;
}