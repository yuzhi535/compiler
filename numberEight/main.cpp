/**
 * @file main.cpp
 * @author Zhou YuXi (周誉喜) (zhouyuxi@stu.zzu.edu.cn)
 * @brief LR1语法分析器的设计与实现
 * @version 0.1
 * @date 2021-11-27
 * @note c++11
 * @copyright Copyright (c) 2021
 *
 */

#include <iostream>
#include <list>
#include <vector>
#include <stack>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>
#include <queue>
#include <stack>

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

struct Item;

class ItemsSet;

class Grammar;

class GrammarLR0;

using NONTERMINAL = set<string>;						  //非终结符
using TERMINAL = set<string>;							  //终结符
using RULE = multimap<string, pair<vector<string>, int>>; //规则
using ITEMS = vector<Item>;								  //局部的项目集，不是正式的项目集
using ITEMFAMILY = vector<ItemsSet>;					  //项目集族
using FIRSTSET = map<string, set<string>>;				  // first集

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

class Grammar
{
public:
	explicit Grammar(ifstream &in)
	{
		in >> nonTerminalNum;
		for (int i = 0; i < nonTerminalNum; ++i)
		{
			string t;
			in >> t;
			nonTerminals.insert(t);
		}

		string c;
		getline(in, c);
		in.clear();
		in >> terminalNum;
		for (int i(0); i < terminalNum; ++i)
		{
			string t;
			in >> t;
			terminals.insert(t);
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
			rules.insert(make_pair(t.substr(0, k - 2), make_pair(symbols, i)));
		}
		getline(in, c);
		in >> startSymbol;
	}

	bool isNonterminal(const string &s) const
	{
		if (nonTerminals.find(s) != nonTerminals.end())
			return true;
		return false;
	}

	pair<RULE::const_iterator, RULE::const_iterator>
	findRulesScope(const string &symbol) const
	{
		return rules.equal_range(symbol);
	}

	void getAllFirstSet()
	{
		for (const auto &i : nonTerminals)
			calFirstSet(i);
	}

	set<string> &getFirstSet(const string &symbol)
	{
		auto back = firstSet.find(symbol);
		if (back != firstSet.end())
			return firstSet[symbol];
	}

	// 求一个非终结符的First集
	set<string> calFirstSet(const string &symbol)
	{
		auto back = firstSet.find(symbol);
		if (back != firstSet.end())
			return firstSet[symbol];
		set<string> myFirstSet;

		auto val = rules.equal_range(symbol);

		for (auto &i = val.first; i != val.second; ++i)
		{
			string ch = i->second.first.at(0);
			if (terminals.find(ch) != terminals.end())
				myFirstSet.insert(ch);
			else if (ch == "ε")
				myFirstSet.insert(ch);
			else
			{ // 非终结符
				if (ch == symbol)
				{
					std::cerr << "直接左递归!!!，无法生成First集" << std::endl;
				}
				auto re = this->calFirstSet(ch);
				for (const auto &j : re)
					myFirstSet.insert(j);
			}
		}
		// firstSet.insert(make_pair(symbol, myFirstSet));
		firstSet[symbol] = myFirstSet;
		return myFirstSet;
	}

protected:
	int nonTerminalNum;		  // 非终结符个数
	NONTERMINAL nonTerminals; // 非终结符
	int terminalNum;		  // 终结符个数
	TERMINAL terminals;		  //终结符
	int rulesNum;			  //规则个数
	RULE rules;				  //规则
	string startSymbol;		  //开始符号
	string input;
	FIRSTSET firstSet; //first集合
};

/**
 * @brief 项目集
 * 龙书的说法上分kernel 和 nonkernel
 * T -> .TE
 * T -> T.E
 *
 */
struct Item
{
	Item(string sym, int position, vector<string> b, string t_lookahead = "$")
		: symbol(sym), dotPos(position), body(b), lookahead(t_lookahead)
	{
	}

	bool operator==(const Item &other) const
	{
		return symbol == other.symbol && dotPos == other.dotPos && body == other.body && lookahead == other.lookahead;
	}

	vector<string> body;
	int dotPos;
	string symbol;
	string lookahead;
};

/**
 * @brief 项目集
 *
 */
class ItemsSet
{
public:
	explicit ItemsSet(ITEMS i)
	{
		kernels = i;
	}

	// 得到first集
	set<string> getFirst(const string &symbol, const vector<string> &body, int pos, Grammar *g, string lookahead)
	{
		auto re = g->getFirstSet(symbol);
		if (re.find("ε") != re.end())
		{
			re.insert(lookahead);
			re.erase("ε");
		}
		return re;
	}

	struct cmp
	{
		bool operator()(const pair<string, pair<vector<string>, string>> &a,
						const pair<string, pair<vector<string>, string>> &b)
		{
			if (a.first < b.first)
				return true;
			if (!std::equal(a.second.first.begin(), a.second.first.end(), b.second.first.begin()))
				return true;
			if (a.second.second < b.second.second)
				return true;
			return false;
		}
	};

