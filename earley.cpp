#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <map>

class Earley {
 public:
  Earley() = default;

  class Grammar {
   public:

    Grammar() = default;

    Grammar(std::unordered_set<char> terminals, std::unordered_set<char> non_terminals)
        : terminals(std::move(terminals)), non_terminals(std::move(non_terminals)) {
    }

    struct GrammarRule {
     public:
      GrammarRule() = default;

      GrammarRule(std::string rule_name, std::string rule) : rule_name(std::move(rule_name)), rule(std::move(rule)) {

      }

      [[nodiscard]] const std::string &GetRuleName() const {
        return rule_name;
      }

      [[nodiscard]] const std::string &GetRule() const {
        return rule;
      }

      [[nodiscard]] uint64_t RuleSize() const {
        return rule.size();
      }

     private:
      std::string rule_name;
      std::string rule;
    };

    void InsertRule(const std::string &rule) {
      states.emplace_back(rule.substr(0, 1), rule.substr(3, rule.size() - 3));
    }

    void SetStart(std::string start) {
      states.emplace_back("#", std::move(start));
      start_state = &states.back();
      non_terminals.insert('#');
    }

    const GrammarRule &GetStart() const {
      return *start_state;
    }

    uint64_t GetStartInd() const {
      return states.size() - 1;
    }

    const std::vector<GrammarRule> &GetRules() const {
      return states;
    }

    bool IsTerminal(char c) {
      return terminals.count(c) != 0;
    }

    bool IsNonTerminal(char c) {
      return non_terminals.count(c) != 0;
    }

   private:
    std::vector<GrammarRule> states;
    std::unordered_set<char> terminals;
    std::unordered_set<char> non_terminals;
    GrammarRule *start_state{};

  };

  void Fit(Grammar G) {
    grammar = std::move(G);
  };

  bool Predict(const std::string &word) {
    D.clear();
    D_new.clear();
    D_new.resize(word.size() + 1);
    D.resize(word.size() + 1);
    const auto &gr_rules = grammar.GetRules();
    D_new[0].emplace(grammar.GetStartInd(), 0, 0);
    for (uint64_t i = 0; i <= word.size(); ++i) {
      while (!D_new[i].empty()) {
        std::vector<EarleyState> to_remove;
        for (const auto &state : D_new[i]) {
          to_remove.push_back(state);
          if (state.index != gr_rules[state.rule_ind].RuleSize()) {
            if (grammar.IsNonTerminal(gr_rules[state.rule_ind].GetRule()[state.index])) {
              Predict(i, state);
            } else {
              Scan(i, word, state);
            }
          } else {
            Complete(i, state);
          }
        }
        for (const auto &state : to_remove) {
          D_new[i].erase(state);
          D[i].insert(state);
        }
        to_remove.clear();
      }
    }
    for (const auto &state : D[word.size()]) {
      if (state.rule_ind == grammar.GetStartInd() && state.predicted == 0 && state.index == 1) {
        return true;
      }
    }
    return false;
  };

 private:

  struct EarleyState {
   public:

    EarleyState() = default;

    explicit EarleyState(uint64_t rule_ind,
                         int64_t index = 0,
                         uint64_t predicted = 0,
                         uint64_t par_rule = 0,
                         int64_t par_ind = 0,
                         uint64_t par_predicted = 0,
                         uint64_t d_ind = 0)
        : rule_ind(rule_ind),
          index(index),
          predicted(predicted),
          par_rule(par_rule),
          par_ind(par_ind),
          par_predicted(par_predicted) {
    }

    bool operator==(const EarleyState &other) const {
      return index == other.index && predicted == other.predicted && rule_ind == other.rule_ind
          && par_rule == other.par_rule && par_ind == other.par_ind && par_predicted == other.par_predicted;
    }

    bool operator!=(const EarleyState &other) const {
      return !(*this == other);
    }

    uint64_t rule_ind = 0;
    int64_t index = 0;
    uint64_t predicted = 0;
    uint64_t par_rule = 0;
    int64_t par_ind = 0;
    uint64_t par_predicted = 0;
  };

