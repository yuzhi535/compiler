/**
 * @file main.cc
 * @author Zhou YuXi (zhouyuxi.abc@gmail.com)
 * @brief NFA2DFA and verify
 * @version 0.1
 * @date 2021-10-19
 * @note C++11
 * @copyright Copyright (c) 2021
 *
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <queue>
#include <set>
#include <list>

using namespace std;

using diagram = map<int, map<int, vector<int>>>;
using dfa_diagram = multimap<int, map<int, int>>;
using state = list<int>;

/**
 * @brief 最原始的FA。
 *
 */
class FA
{
public:
    diagram diag;
    int stateNum;
    int symbolNum;
    state beginState;
    state endState;

    // 验证是否NFA。若不是，则直接DFA
    bool isNFA()
    {
        for (const auto &i : diag)
        {
            for (const auto &j : i.second)
            {
                if (j.first == 0)
                    return true;
                if (j.second.size() > 1)
                    return true;
            }
        }

        return false;
    }

    explicit FA(int states, int symbols, diagram &dia, state &begin, state &end)
        : stateNum(states), symbolNum(symbols), diag(dia), beginState(begin), endState(end)
    {
    }

    friend ostream &operator<<(ostream &os, FA &fa)
    {
        os << "DFA" << endl;

        os << "\t状态个数：" << fa.stateNum << endl;
        os << "\t字符表个数：" << fa.symbolNum << endl;
        os << "状态转换：" << endl;
        for (const auto &i : fa.diag)
        {
            auto start = i.first;
            for (const auto &j : i.second)
            {
                for (auto k : j.second)
                    os << "\t(" << start << "," << j.first << ")"
                       << "->" << k << endl;
            }
        }
        return os;
    }
};

class NFA : public FA
{
private:
    dfa_diagram dfa;

public:
    void toDFA()
    {
        queue<state> q;
        vector<state> waitStates(symbolNum + 1);
        map<state, int> all_states;
        set<int> begin;
        set<int> end;

        for (auto i : beginState)
            begin.insert(i);
        for (auto i : endState)
            end.insert(i);

        // ℨ 闭包
        for (auto k = beginState.begin(); k != beginState.end(); ++k)
        {
            int index = *k;
            auto j = diag[index].find(0);
            if (j != diag[index].end())
            {
                for (auto l : j->second)
                {
                    beginState.insert(beginState.end(), l);
                }
            }
        }

        q.push(beginState);
        int index(0);
        all_states.insert(make_pair(beginState, index++));

        while (!q.empty())
        {
            beginState = q.front();
            q.pop();

            for (auto i : beginState)
            {
                for (int j(1); j <= symbolNum; ++j)
                {
                    auto k = diag[i].find(j);
                    if (k != diag[i].end())
                    {
                        for (auto l : k->second)
                            waitStates[j].push_back(l);
                    }
                }
            }

            for (int i(1); i <= symbolNum; ++i)
            {
                if (waitStates[i].empty())
                    continue;
                for (auto k = waitStates[i].begin(); k != waitStates[i].end(); ++k)
                {
                    int num = *k;
                    auto j = diag[num].find(0);
                    if (j != diag[num].end())
                    {
                        for (auto l : j->second)
                        {
                            waitStates[i].insert(waitStates[i].end(), l);
                        }
                    }
                }
                waitStates[i].sort();
                waitStates[i].unique();

                map<int, int> temp;
                if (all_states.find(waitStates[i]) == all_states.end())
                {
                    q.push(waitStates[i]);
                    all_states.insert(make_pair(waitStates[i], index++));
                }
                temp.insert(make_pair(i, all_states[waitStates[i]]));
                dfa.insert(make_pair(all_states[beginState], temp));
                waitStates[i].clear();
            }
        }
        stateNum = all_states.size();

        beginState.clear();
        endState.clear();
        for (const auto &i : all_states)
        {
            for (auto j : begin)
            {
                auto k = find(i.first.begin(), i.first.end(), j);
                if (k != i.first.end())
                    beginState.push_back(i.second);
            }
            for (auto j : end)
            {
                auto k = find(i.first.begin(), i.first.end(), j);
                if (k != i.first.end())
                    endState.push_back(i.second);
            }
        }
    }

    explicit NFA(int states, int symbols, diagram &dia, state &beginState, state &endState)
        : FA(states, symbols, dia, beginState, endState)
    {
    }

    friend ostream &operator<<(ostream &os, NFA &nfa)
    {
        os << "NFA" << endl;
        os << "  状态个数：" << nfa.stateNum << endl;
        os << "  字符表个数：" << nfa.symbolNum << endl;
        os << "  状态转换：" << endl;
        for (const auto &i : nfa.dfa)
        {
            auto start = i.first;
            for (auto j : i.second)
            {
                os << "\t(" << start << "," << j.first << ")"
                   << "->" << j.second << endl;
            }
        }

        os << "  开始状态：" << *nfa.beginState.begin() << endl;
        os << "  结束状态集：{ ";
        for (auto i : nfa.endState)
            os << i << " ";
        os << "}" << endl;
        return os;
    }
};

// 去除多余行
void strip(ifstream &in, string &s)
{
    while (getline(in, s))
    {
        if (!s.empty())
            break;
    }
}

void getNum(state &thisState, string &s)
{
    int lastpos(0);
    for (int i(0); i < s.size(); ++i)
    {
        if (s[i] == ' ')
        {
            thisState.push_back(stoi(s.substr(lastpos, i - lastpos)));
            while (s[i] == ' ')
                ++i;
            lastpos = i;
        }
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        cerr << "must follow a file name to be detected!";
        return -1;
    }

    ifstream in;
    in.open(argv[1]);
    int stateNum, symbolNum;
    diagram diag;
    in >> stateNum;
    in >> symbolNum;
    string s;
    strip(in, s);

    // 获取状态转换图
    while (true)
    {
        // 结束
        if (s == "-1")
        {
            break;
        }
        queue<int> nums;
        int lastpos(0);
        for (int i(0); i < s.size(); ++i)
        {
            if (s[i] == ' ')
            {
                nums.push(stoi(s.substr(lastpos, i - lastpos)));
                while (s[i] == ' ')
                    ++i;
                lastpos = i;
            }
        }
        getline(in, s);

        int state = nums.front();
        nums.pop();
        int symbol = nums.front();
        nums.pop();

        map<int, vector<int>> temp_map;
        vector<int> temp_vec;
        while (!nums.empty())
        {
            int otherState = nums.front();
            nums.pop();
            // diag.insert(make_pair(make_pair(state, symbol), otherState));
            temp_vec.push_back(otherState);
        }
        temp_map[symbol] = temp_vec;
        diag.insert(make_pair(state, temp_map));
    }

    state beginState;
    state endState;
    // 开始状态
    strip(in, s);
    getNum(beginState, s);
    //终结状态
    strip(in, s);
    getNum(endState, s);
    in.close();

    FA *fa = new NFA(stateNum, symbolNum, diag, beginState, endState);
    if (fa->isNFA())
    {
        static_cast<NFA *>(fa)->toDFA();
        cout << *static_cast<NFA *>(fa);
    }
    else
    {
        cout << *fa;
    }
    return 0;
}
