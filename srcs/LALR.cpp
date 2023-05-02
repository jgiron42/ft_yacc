#include "LALR.hpp"
#include "utils.hpp"
#include <unordered_set>

LALR::LALR() {
	add_token_mapping("$end"); // 0
	add_token_mapping("$epsilon"); // 1
	add_rule({"$initial"});
	this->states.push_back({.rules = {{0, 0, {}}}});
}

std::string LALR::generate_symbol_name() {
	static int current = 1;
	return "@" + std::to_string(current++);
}

int LALR::generate_token_value() {
	static int current = 257;
	while (token_values.contains(current))
		current++;
	return current++;
}

void LALR::add_token(const std::string &name, Token &&t) {
	if (auto it = this->tokens.find(name); it != this->tokens.end())
	{
		if (is_terminal(name))
			throw DuplicatedDefinition(name);
		else
			return;
	}
	if (t.value == -1)
		t.value = generate_token_value();
	this->tokens.insert(std::pair<std::string, Token>(name, t));
	this->token_values.insert(t.value);
}

void LALR::add_token_mapping(const std::string &t) {
	if (is_terminal(t))
		this->map_terminals.insert(std::pair<std::string, int>(t, this->map_terminals.size()));
	else
		this->map_nonterminals.insert(std::pair<std::string, int>(t, this->map_nonterminals.size()));
}

void LALR::add_rule(Rule &&r) {
	this->rules.push_back(r);
	this->rules_by_symbol.insert(std::pair<std::string, int>(r.symbol, (int)this->rules.size() - 1));
	for (auto &s : r.syntax)
		this->add_token_mapping(s);
}

void LALR::build_state(State &state) {
	state.accept = false;

	derivate_state_rules(state);

	generate_reduces(state);

	generate_shifts_and_gotos(state);
}

void LALR::generate_shifts_and_gotos(LALR::State &state) {
	std::multimap<std::string, State::StateRule &> mm = map_rules_by_next_symbol(state);

	if (mm.empty())
		return;

	// iterate over all the equivalent range of mm
	for (auto lower_bound = mm.begin(), upper_bound = mm.upper_bound(lower_bound->first);
		lower_bound != mm.end();
		std::swap(lower_bound, upper_bound),
		upper_bound = mm.upper_bound(upper_bound->first))
	{
		std::deque<State::StateRule> new_state_rules;
		for (auto &[sym, sr] : std::ranges::subrange(lower_bound, upper_bound))
			new_state_rules.push_back({sr.rule_id, sr.pos + 1, sr.lookahead});

		// equivalent_rule_sets are defined as the sets of rules that share the same core items
		auto equivalent_rule_sets = [](const std::deque<State::StateRule> &l, const std::deque<State::StateRule> &r)
		{
			return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end(),
				[](const State::StateRule &l, const State::StateRule &r) -> bool
				{
					return l.rule_id < r.rule_id || (l.rule_id == r.rule_id && l.pos < r.pos);
				});
		};
		static std::map<std::deque<State::StateRule>, int, decltype(equivalent_rule_sets)> filter(equivalent_rule_sets);

		int dst;
		// find states with equivalent rule sets
		auto fit = filter.find(new_state_rules);
		if (fit == filter.end()) // no equivalent, create a new state with this rule sets
		{
			states.push_back({new_state_rules});
			filter[new_state_rules] = (int)states.size() - 1;
			dst = (int)states.size() - 1;
		}
		else // there is an equivalent state
		{
			// merge it
			dst = fit->second;
			merge({new_state_rules}, fit->second);
		}

		// finally register all the shifts and gotos to our dst
		for (auto &[sym, sr] : std::ranges::subrange(lower_bound, upper_bound))
		{
			if (is_terminal(sym))
				state.shifts.insert({sym, {dst, get_rule_precedence(this->rules[sr.rule_id])}});
			else
				state.gotos.insert({sym, dst});
		}

	}
}

void LALR::generate_reduces(LALR::State &state) {
	for (auto &sr : state.rules)
		if (sr.pos == rules[sr.rule_id].syntax.size()) // reduce
		{
			int precedence = get_rule_precedence(this->rules[sr.rule_id]);
			if (sr.rule_id == rules_by_symbol.find("$initial")->second)
				state.accept = true;
			else for (auto &l : sr.lookahead)
			{
				state.reduces.insert({l, {sr.rule_id, precedence}});
			}
		}
}

