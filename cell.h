#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <set>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:

	Cell(Sheet& sheet, Position pos);
	~Cell();

	void Set(std::string text);
	void Clear();

	Value GetValue() const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;

	/*bool IsReferenced() const;*/
private:

	class Impl;
	class EmptyImpl;
	class TextImpl;
	class FormulaImpl;

	void AddEdges(const std::vector<Position>& cells_positions);
	void ClearEdges();
	void CheckCircularError(const std::vector<Position>& cells);
	void CheckCircularError(std::unordered_set<Position, PositionHasher>& checked_cells, std::unordered_set<Position, PositionHasher>& cells_in_formula_from_checked_cells);
	void ClearCache();
	void ClearCaches(std::unordered_set<Position, PositionHasher>& checked_cells, const std::set <Cell*>& referenced_in_cells);

	mutable std::optional<Value> cell_cache_;
	Sheet& sheet_;
	Position position_;
	std::unique_ptr<Impl> impl_;
	std::set<Cell*> cells_included_in_formula_;
	std::set<Cell*> referenced_in_cells_;

};

