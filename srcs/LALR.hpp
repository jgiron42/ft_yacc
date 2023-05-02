#ifndef FT_YACC_POC_LALR_HPP
#define FT_YACC_POC_LALR_HPP
#include <string>
#include <list>
#include <map>
#include <stdexcept>
#include <vector>
#include <deque>
#include <set>
#include <tuple>
#include <ranges>
#include <iostream>
#include <queue>
#include <stack>

class LALR {
public:
	struct Token {
		bool						is_terminal;
		int							value;
		int							precedence;
		enum Associativity {
			DEFAULT,
			RIGHT,
			LEFT,
			NONASSOC
		}							associativity;
	};
	struct Rule {
		std::string					symbol;
		std::vector<std::string>	syntax;
		std::string					precedence; // same-as precedence
		std::string					action;
	};
	struct State {
		struct StateRule {
			int						rule_id;
			int						pos;
			std::set<std::string>	lookahead;
			friend bool operator==(const StateRule &l, const StateRule &r) = default;
			friend bool operator<(const StateRule &l, const StateRule &r)
			{
				return l.rule_id < r.rule_id ||
				(l.rule_id == r.rule_id && l.pos < r.pos) ||
				(l.rule_id == r.rule_id && l.pos == r.pos && l.lookahead < r.lookahead);
			}
		};
		std::deque<StateRule>		rules;
		std::multimap<std::string, std::pair<int, int>>	shifts; // symbol, {next state, precedence}
		std::multimap<std::string, std::pair<int, int>>	reduces; // symbol, {rule, precedence}
		std::multimap<std::string, int>	gotos; // symbol, next state
		bool accept;
	};
private:
	std::map<std::string, Token>		tokens;
	std::set<int>						token_values;
	std::vector<Rule>					rules;
	std::multimap<std::string, int>		rules_by_symbol;
	std::deque<State>					states;
	std::queue<int>						updated_states;

	std::map<std::string, int>			map_terminals;
	std::map<std::string, int>			map_nonterminals;

	int sr_conflicts;
	int rr_conflicts;
public:
	LALR();
	void											build(const std::string &start);
	[[nodiscard]] std::vector<std::vector<int> >	get_table() const;
	[[nodiscard]] std::vector<std::string>			get_actions() const;
	[[nodiscard]] std::vector<int>					get_token_map() const;
	[[nodiscard]] std::vector<size_t>				get_reduce_table() const;
	[[nodiscard]] std::vector<int>					get_reduce_tokens() const;
	[[nodiscard]] int								get_terminals_limit() const;
	[[nodiscard]] int								get_accept() const;
	[[nodiscard]] std::map<std::string, int>		get_token_definitions() const;
	void											print(std::ostream &out = std::cout) const;
private:
	[[nodiscard]] int												get_token_val(const std::string &) const;
	void															build_state(State &);
	std::set<std::string>											get_lookahead(const std::string &non_terminal);
	void															merge(const State &new_state, int other_id);
	void															derivate_state_rules(State &state);
	[[nodiscard]] std::multimap<std::string, State::StateRule &>	map_rules_by_next_symbol(State &state) const;
	void															generate_reduces(State &state);
	void															generate_shifts_and_gotos(State &state);
	void															solve_conflicts(State &state);
	void															create_epsilons(State &state);
	int																get_rule_precedence(const Rule &);
protected:
	[[nodiscard]] bool												is_terminal(const std::string &sym) const;
	void				add_token(const std::string &name, Token &&);
	void				add_token_mapping(const std::string &);
	void				add_rule(Rule &&);
	static std::string	generate_symbol_name();
	int					generate_token_value();

	class DuplicatedDefinition : public std::runtime_error {
	public:
		DuplicatedDefinition(const std::string &s) : std::runtime_error("Duplicated definition: " + s) {}
	};

	class InternalError : public std::exception {};

	friend class Parser;
};

#endif
