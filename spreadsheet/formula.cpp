#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
    return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try
    : ast_{ ParseFormulaAST(std::move(expression)) } {
    } catch (const FormulaException&) {
        throw FormulaException("ARITHM");
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        const std::function<double(Position)> value = [&](const Position p)->double {
            if (!p.IsValid()) throw FormulaError(FormulaError::Category::Ref);
            const auto* cell = sheet.GetCell(p);
            if (!cell) {
                return 0;
            }
            auto value = cell->GetValue();
            if (std::holds_alternative<double>(value)) {
                return std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                auto str = std::get<std::string>(value);
                double result = 0;
                if (!str.empty()) {
                    std::istringstream in(str);
                    if (!(in >> result)) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }
                return result;
            }
            throw FormulaError(std::get<FormulaError>(value));
        };
        try {
            return ast_.Execute(value);
        }
        catch (const FormulaError& e) {
            return e;
        }
    }
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> positions;
        for (auto& pos : ast_.GetCells()) {
            if (pos.IsValid()) {
                positions.push_back(pos);
            }
        }
        auto iter = std::unique(positions.begin(), positions.end());
        positions.erase(iter, positions.end());
        return positions;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (FormulaException& err) {
        throw err;
    }
}