  struct StateHasher {
    size_t operator()(const EarleyState &state) const {
      return std::hash<uint64_t>()(state.rule_ind);
    }
  };

  void Scan(int64_t index, const std::string &word, const EarleyState &state) {
    const auto &gr_rule = grammar.GetRules()[state.rule_ind];
    if (gr_rule.GetRule()[state.index] == word[index]) {
      D_new[index + 1].insert(EarleyState(state.rule_ind,
                                          state.index + 1,
                                          state.predicted,
                                          state.index,
                                          state.index,
                                          state.par_predicted,
                                          index));
    }
  }

  void Complete(int64_t index, const EarleyState &state) {
    for (const auto &new_state : D_new[state.predicted]) {
      const auto &gr_rule = grammar.GetRules()[new_state.rule_ind];
      if (new_state.index == gr_rule.RuleSize()
          || gr_rule.GetRule()[new_state.index] != grammar.GetRules()[state.rule_ind].GetRuleName()[0]) {
        continue;
      }
      EarleyState to_add
          (new_state.rule_ind, new_state.index + 1, new_state.predicted, state.rule_ind, state.index, state.predicted);
      if (D[index].count(to_add) == 0) {
        D_new[index].insert(to_add);
      }
    }
    for (const auto &new_state : D[state.predicted]) {
      const auto &gr_rule = grammar.GetRules()[new_state.rule_ind];
      if (new_state.index == gr_rule.RuleSize()
          || gr_rule.GetRule()[new_state.index] != grammar.GetRules()[state.rule_ind].GetRuleName()[0]) {
        continue;
      }
      EarleyState to_add
          (new_state.rule_ind, new_state.index + 1, new_state.predicted, state.rule_ind, state.index, state.predicted);
      if (D[index].count(to_add) == 0) {
        D_new[index].insert(to_add);
      }
    }
  }

  void Predict(int64_t index, const EarleyState &state) {
    const auto &gr_rules = grammar.GetRules();
    for (uint64_t i = 0; i < gr_rules.size(); ++i) {
      if (gr_rules[state.rule_ind].GetRule()[state.index] != gr_rules[i].GetRuleName()[0]) {
        continue;
      }
      EarleyState to_add(i, 0, index, state.rule_ind, state.index, state.predicted);
      if (D[index].count(to_add) == 0) {
        D_new[index].insert(to_add);
      }
    }
  }

  Grammar grammar;
  std::vector<std::unordered_set<EarleyState, StateHasher>> D;
  std::vector<std::unordered_set<EarleyState, StateHasher>> D_new;

};

int main() {
  std::cout << "Insert amount of not-term symbols, amount of term-symbols and amount of rules in your grammar:\n";
  int64_t non_term_amount;
  int64_t term_amount;
  int64_t rules_amount;
  std::cin >> non_term_amount >> term_amount >> rules_amount;
  std::cout << "Insert non-terminal symbols:\n";
  std::string non_term_symbols;
  std::cin >> non_term_symbols;
  std::unordered_set<char> non_terms_set;
  for (char c : non_term_symbols) {
    non_terms_set.insert(c);
  }
  std::cout << "Insert terminal symbols:\n";
  std::string term_symbols;
  std::cin >> term_symbols;
  std::unordered_set<char> terms_set;
  for (char c : term_symbols) {
    terms_set.insert(c);
  }
  std::cout << "Insert your grammar:\n";
  std::string new_rule;
  Earley::Grammar grammar(terms_set, non_terms_set);
  for (int64_t i = 0; i < rules_amount; ++i) {
    std::cin >> new_rule;
    grammar.InsertRule(new_rule);
  }
  std::string start;
  std::cout << "Insert start rule:\n";
  std::cin >> start;
  grammar.SetStart(start);
  Earley parser;
  parser.Fit(grammar);
  std::cout << "Insert amount of words:\n";
  int64_t words_amount;
  std::cin >> words_amount;
  std::cout << "Insert your words:\n";
  std::string word;
  std::getline(std::cin, word);
  for (int64_t i = 0; i < words_amount; ++i) {
    std::getline(std::cin, word);
    std::cout << (parser.Predict(word) ? "Yes\n" : "No\n");
  }
  return 0;
}