std::multimap<std::string, LALR::State::StateRule &> LALR::map_rules_by_next_symbol(LALR::State &state) const {
	std::multimap<std::string, State::StateRule &> ret;
	for (auto & sr : state.rules)
		if (sr.pos != rules[sr.rule_id].syntax.size())
			ret.insert({rules[sr.rule_id].syntax[sr.pos], sr});
	return ret;
}

void LALR::derivate_state_rules(LALR::State &state) {
	for (size_t i = 0; i < state.rules.size(); i++) {
		auto &sr = state.rules[i];
		auto &r = rules[sr.rule_id];
		if (sr.pos < r.syntax.size() && !is_terminal(r.syntax[sr.pos])) {
			auto symbol_rules = rules_by_symbol.equal_range(r.syntax[sr.pos]);
			if (symbol_rules.first == symbol_rules.second)
				throw std::runtime_error("nonterminal " + r.syntax[sr.pos] + " has no production rule");
			std::set<std::string> lookahead = {};
			std::set<std::string> non_terminals = {};
			if (sr.pos + 1 < r.syntax.size())
				lookahead = get_lookahead(r.syntax[sr.pos + 1]);
			else if (!sr.lookahead.empty())
				lookahead = sr.lookahead;
			for (auto &symbol_rule: std::ranges::subrange(symbol_rules.first, symbol_rules.second))
			{
				auto it = std::find_if(state.rules.begin(), state.rules.end(),
									   [&symbol_rule](State::StateRule &sr2) -> bool {
										   return sr2.rule_id == symbol_rule.second && sr2.pos == 0;
									   }
				);
				if (it == state.rules.end())
					state.rules.push_back({symbol_rule.second, 0, lookahead});
				else
					it->lookahead.insert(lookahead.begin(), lookahead.end());
			}
		}
	}
}

void LALR::merge(const LALR::State &new_state, const int other_id) {
	auto &other_state = states[other_id];
	bool updated = false;
	for (int i = 0; i < new_state.rules.size(); i++)
		for (auto &s : new_state.rules[i].lookahead)
			updated |= other_state.rules[i].lookahead.insert(s).second;
	if (updated)
		updated_states.push(other_id);
}

std::set<std::string> LALR::get_lookahead(const std::string &non_terminal) {
	std::stack<std::string> stack;
	std::set<std::string>	ret;
	std::set<std::string>	filter;
	static std::map<std::string, std::set<std::string> > cache;

	if (auto it = cache.find(non_terminal); it != cache.end())
		return it->second;
	stack.push(non_terminal);
	while (!stack.empty())
	{
		std::string current = stack.top();
		stack.pop();
		if (is_terminal(current))
			ret.insert(current);
		else if (!filter.count(current))
		{
			auto rules = this->rules_by_symbol.equal_range(current);
			for (auto &r : std::ranges::subrange(rules.first, rules.second))
				if (!this->rules[r.second].syntax.empty())
					stack.push(this->rules[r.second].syntax.front());
			filter.insert(current);
		}
	}
	cache[non_terminal] = ret;
	return ret;
}

// if the parser is in the state where
// the only action possible is a reduce
// we don't need to check the lookahead character (and block)
// in this case we create an epsilon reduce.
void LALR::create_epsilons(State &state) {
	if (!state.shifts.empty() || state.reduces.empty())
		return;
	int target = state.reduces.begin()->second.first;
	for (auto &r : state.reduces)
		if (r.second.first != target)
			return;
	state.reduces.clear();
	state.reduces.insert({"$epsilon", {target, 0}}); // precedence doesn't matter at this stage
}

