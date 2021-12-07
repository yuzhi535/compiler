/**
 * @file main.cpp
 * @author Zhou YuXi (周誉喜) (zhouyuxi@stu.zzu.edu.cn)
 * @brief LR1语法分析器的设计与实现
 * @version 0.1
 * @date 2021-11-28
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
#include <string>
#include <memory>

using std::cout;
using std::fstream;
using std::ifstream;
using std::make_pair;
using std::make_shared;
using std::map;
using std::multimap;
using std::ostream;
using std::pair;
using std::queue;
using std::set;
using std::shared_ptr;
using std::stack;
using std::string;
using std::vector;

struct Item;

class ItemsSet;

class Grammar;

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

/**
 * @brief 语法生成树的节点
 * @note 利用二叉树构建多叉树
 * @note 左子右兄法表示多叉树
 */
class Node
{
public:
	Node(string c) : symbol(c) {}
	Node(string c, Node &p, Node &l, Node &r, bool color = false)
		: Node(c)
	{
		parent = make_shared<Node>(p);
		lChild = make_shared<Node>(l);
		rChild = make_shared<Node>(r);
	}
	// 创建父节点
	shared_ptr<Node> createParent(shared_ptr<Node> node)
	{
		// this->parent = make_shared<Node>(node);
		node->createChild(make_shared<Node>(*this));
		return node;
	}
	// 创建子节点  注意节点是创建的时候决定是否兄弟还是孩子，即红还是黑 其实红黑颜色是在遍历的时候非常重要
	shared_ptr<Node> createChild(shared_ptr<Node> node)
	{
		this->lChild = node;
		node->parent = make_shared<Node>(*this);
		return node;
	}
	// 创建兄弟节点
	shared_ptr<Node> createSibling(shared_ptr<Node> node)
	{
		this->rChild = node;
		node->parent = make_shared<Node>(*this);
		return node;
	}

	const string &printSymbol() { return symbol; }

private:
	string symbol;
	shared_ptr<Node> parent, lChild, rChild;
};

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
		// auto back = firstSet.find(symbol);
		// if (back != firstSet.end())
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
	FIRSTSET firstSet; // first集合
};

/**
 * @brief 不完整的项目集
 * 龙书的说法上分kernel 和 nonkernel
 * T -> .TE
 * T -> T.E
 *
 */
struct Item
{
	Item(string sym, int position, vector<string> b, string t_lookahead = "#")
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
						const pair<string, pair<vector<string>, string>> &b) const
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
			const int &pos = items[index].dotPos;
			if (pos < items[index].body.size()) // 如果还有移动的空间
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

	// 该项目集是否可被规约，在LR(0)里面与上一个函数其实是一样的，但在LR1不一样
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
 * @TODO parse
 * @TODO 检查是否LR1文法
 */
class GrammarLR1 : public Grammar
{
	friend ostream &operator<<(ostream &os, GrammarLR1 &other)
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

		other.printItemsFamily(os);

		os << "[LR(1) analytical table]\n";

		other.printACTION(os);

		other.printGOTO(os);

		if (other.isLR1)
			os << "文法是 LR(1) 文法！\n";

		return os;
	}

