#include <iostream>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>

using namespace std;

const int maxn = 100;

int flag;
vector<int>::iterator it, itt, t;
map<vector<int>, int> state;
stack<vector<int>> statestack;

int same(const vector<int> v);

void debug(vector<int> v)
{
    for (t = v.begin(); t != v.end(); t++)
        cout << *t << " ";
    cout << endl;
}

class DFA
{
public:
    int stateNum;
    int symbolNum;
    vector<int> stateSet[maxn][maxn];
    int symbol[maxn];
    int start;
    vector<int> endSet;

    DFA(int a_symbolNum, int a_stateNum = 0) : symbolNum(a_symbolNum) {}
    void setDFASymbolNum(int a_symbolNum)
    {
        this->symbolNum = a_symbolNum;
    }
    void printDFA()
    {
        cout << "DFA" << endl;
        cout << "\t状态个数" << stateNum << endl; //0-stateNUm
        cout << "\t字符表个数" << symbolNum << endl;
        cout << "\t状态转换" << endl;
        for (int i = 0; i <= stateNum; i++)
        {
            for (int j = 0; j <= symbolNum; j++)
            {
                for (it = stateSet[i][j].begin(); it != stateSet[i][j].end(); it++)
                {
                    cout << "\t\t" << '(' << i << ',' << j << ')' << "->" << *it << endl;
                }
            }
        }
        cout << "\t开始状态" << ' ' << start << ' ' << endl;
        cout << "\t结束状态集" << '{' << ' ';
        for (it = endSet.begin(); it != endSet.end(); it++)
        {
            cout << *it;
            if (it != endSet.end() - 1)
                cout << ' ';
        }
        cout << ' ' << '}' << endl;
    }
};

class NFA
{
private:
    int stateNum;
    int symbolNum;
    vector<int> stateSet[maxn][maxn]; //0-stateNUm-1
    vector<int> symbolSet;            //1-symbolNUm 0是空
    vector<int> startSet;
    vector<int> endSet;

public:
    NFA(int a_stateNum, int a_symbolNum) : stateNum(a_stateNum), symbolNum(a_symbolNum){};
    ~NFA(){};
    void setStateSet()
    {
        int state, symbol, res;
        cin >> state;
        while (state != -1)
        {
            cin >> symbol >> res;
            while (res != -1)
            {
                stateSet[state][symbol].push_back(res);
                cin >> res;
            }
            cin >> state;
        }
    }
    void setStartSet()
    {
        int t;
        cin >> t;
        while (t != -1)
        {
            startSet.push_back(t);
            cin >> t;
        }
    }
    void setEndSet()
    {
        int t;
        cin >> t;
        while (t != -1)
        {
            endSet.push_back(t);
            cin >> t;
        }
    }
    void changeToDFA(DFA &dfa);
    void printNFA()
    {
        cout << "NFA" << endl;
        cout << "\t状态个数" << stateNum << endl;
        cout << "\t字符表个数" << symbolNum << endl;
        cout << "\t状态转换" << endl;
        for (int i = 0; i <= stateNum; i++)
        {
            for (int j = 0; j <= symbolNum; j++)
            {
                for (it = stateSet[i][j].begin(); it != stateSet[i][j].end(); it++)
                {
                    cout << "\t\t" << '(' << i << ',' << j << ')' << "->" << *it << endl;
                }
            }
        }
        cout << "\t开始状态" << '{' << ' ';
        for (it = startSet.begin(); it != startSet.end(); it++)
        {
            cout << *it;
            if (it != startSet.end() - 1)
                cout << ' ';
        }
        cout << ' ' << '}' << endl;
        cout << "\t结束状态集" << '{' << ' ';
        for (it = endSet.begin(); it != endSet.end(); it++)
        {
            cout << *it;
            if (it != endSet.end() - 1)
                cout << ' ';
        }
        cout << ' ' << '}' << endl;
    }

    void voidstring(vector<int> &vect)
    {
        int len = vect.size();
        for (int i = 0; i < len; i++)
        {
            for (itt = stateSet[vect[i]][0].begin(); itt != stateSet[vect[i]][0].end(); itt++)
            {
                if (isendset(*itt))
                    flag = 1;
                vect.push_back(*itt);
            }
            len = vect.size();
        }
        sort(vect.begin(), vect.end());
    }
    int isendset(int n);
};

void NFA::changeToDFA(DFA &dfa)
{
    string tmp;
    vector<int> sstart(startSet);
    voidstring(sstart);
    if (!same(sstart))
        state[sstart] = dfa.stateNum++;
    dfa.start = state[sstart];

    statestack.push(sstart);
    while (!statestack.empty())
    {
        vector<int> statcktmp = statestack.top();
        statestack.pop();
        for (int k = 1; k <= 2; k++)
        {
            flag = 0;
            vector<int> tmpv;
            tmpv.clear();
            for (int i = 0; i < statcktmp.size(); i++)
            {
                for (itt = stateSet[statcktmp[i]][k].begin(); itt != stateSet[statcktmp[i]][k].end(); itt++)
                {
                    if (isendset(*itt))
                        flag = 1;
                    tmpv.push_back(*itt);
                }
            }
            voidstring(tmpv);
            if (tmpv.empty())
                continue;
            if (!same(tmpv))
            {
                state[tmpv] = dfa.stateNum++;
                statestack.push(tmpv);
                if (flag)
                    dfa.endSet.push_back(state[tmpv]);
            }
            dfa.stateSet[state[statcktmp]][k].push_back(state[tmpv]);
        }
    }
}

int same(const vector<int> v)
{
    return state.count(v);
}

int NFA::isendset(int n)
{
    for (int i = 0; i < endSet.size(); i++)
    {
        if (endSet[i] == n)
        {
            return 1;
        }
    }
    return 0;
}

int main()
{
    fflush(stdin);
    int stateNum;
    int symbolNum;
    cin >> stateNum;
    cin >> symbolNum;
    NFA nfa(stateNum, symbolNum);
    nfa.setStateSet();
    nfa.setStartSet();
    nfa.setEndSet();

    DFA dfa(symbolNum);
    nfa.changeToDFA(dfa);
    dfa.printDFA();

    fflush(stdin);
}

//NFA.exe < NFA.txt