void LALR::solve_conflicts(State &state) {
	using namespace std::views;
	using namespace std::ranges;
	std::set<std::string> keys;
	auto r = std::views::keys(state.shifts);
	keys.insert(r.begin(), r.end());
	r = std::views::keys(state.reduces);
	keys.insert(r.begin(), r.end());

	for (auto &k : keys)
	{
		auto reduces_range = state.reduces.equal_range(k);
		auto shifts_range = state.shifts.equal_range(k);
		std::list<decltype(state.reduces)::value_type> reduces(reduces_range.first, reduces_range.second);
		std::list<decltype(state.reduces)::value_type> shifts(shifts_range.first, shifts_range.second);
		reduces.sort();
		reduces.unique();
		shifts.sort();
		shifts.unique();
		size_t nreduces = reduces.size();
		size_t nshifts = shifts.size();
		if (nreduces > 0 && nshifts > 0) // shift reduce conflict!
		{
			int rprec = reduces.begin()->second.second;
			int sprec = shifts.begin()->second.second;
			if (rprec < sprec)
				state.reduces.erase(k);
			else if (rprec > sprec)
				state.shifts.erase(k);
			else
			{
				switch (this->tokens[k].associativity)
				{
					case LALR::Token::LEFT:
						state.shifts.erase(k);
						break;
					case LALR::Token::RIGHT:
						state.reduces.erase(k);
						break;
					case LALR::Token::NONASSOC:
						state.reduces.erase(k);
						state.shifts.erase(k);
						break;
					default:
						state.reduces.erase(k);
						this->sr_conflicts++;
				}
			}
			if (nreduces > 1)
				this->rr_conflicts++;
		}
		else if (nreduces > 1) // reduce reduce conflict
		{
			int min = this->rules.size();
			// find the lowest rule (first in file)
			for (auto e : reduces)
				if (e.second.first < min)
					min = e.second.first;
			// delete all other reduces
			for (auto it = reduces_range.first; it != reduces_range.second; it++)
				if (it->second.first == min)
				{
					auto tmp = *it;
					state.reduces.erase(it->first);
					state.reduces.insert(tmp);
					break;
				}
			this->rr_conflicts++;
		}

		// clean false conflicts (same symbol same rule)
		reduces_range = state.reduces.equal_range(k);
		shifts_range = state.shifts.equal_range(k);
		if (reduces_range.first != reduces_range.second)
			state.reduces.erase(++reduces_range.first, reduces_range.second);
		if (shifts_range.first != shifts_range.second)
			state.shifts.erase(++shifts_range.first, shifts_range.second);
	}
}

void LALR::build(const std::string &start) {
	this->rules[0].syntax = {start, "$end"};
	this->tokens.insert({"error", {true, 256}});
	this->sr_conflicts = 0;
	this->rr_conflicts = 0;
	add_token_mapping("error");
	for (size_t i = 0; i < this->states.size() && i < 50; i++)
	{
//		std::cout << "State: " << i << std::endl;
		build_state(this->states[i]);
	}
	for (;!this->updated_states.empty() && this->states.size() < 50; this->updated_states.pop())
	{
		this->generate_reduces(this->states[this->updated_states.front()]);
		this->generate_shifts_and_gotos(this->states[this->updated_states.front()]);
	}
	for (auto &s : this->states)
	{
		this->solve_conflicts(s);
		this->create_epsilons(s);
	}
	if (this->sr_conflicts)
		std::cout << this->sr_conflicts << " shift/reduce conflicts" << std::endl;
	if (this->rr_conflicts)
		std::cout << this->rr_conflicts << " reduce/reduce conflicts" << std::endl;
}

int LALR::get_token_val(const std::string &t) const {
	if (is_terminal(t))
		return this->map_terminals.find(t)->second;
	else
		return (int)this->map_terminals.size() + this->map_nonterminals.find(t)->second;
}

bool LALR::is_terminal(const std::string &sym) const {
	if (sym[0] == '\'' || sym[0] == '$') // literal or internal
		return true;
	if (const auto &p = this->tokens.find(sym); p != this->tokens.end())
		return p->second.is_terminal;
	return false;
}

int LALR::get_rule_precedence(const Rule &r) {
	if (!r.precedence.empty())
		return this->tokens[r.precedence].precedence;
	int prec = -1;
	for (auto &sym : r.syntax)
		if (is_terminal(sym))
			prec = this->tokens[sym].precedence;
	return prec;
}

std::vector<std::vector<int>> LALR::get_table() const {
	std::vector<std::vector<int>> ret(this->states.size(), std::vector<int>(this->map_terminals.size() + this->map_nonterminals.size(), 0));
	auto it = ret.begin();
	for (auto &s : this->states)
	{
		std::vector<int> &current = *it;
//		std::cout << "reduces: " << std::endl;
//		std::cout << current.size() << " " <<  this->map_terminals.size() + this->map_nonterminals.size()  << std::endl;
		for (auto &p : s.reduces) {
//			std::cout << p.first << std::endl;
//			std::cout << get_token_val(p.first) << std::endl;
//			std::cout << p.second << std::endl;
			current[get_token_val(p.first)] = -p.second.first;
		}
//		std::cout << "shifts: " << std::endl;
		for (auto &p : s.shifts)
		{
//			std::cout << p.first << std::endl;
//			std::cout << get_token_val(p.first) << std::endl;
//			std::cout << p.second << std::endl;
			current[get_token_val(p.first)] = p.second.first;
		}
//		std::cout << "gotos: " << std::endl;
		for (auto &p : s.gotos)
		{
//			std::cout << p.first << std::endl;
//			std::cout << get_token_val(p.first) << std::endl;
//			std::cout << p.second << std::endl;
			current[get_token_val(p.first)] = p.second;
		}
		it++;
	}
	return ret;
}