public:
	explicit GrammarLR1(ifstream &in) : Grammar(in)
	{
		root = make_shared<Node>(startSymbol);
		isLR1 = true;
		getAllFirstSet();
		makeItemsSetFamily();
	}

	// 输出action
	// 判断是否LR1
	void printACTION(ostream &os) const
	{
		if (isLR1)
		{
			os << "  Action:  \n\t";
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
				if (itemsSetFamily[i].isToBeReduced(index))
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
								if (k->second.first == kernel.body && (j == kernel.lookahead))
								{
									os << "r" << k->second.second;
									signal = k->second.second;
								}
								if (k->second.first == kernel.body && kernel.lookahead == "#")
								{
									signal = k->second.second;
								}
							}
						}
						os << "\t";
					}
					if (flag)
					{
						// gotoTable[make_pair()]
						os << "ACC";
					}
					else
					{
						os << "r" << signal;
					}
					os << "\n";
					continue;
				}
				for (const auto &sym : terminals) // action 表
				{
					auto result = gotoTable.find(make_pair(i, sym));
					if (result != gotoTable.end())
					{
						os << "s" << result->second;
					}
					os << "\t";
				}
				os << "\n";
			}
		}
		// else
		// {
		// 	os << "  文法不是LR(1)文法!\n";
		// }
	}

	// 输出goto
	void printGOTO(ostream &os) const
	{
		// 非终结符
		os << "Goto:\n";
		for (const auto sym : nonTerminals)
		{
			if (sym != startSymbol) // 准确的说 应该不是增强文法的开始符号
				os << "   " << sym << "\t";
		}
		os << "\n";
		for (int i(0); i < this->itemsSetFamily.size(); ++i)
		{
			os << i << "  ";
			for (const auto &sym : nonTerminals)
			{
				auto result = gotoTable.find(make_pair(i, sym));
				if (result != gotoTable.end())
				{
					os << result->second;
				}
				os << "\t";
			}
			os << "\n";
		}
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
					if (k.dotPos == j)
					{
						os << ". ";
					}
					os << k.body[j] << ' ';
				}
				if (k.dotPos == j)
				{
					os << ". ";
				}
				os << ", " << k.lookahead << "\n";
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

	// 总控程序
	// 我这里入栈的是状态集的索引，不是具体的符号，有点偷懒，但是确实可以
	void parser(vector<string> &input)
	{
		// 分析一个具体字符串
		if (isLR1)
		{
			cout << "[parsing]\n";
			cout << "栈顶\t输入\t查表\t动作\t注\n";
			// 栈  入栈 出栈
			vector<int> inStack;		// 我觉得只用状态集的索引就行了
			vector<string> symbolStack; //符号栈
			inStack.push_back(0);
			int id = 0;
			symbolStack.push_back("#");
			// symbolStack.push_back(input[id]);
			while (!inStack.empty())
			{
				int item = inStack[inStack.size() - 1];
				cout << item << " ";

				int index;
				bool hasReduced = false;
				bool isNotShifted = false;
				if (this->itemsSetFamily[item].isToBeReduced(index))
				{
					auto kernel = itemsSetFamily[item].getKernel()[index];
					auto range = this->findRulesScope(kernel.symbol);
					for (auto k = range.first; k != range.second; ++k)
					{
						if (k->second.first == kernel.body)
						{
							if (input[id] == kernel.lookahead)
							{
								cout << symbolStack[symbolStack.size() - 1] << "\t";
								cout << input[id] << "\t"; //输入

								cout << "出栈: " << kernel.body.size() << " 个符号和状态 ";

								int len(0);
								for (; len < kernel.body.size(); ++len)
								{
									inStack.pop_back();
									symbolStack.pop_back();
								}
								if (inStack.empty() && id < input.size() - 1)
								{
									std::cerr << "失败\n";
									exit(-1);
								}
								int target = gotoTable.at(make_pair(inStack[inStack.size() - 1], kernel.symbol));
								cout << "进栈: " << target << " " << kernel.symbol << "\t";
								symbolStack.push_back(kernel.symbol);
								cout << kernel.symbol << " -> ";
								for (const auto &body : kernel.body)
									cout << body << " ";
								cout << "\n";
								if (inStack.size() == 1 && id == input.size() - 1)
								{
									cout << inStack[inStack.size() - 1] << " " << symbolStack[symbolStack.size() - 1] << "\t";
									symbolStack.pop_back();
									cout << input[id] << "\tACC";
									std::cout << "\t成功接收\n";
									inStack.clear();
									hasReduced = true;
									break;
								}
								inStack.push_back(target);
								hasReduced = true;
								break;
							}
						}
					}
				}
				if (!hasReduced)
				{
					// shift (移进)
					auto k = gotoTable.find(make_pair(item, input[id]));
					if (k != gotoTable.end())
					{
						cout << symbolStack[symbolStack.size() - 1];
						// 输入
						cout << "\t" << input[id] << "\t"; //输入
						cout << "进栈：" << k->second << ", " << input[id] << "\n";
						symbolStack.push_back(input[id]);
						// inStack.push_back(string(*ptr, 1));
						item = k->second;
						inStack.push_back(item);
						id += 1; //字符串向后移动
						isNotShifted = true;
					}
					else
					{
						std::cerr << "出现错误!\n";
						isLR1 = false;
						break;
					}
				}
				if (!hasReduced && !isNotShifted)
				{
					std::cerr << "错误!\n";
					isLR1 = false;
					exit(-1);
				}
			}
		}
		else
		{
			cout << "不是LR1文法，无法分析\n";
		}
	}

	// 输出语法分析树
	void printAST(vector<string> &input)
	{
		cout << "语法分析树\n";
		buildGrammarAnalysisTree(input);
		if (isLR1)
		{
			// @TODO 输出
			shared_ptr<Node> node = root;
			while (node) {

			}
		}
		else
		{
			cout << "fuck you! you are wrong!!!\n";
		}
	}