	/**
	 * @brief 闭包
	 *
	 * @param g
	 * @note 上一个实验LR0验证是否已经闭包完成只需要用一个数组标记一下就行了，相当于偷个懒
	 * @note 这次需要作出一些变化才能这样进行直接的映射比较
	 */
	void closure(Grammar *g)
	{
		map<pair<string, pair<vector<string>, string>>, bool, cmp> visited; //防止无限循环

		auto clos = [&](vector<Item> &items, int index, bool isKernel)
		{
			const int &pos = items[index].dotPos; // 必是0
			if (pos < items[index].body.size())	  // 如果还有移动的空间
			{
				string lookahead = items[index].lookahead;
				string str = items[index].body[pos];
				if (isKernel)
				{
					auto scope = g->findRulesScope(str);
					for (auto i = scope.first; i != scope.second; ++i)
					{
						// 这时候得考虑nonkernel加lookahead
						// 找 向前搜索符，即lookahead
						set<string> lookaheads;
						if (pos + 1 < items[index].body.size())
						{
							const string &signal = items[index].body[pos + 1];
							if (!g->isNonterminal(signal))
							{
								lookahead = signal;
								lookaheads.insert(lookahead);
							}
							else
							{
								// 求first集
								lookaheads = getFirst(items[index].symbol, items[index].body, pos + 1, g, lookahead);
							}
						}
						else
						{
							lookaheads = set<string>{lookahead};
						}

						// 查找是否已经有这个
						for (const auto &_lookahead : lookaheads)
						{
							if (!visited[make_pair(str, make_pair(i->second.first, _lookahead))])
							{
								nonKernels.push_back(Item(str, 0, i->second.first, _lookahead));
								visited[make_pair(str, make_pair(i->second.first, _lookahead))] = true;
							}
						}
					}
				}
				else
				{
					int pos = items[index].dotPos;
					if (pos < items[index].body.size())
					{
						string str = items[index].body[pos];
						// 是否非终结符
						if (g->isNonterminal(str))
						{
							auto scope = g->findRulesScope(str);
							for (auto i = scope.first; i != scope.second; ++i)
							{
								set<string> lookaheads;
								if (pos + 1 < items[index].body.size())
								{
									const string &signal = items[index].body[pos + 1];
									if (!g->isNonterminal(signal))
									{
										lookahead = signal;
										lookaheads.insert(lookahead);
									}
									else
									{
										// 求first集
										lookaheads = getFirst(items[index].symbol, items[index].body, pos + 1, g,
															  lookahead);
									}
								}
								else
								{
									lookaheads = set<string>{lookahead};
								}
								for (const auto &lookahead : lookaheads)
								{
									if (!visited[make_pair(str, make_pair(i->second.first, lookahead))])
									{
										// 注意迭代器失效的问题!!!
										// 用索引解决
										nonKernels.insert(nonKernels.end(), Item(str, 0, i->second.first, lookahead));
										visited[make_pair(str, make_pair(i->second.first, lookahead))] = true;
									}
								}
							}
						}
					}
				}
			}
		};

		// 对kernel进行移动
		for (int i(0); i < kernels.size(); ++i)
		{
			clos(kernels, i, true);
		}
		for (int i(0); i < nonKernels.size(); ++i)
		{
			clos(nonKernels, i, false);
		}
	}

	// 该项目集是否可全部被规约
	bool isAllToBeReduced() const
	{
		for (const auto &kernel : kernels)
			if (kernel.dotPos != kernel.body.size())
				return false;
		if (!nonKernels.empty())
			return false;
		return true;
	}

	// 该项目集是否可被规约，在LR(0)里面与上一个函数其实是一样的
	bool isToBeReduced(int &index) const
	{
		for (int i(0); i < kernels.size(); ++i)
			if (kernels[i].dotPos == kernels[i].body.size())
			{
				index = i;
				return true;
			}
		return false;
	}

	const ITEMS &getKernel() const
	{
		return kernels;
	}

	const ITEMS &getNonKernel() const
	{
		return nonKernels;
	}

	bool operator==(const ItemsSet &other)
	{
		if (kernels == other.kernels)
			return true;
		return false;
	}

private:
	ITEMS kernels;
	ITEMS nonKernels;
};

/**
 * @brief LR1文法
 * @TODO first set
 * @TODO parse
 * @TODO
 */
