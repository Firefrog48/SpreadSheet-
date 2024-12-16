#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;



namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression) try : ast_{ ParseFormulaAST(expression) } {
            Position prev = Position::NONE;
            for (auto& cell : ast_.GetCells()) {
                if (cell.IsValid() && !(cell == prev)) {
                    referenced_cells_.push_back(std::move(cell));
                    prev = cell;
                }
            }
        }
        catch (std::exception& exc) {
            std::throw_with_nested(FormulaException(exc.what()));
        }
        Value Evaluate(const SheetInterface& sheet) const override {
            Value value;
            try {
                value = ast_.Execute(sheet);
            }
            catch (const FormulaError& exc) {
                value = FormulaError(exc);
            }
            return value;
        }

        std::string GetExpression() const override {
            std::stringstream ss;
            ast_.PrintFormula(ss);
            return ss.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            return referenced_cells_;
        }

    private:
        FormulaAST ast_;
        std::vector<Position> referenced_cells_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}