private:
	// 语法分析树
	// @TODO 构建语法分析树
	void buildGrammarAnalysisTree(vector<string> &input)
	{
		if (input.empty())
		{
			cout << "字符串空!\n";
			return;
		}
		if (isLR1)
		{
			vector<int> inStack;		// 我觉得只用状态集的索引就行了
			vector<string> symbolStack; //符号栈
			inStack.push_back(0);
			symbolStack.push_back("#");
			int id = 0;
			symbolStack.push_back(input[id]);

			vector<shared_ptr<Node>> symbolVec;
			// symbolVec.push_back(root);

			while (!inStack.empty())
			{
				int item = inStack[inStack.size() - 1];

				int index;
				bool hasReduced = false;
				bool isNotShifted = false;
				if (this->itemsSetFamily[item].isToBeReduced(index))
				{
					auto kernel = itemsSetFamily[item].getKernel()[index];
					auto range = this->findRulesScope(kernel.symbol);
					for (auto k = range.first; k != range.second; ++k)
					{
						if (k->second.first == kernel.body)
						{
							if (input[id] == kernel.lookahead)
							{
								// cout << kernel.body[kernel.body.size() - 1] << "\t";
								// cout << input[id] << "\t"; //输入

								// cout << "出栈: " << kernel.body.size() << " 个符号和状态 ";

								int len(0);
								int symbol_sz(0);
								for (; len < kernel.body.size(); ++len)
								{
									inStack.pop_back();
									symbolStack.pop_back();

									// 这些是要添加到树上
									++symbol_sz;
								}
								if (inStack.empty() && id < input.size() - 1)
								{
									std::cerr << "失败\n";
									isLR1 = false;
									exit(-1);
								}
								int target = gotoTable.at(make_pair(inStack[inStack.size() - 1], kernel.symbol));
								symbolStack.push_back(kernel.symbol);
								// ----------------------------------------
								// 生成树
								shared_ptr<Node> parent = make_shared<Node>(kernel.symbol);
								int ind = symbolVec.size() - 1;
								bool isChild = true;
								shared_ptr<Node> backup;
								for (int symbol_index(0); symbol_index < symbol_sz; ++symbol_index)
								{
									auto temp = symbolVec[ind - symbol_index];
									if (isChild)
									{
										backup = parent->createChild(temp);
										isChild = false;
									}
									else
										backup = backup->createSibling(temp);
									symbolVec.pop_back();
								}
								symbolVec.push_back(parent);

								// ----------------------------------------

								// cout << kernel.symbol << " -> ";
								// for (const auto &body : kernel.body)
								// 	cout << body << " ";
								// cout << "\n";
								if (inStack.size() == 1 && id == input.size() - 1)
								{
									// @TODO 注意要跟根节点有联系
									// -------------------------------------------
									root = symbolVec[0];
									symbolVec.pop_back();
									symbolStack.pop_back();
									// -------------------------------------------
									// cout << inStack[inStack.size() - 1] << "\t";
									// cout << input[id] << "\tACC";
									// cout << "\t成功接收\n";
									inStack.clear();
									hasReduced = true;
									break;
								}
								inStack.push_back(target);
								hasReduced = true;
								break;
							}
						}
					}
				}
				if (!hasReduced)
				{
					// shift (移进)
					auto k = gotoTable.find(make_pair(item, input[id]));
					if (k != gotoTable.end())
					{
						// 输入
						// cout << "\t" << input[id] << "\t"; //输入
						symbolStack.push_back(input[id]); // 进入符号栈
						symbolVec.push_back(make_shared<Node>(Node(input[id])));
						// cout << "进栈：" << k->second << ", " << input[id] << "\n";
						// inStack.push_back(string(*ptr, 1));
						// symbolStack.pop_back();
						item = k->second;
						inStack.push_back(item);
						id += 1; //字符串向后移动
						isNotShifted = true;
					}
					else
					{
						std::cerr << "出现错误!\n";
						break;
					}
				}
				if (!hasReduced && !isNotShifted)
				{
					std::cerr << "错误!\n";
					exit(-1);
				}
			}
		}
		else
		{
			cout << "不是LR1文法，无法分析\n";
		}
	}

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
					string shiftSymbol = body[pos];
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
				return make_pair(character, flag1);
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
				if (!flag)
				{
					gotoTable[make_pair(index, nonTerminal)] = itemsSetFamily.size();
					itemsSetFamily.push_back(itemsSet1);
				}
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

	ITEMFAMILY itemsSetFamily;			   // 状态集簇
	map<pair<int, string>, int> gotoTable; // 用于记录状态集之间的去向，在龙书里面ACTION 和 GOTO 统一为GOTO，所以如此命名
	bool isLR1;
	shared_ptr<Node> root;
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
	g.parser(ans);
	g.printAST(ans);
	return 0;
}