class GrammarLR1 : public Grammar
{
	friend ostream &operator<<(ostream &os, const GrammarLR1 &other)
	{
		os << "CFG=(VN,VT,P,S)\n";
		os << "VN: ";
		for (const auto &nonTerminal : other.nonTerminals)
		{
			os << nonTerminal << " ";
		}
		os << "\n";
		os << "VT: ";
		for (const auto &terminal : other.terminals)
		{
			os << terminal << " ";
		}
		os << "\n";
		os << "Production: \n";
		for (const auto &rule : other.rules)
		{
			os << "  " << rule.second.second << " " << rule.first << " -> ";
			for (const auto &prod : rule.second.first)
			{
				os << prod << " ";
			}
			os << "\n";
		}
		os << "StartSymbol: " << other.startSymbol << "\n";

		os << "[LR(1) item set cluster]\n";

		return os;
	}

public:
	GrammarLR1(ifstream &in) : Grammar(in)
	{
		getAllFirstSet();
		makeItemsSetFamily();
	}

	// 总控程序
	void parser(vector<string> input)
	{
	}

private:
	// 生成项目集簇
	void makeItemsSetFamily()
	{
		// @TODO 记住要寻找first集!!!
		// -------------------------

		auto production = *rules.find(startSymbol);
		Item item(startSymbol, 0, production.second.first);
		ITEMS t;
		t.push_back(item);
		ItemsSet itemsSet(t);
		itemsSet.closure(this);
		// -------------------------

		queue<pair<ItemsSet, int>> q;
		q.push(make_pair(itemsSet, itemsSetFamily.size()));
		itemsSetFamily.push_back(itemsSet);

		// shift until nothing is added in this queue
		while (!q.empty())
		{
			auto top = q.front();
			q.pop();

			const auto &kernels = top.first.getKernel();
			const auto &nonKernels = top.first.getNonKernel();

			vector<Item> kkernels;
			kkernels.reserve(kernels.size() + nonKernels.size());
			for (const auto &kernel : kernels)
				kkernels.push_back(kernel);
			for (const auto &nonKernel : nonKernels)
				kkernels.push_back(nonKernel);

			int index = top.second; // 源的索引

			// 查看哪些是否可以移进一个位置
			auto find = [&](const string &character, bool &flag, ITEMS &items) -> pair<string, bool>
			{
				string shiftSymbol;
				bool flag1 = false;
				for (const auto &kk : kkernels)
				{
					// 遍历得到可以shift的item
					int pos = kk.dotPos;
					auto body = kk.body;
					string symbol = kk.symbol;
					// 如果是要规约
					if (pos == body.size())
					{
						continue;
					}

					// 如果是要移进
					shiftSymbol = body[pos];
					pos += 1;

					if (shiftSymbol == character)
					{
						// 如果都没有，则需要在后续返回一个flag
						flag1 = true; //表示可以shift
						Item item1(symbol, pos, body, kk.lookahead);
						items.push_back(item1);
						for (int i(0); i < this->itemsSetFamily.size(); ++i)
						{
							// 如果找到之前的项
							if (itemsSetFamily[i].getKernel() == items)
							{
								gotoTable[make_pair(index, character)] = i;
								flag = true; //表示可以shift，但是添加到表里面了，也就是不用再添加了
								break;
							}
						}
					}
				}
				return make_pair(shiftSymbol, flag1);
			};

			// 先移进非终结符
			for (const auto &nonTerminal : nonTerminals)
			{
				ITEMS items;
				bool flag = false; // 验证是否是新的项目集
				auto re = find(nonTerminal, flag, items);
				if (!re.second)
					continue;
				string shiftSymbol = re.first;

				ItemsSet itemsSet1(items);
				itemsSet1.closure(this);
				// 如果有一个项不规约，则可以继续入队列
				if (!itemsSet1.isAllToBeReduced())
				{
					q.push(make_pair(itemsSet1, itemsSetFamily.size()));
				}
				gotoTable[make_pair(index, nonTerminal)] = itemsSetFamily.size();
				if (!flag)
					itemsSetFamily.push_back(itemsSet1);
			}

			// 移进终结符
			for (const auto &terminal : terminals)
			{
				ITEMS items;
				bool flag = false;
				auto re = find(terminal, flag, items);
				if (!re.second)
					continue;
				string shiftSymbol = re.first;
				if (flag)
					continue;

				ItemsSet itemsSet1(items);
				itemsSet1.closure(this);
				// 如果有一个项不规约，则可以继续入队列
				if (!itemsSet1.isAllToBeReduced())
				{
					q.push(make_pair(itemsSet1, itemsSetFamily.size()));
				}
				gotoTable[make_pair(index, terminal)] = itemsSetFamily.size();
				if (!flag)
					itemsSetFamily.push_back(itemsSet1);
			}
		}
	}

	ITEMFAMILY itemsSetFamily;
	map<pair<int, string>, int> gotoTable;
	bool isLR0;
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
	ifstream in;
	in.open(filename);
	if (!in.is_open())
	{
		std::cerr << "不能打开文件，文件不存在?" << std::endl;
		exit(-1);
	}
	GrammarLR1 g(in);
	string input;
	do
	{
		getline(in, input);
	} while (input == "");
	vector<string> ans = recognizeSymbols(input);
	cout << g << std::endl;
	// g.parser(ans);
	return 0;
}