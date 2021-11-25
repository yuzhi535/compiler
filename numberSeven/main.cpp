/**
 * @file main.cpp
 * @author Zhou YuXi (周誉喜) (zhouyuxi.abc@gmail.com)
 * @brief LR0语法分析器的设计与实现
 * @version 0.1
 * @date 2021-11-20
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
using std::string;
using std::vector;

class Item;

class ItemsSet;

class Grammar;

class GrammarLR0;

using NONTERMINAL = set<string>;						  //非终结符
using TERMINAL = set<string>;							  //终结符
using RULE = multimap<string, pair<vector<string>, int>>; //规则
using ITEMS = vector<Item>;								  //局部的项目集，不是正式的项目集
using ITEMFAMILY = vector<ItemsSet>;					  //项目集族

class Grammar
{
public:
	explicit Grammar(const string &filename, ifstream &in)
	{
		in.open(filename);
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

	vector<string> recognizeSymbols(string &s) const
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

protected:
	int nonTerminalNum;		  // 非终结符个数
	NONTERMINAL nonTerminals; // 非终结符
	int terminalNum;		  // 终结符个数
	TERMINAL terminals;		  //终结符
	int rulesNum;			  //规则个数
	RULE rules;				  //规则
	string startSymbol;		  //开始符号
	string input;
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
	Item(string sym, int position, vector<string> b) : symbol(sym), pos(position), body(b)
	{
	}

	bool operator==(const Item &other) const
	{
		return symbol == other.symbol && pos == other.pos && body == other.body;
	}

	vector<string> body;
	int pos;
	string symbol;
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

	/**
	 * @brief 闭包
	 *
	 * @param g
	 */
	void closure(const Grammar *g)
	{
		map<pair<string, string>, bool> visited;
		for (const auto &item : kernels)
		{
			int pos = item.pos;
			if (pos < item.body.size())
			{
				string str = item.body[pos];
				// 是否非终结符
				if (g->isNonterminal(str))
				{
					auto scope = g->findRulesScope(str);
					vector<string> vec;
					for (auto i = scope.first; i != scope.second; ++i)
					{
						if (!visited[make_pair(str, i->second.first[0])])
							nonKernels.push_back(Item(str, 0, i->second.first));
						else
							continue;
						visited[make_pair(str, i->second.first[0])] = true;
					}
				}
			}
		}
		for (int i(0); i < nonKernels.size(); ++i)
		{
			int pos = nonKernels[i].pos;
			if (pos < nonKernels[i].body.size())
			{
				string str = nonKernels[i].body[pos];
				str = g->recognizeSymbols(str)[0];
				// 是否非终结符
				if (g->isNonterminal(str))
				{
					auto scope = g->findRulesScope(str);
					vector<string> vec;
					for (auto i = scope.first; i != scope.second; ++i)
					{
						if (!visited[make_pair(str, i->second.first[0])])
							nonKernels.push_back(Item(str, 0, i->second.first));
						else
							continue;
						visited[make_pair(str, i->second.first[0])] = true;
					}
				}
			}
		}
	}

	// 该项目集是否可全部被规约
	bool isAllToReduced() const
	{
		for (const auto &kernel : kernels)
			if (kernel.pos != kernel.body.size())
				return false;
		if (!nonKernels.empty())
			return false;
		return true;
	}

	// 该项目集是否可被规约
	bool isToReduced(int &index) const
	{
		for (int i(0); i < kernels.size(); ++i)
			if (kernels[i].pos == kernels[i].body.size())
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
 * @brief LR0文法
 *
 */
class GrammarLR0 : public Grammar
{
	friend ostream &operator<<(ostream &os, GrammarLR0 &other)
	{
		os << "LR(0) 项目集簇\n";
		other.printItemsFamily(os);
		other.printDFA(os);
		other.printACTION(os);
		if (other.isLR0)
			other.printGOTO(os);
		// other.buildTable();
		return os;
	}

public:
	explicit GrammarLR0(const string &filename, ifstream &in) : Grammar(filename, in)
	{
		// augment();
		makeItemsFamily();
		isLR0 = true;
	}

	// 输出项目集
	void printItemsFamily(ostream &os) const
	{
		for (int i(0); i < itemsSetFamily.size(); ++i)
		{
			os << "I" << i << "\n";
			const auto &kernel = itemsSetFamily[i].getKernel();
			const auto &nonKernel = itemsSetFamily[i].getNonKernel();

			auto print = [&](Item k)
			{
				os << "\t" << k.symbol << " -> ";
				int j(0);
				for (; j < k.body.size(); ++j)
				{
					if (k.pos == j)
					{
						os << ". ";
					}
					os << k.body[j] << ' ';
				}
				if (k.pos == j)
				{
					os << ". ";
				}
				os << "\n";
			};

			for (const auto &k : kernel)
			{
				print(k);
			}
			for (const auto &k : nonKernel)
			{
				print(k);
			}
		}
	}

	// 输出DFA等信息
	void printDFA(ostream &os) const
	{
		os << "DFA\n";
		os << "  状态个数：" << this->itemsSetFamily.size() << "\n";
		os << "  字符表个数" << (this->terminalNum + this->nonTerminalNum) << "\n";
		os << "  状态转换: \n";
		for (const auto &i : this->gotoTable)
		{
			os << "  (" << i.first.first << "," << i.first.second << ")->" << i.second << "\n";
		}
		os << "  开始状态: 0\n";
		os << "  结束状态集: { ";
		for (int i(0); i < this->itemsSetFamily.size(); ++i)
		{
			int temp;
			if (this->itemsSetFamily[i].isToReduced(temp))
				os << i << " ";
		}
		os << "}\n";
	}

	void printACTION(ostream &os)
	{
		//@TODO 验证是否LR(0)文法
		for (auto &g : this->itemsSetFamily)
		{
			int temp;
			if (g.isToReduced(temp))
			{
				if (g.getNonKernel().size())
					isLR0 = false;
			}
		}
		if (isLR0)
		{
			os << "  文法是LR(0)文法!\nLR(0)分析表\n Action:  \n\t";
			// 接着就是一波操作
			for (const auto &i : terminals)
			{
				os << i << "\t";
			}
			os << "#\n";
			for (int i(0); i < this->itemsSetFamily.size(); ++i)
			{
				os << "  " << i << "\t";
				// 若reduce，则没有shift的事情了，并且reduce需要的只有终结符
				int index;
				bool flag(false);
				if (itemsSetFamily[i].isToReduced(index))
				{
					int signal = -1;
					for (const auto &j : terminals)
					{
						auto kernel = itemsSetFamily[i].getKernel()[index];
						if (kernel.symbol == startSymbol) // ACC
							flag = true;
						else // not ACC
						{
							auto range = this->findRulesScope(kernel.symbol);
							for (auto k = range.first; k != range.second; ++k)
							{
								if (k->second.first == kernel.body)
								{
									os << "r" << k->second.second;
									signal = k->second.second;
								}
							}
						}
						os << "\t";
					}
					if (flag)
						os << "ACC";
					else
						os << "r" << signal;
					os << "\n";
					continue;
				}
				for (const auto &sym : terminals) //action 表
				{
					
				}
				os << "\n";
			}
		}
		else
		{
			os << "  文法不是LR(0)文法!\n";
		}
	}

	void printGOTO(ostream &os)
	{
		// 非终结符
		for (const auto sum : nonTerminals)
		{
		}
	}

	void parser(string str)
	{
		// 分析一个具体字符串
		if (isLR0)
		{
		}
		else
		{
		}
	}

private:
	// 增强文法
	__attribute__((unused)) void augment()
	{
		string aug_start_symbol = "S`";
		rules.insert(make_pair(aug_start_symbol, make_pair(vector<string>{startSymbol}, rules.size())));
		nonTerminals.insert(aug_start_symbol);
		nonTerminalNum = nonTerminals.size();
		startSymbol = aug_start_symbol;
	}

	// 制作项目集簇
	// 往右移动一个位置
	void makeItemsFamily()
	{
		auto production = *rules.find(startSymbol);
		Item item(startSymbol, 0, production.second.first);
		ITEMS t;
		t.push_back(item);
		ItemsSet itemsSet(t);
		itemsSet.closure(this);
		queue<pair<ItemsSet, int>> q;
		q.push(make_pair(itemsSet, itemsSetFamily.size()));
		itemsSetFamily.push_back(itemsSet);

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

			auto find = [&](const string &character, bool &flag, ITEMS &items) -> pair<string, bool>
			{
				string shiftSymbol;
				bool flag1 = false;
				for (const auto &kk : kkernels)
				{
					// 遍历得到可以shift的item
					int pos = kk.pos;
					auto body = kk.body;
					string symbol = kk.symbol;
					shiftSymbol = body[pos];
					pos += 1;
					if (shiftSymbol == character)
					{
						// 如果都没有，则需要在后续返回一个flag
						flag1 = true; //表示可以shift
						Item item1(symbol, pos, body);
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
				return std::make_pair(shiftSymbol, flag1);
			};

			for (const auto &nonTerminal : nonTerminals)
			{
				ITEMS items;
				bool flag = false;
				auto re = find(nonTerminal, flag, items);
				if (!re.second)
					continue;
				string shiftSymbol = re.first;

				ItemsSet itemsSet1(items);
				itemsSet1.closure(this);
				// 如果有一个项不规约，则可以继续入队列
				if (!itemsSet1.isAllToReduced())
				{
					q.push(make_pair(itemsSet1, itemsSetFamily.size()));
				}
				gotoTable[make_pair(index, nonTerminal)] = itemsSetFamily.size();
				if (!flag)
					itemsSetFamily.push_back(itemsSet1);
			}

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
				if (!itemsSet1.isAllToReduced())
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
	GrammarLR0 g(filename, in);
	string c, input;
	getline(in, c);
	getline(in, c);
	getline(in, input);
	cout << g << std::endl;
	g.parser(input);
	return 0;
}