std::vector<size_t>	LALR::get_reduce_table() const {
	auto v = rules | std::views::transform([](const Rule &r){return r.syntax.size();});
	return {v.begin(), v.end()};
}

std::vector<int>	LALR::get_reduce_tokens() const {
	auto v = rules | std::views::transform([this](const Rule &r) -> int {return get_token_val(r.symbol);});
	return {v.begin(), v.end()};
}

std::vector<std::string>	LALR::get_actions() const {
	auto v = rules | std::views::transform([](const Rule &r){return r.action;});
	return {v.begin(), v.end()};
}

std::vector<int> LALR::get_token_map() const {
	std::vector<int> ret;
	for (auto &p : this->map_terminals)
	{
		if (p.first[0] == '$')
			continue;
		int n;
		if (p.first[0] == '\'')
			n = parse_literal(p.first);
		else if (const auto &t = this->tokens.find(p.first); t != this->tokens.end())
			n = this->tokens.find(p.first)->second.value;
		else
			throw InternalError();
		if (n >= ret.size())
			ret.resize(n + 1);
//		std::cout << p.first << " -> " <<  n << std::endl;
		ret[n] = get_token_val(p.first);
	}

	for (auto &p : this->map_nonterminals)
	{
		if (p.first[0] == '$')
			continue;
		int n;
		if (const auto &t = this->tokens.find(p.first); t != this->tokens.end())
			n = t->second.value;
		else
			throw InternalError();
		if (n > ret.size())
			ret.resize(n + 1);
//		std::cout << p.first << " -> " <<  n << std::endl;
		ret[n] = get_token_val(p.first);
	}

	return ret;
}

std::map<std::string, int> LALR::get_token_definitions() const {
	auto ret = tokens |
			std::views::filter([](const std::pair<std::string, Token> &p){return p.second.is_terminal && p.first[0] != '\'';}) |
			std::views::transform([](const std::pair<std::string, Token> &p) {return std::pair(p.first, p.second.value);});
	return {ret.begin(), ret.end()};
}

int LALR::get_terminals_limit() const {return (int)this->map_nonterminals.size();}

int LALR::get_accept() const {
	for (size_t i = 0; i < this->states.size(); i++)
		if (this->states[i].accept)
			return (int)i;
	return -1;
}

void LALR::print(std::ostream &out) const {
	out << "Grammar:" << std::endl << std::endl;
	for (auto &r : this->rules)
	{
		out << "\t" << r.symbol << " : ";
		for (auto &component : r.syntax)
			out << component << ' ';
		out << std::endl;
	}
	out << std::endl;

	out << "Tokens" << std::endl;
	for (auto &t : tokens)
		out << "\t" << t.first << " (" << t.second.value << "|" << this->get_token_val(t.first) << ")" <<  std::endl;
	out << std::endl;

	for (size_t i = 0; i < this->states.size(); i++)
	{
		out << "State " << i << ":" << std::endl;
		for (auto &sr : this->states[i].rules)
		{
			out << "\t" << sr.rule_id << " " << this->rules[sr.rule_id].symbol << ": ";
			for (int j = 0; j < this->rules[sr.rule_id].syntax.size(); j++)
			{
				if (j == sr.pos)
					out << "• ";
				out << this->rules[sr.rule_id].syntax[j];
				if (j + 1 != this->rules[sr.rule_id].syntax.size())
					out << " ";
			}
			if (sr.pos == this->rules[sr.rule_id].syntax.size())
				out << " •";
			if (!sr.lookahead.empty()) {
				out <<" [";
				bool first = true;
				for (auto &sym: sr.lookahead) {
					if (!first)
						out << ", ";
					out << sym;
					first = false;
				}
				out << "]";
			}
			out << std::endl;
		}
		out << std::endl;
		for (auto &s : this->states[i].shifts)
			out << "\t" << s.first << " shift, and go to state " << s.second.first << std::endl;
		out << std::endl;
		for (auto &r : this->states[i].reduces)
			out << "\t" << r.first << " reduce using rule " << r.second.first << " (" << this->rules[r.second.first].symbol << ")" << std::endl;
		out << std::endl;
		for (auto &s : this->states[i].gotos)
			out << "\t" << s.first << " go to state " << s.second << std::endl;
		out << std::endl;
	}
}
