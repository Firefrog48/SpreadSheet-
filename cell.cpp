#include "cell.h"
#include "sheet.h"
#include "common.h"


#include <cassert>
#include <iostream>
#include <string>


class Cell::Impl {
public:
	virtual CellInterface::Value GetValue(const SheetInterface& sheet) = 0;
	virtual std::string GetText() = 0;
	virtual ~Impl() = default;
	virtual std::vector<Position> GetReferencedCells() const = 0;
};

class Cell::EmptyImpl final : public Cell::Impl {
public:
	EmptyImpl() = default;
	CellInterface::Value GetValue(const SheetInterface& sheet) override;
	std::string GetText() override;
	std::vector<Position> GetReferencedCells() const override;
};

class Cell::TextImpl final : public Impl {
public:
	TextImpl(std::string text) : text_(std::move(text)) {}
	CellInterface::Value GetValue(const SheetInterface& sheet) override;
	std::string GetText() override;
	std::vector<Position> GetReferencedCells() const override;
private:
	std::string text_;
};

class Cell::FormulaImpl final : public Impl {
public:
	FormulaImpl(std::string text) : formula_ptr_(ParseFormula(std::move(text.substr(1)))) {}
	CellInterface::Value GetValue(const SheetInterface& sheet) override;
	std::string GetText() override;
	std::vector<Position> GetReferencedCells() const override;
private:
	std::unique_ptr<FormulaInterface> formula_ptr_;
};

Cell::Cell(Sheet& sheet, Position pos) : sheet_(sheet)
, position_(pos) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
	if (impl_ != nullptr && impl_->GetText() == text) {
		return;
	}

	if (text[0] == FORMULA_SIGN && text.size() > 1) {
		auto temp_impl = std::make_unique<FormulaImpl>(std::move(text));
		CheckCircularError(temp_impl->GetReferencedCells());
		impl_ = std::move(temp_impl);
	}
	else if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
	}
	else {
		impl_ = std::make_unique<TextImpl>(std::move(text));
	}
	ClearEdges();
	AddEdges(impl_->GetReferencedCells());

}

void Cell::Clear() {
	cells_included_in_formula_.clear();
	ClearCache();
	impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
	if (!cell_cache_.has_value()) {
		cell_cache_ = impl_->GetValue(sheet_);
	}
	return cell_cache_.value();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}



void Cell::AddEdges(const std::vector<Position>& cells_positions) {
	for (const auto& cell_pos : cells_positions) {
		Cell* cell = dynamic_cast<Cell*>(sheet_.GetCell(cell_pos));
		// добавляем указатель на эту ячейку всем ячейкам включенным в формулу
		cell->referenced_in_cells_.insert(this);
		// добавляем указатель на ячейку включенную в формулу
		if (cell == this) {
			throw CircularDependencyException("Circular dependency error");
		}
		cells_included_in_formula_.insert(cell);
	}
}

void Cell::ClearEdges() {
	// если ячейка используется другими клетками, нужно очистить кэш этих ячеек.
	ClearCache();
	// удаляем все ячейки которые были использованы в формуле до этого.
	cells_included_in_formula_.clear();
}

void Cell::CheckCircularError(const std::vector<Position>& cells) {
	std::unordered_set<Position, PositionHasher> checked_cells;
	std::unordered_set<Position, PositionHasher> cells_in_formula_from_checked_cells;
	checked_cells.insert(position_);
	cells_in_formula_from_checked_cells.insert(position_);
	for (const auto& cell_pos : cells) {
		Cell* cell_ptr = dynamic_cast<Cell*>(sheet_.GetCell(cell_pos));
		// если по указанной позиции нет ячейки, добавляем пустую ячейку и присваиваем указателю эту ячейку
		if (cell_ptr == nullptr) {
			sheet_.SetCell(cell_pos, "");
			cell_ptr = dynamic_cast<Cell*>(sheet_.GetCell(cell_pos));
		}
		cell_ptr->CheckCircularError(checked_cells, cells_in_formula_from_checked_cells);
	}
}

void Cell::CheckCircularError(std::unordered_set<Position, PositionHasher>& checked_cells, std::unordered_set<Position, PositionHasher>& cells_in_formula_from_checked_cells) {
	for (const auto& cell_ptr : cells_included_in_formula_) {
		if (cells_in_formula_from_checked_cells.count(cell_ptr->position_)) {
			throw CircularDependencyException("Circular dependency error");
		}
		if (!checked_cells.count(cell_ptr->position_)) {
			checked_cells.insert(cell_ptr->position_);
			cell_ptr->CheckCircularError(checked_cells, cells_in_formula_from_checked_cells);
		}
	}
}

void Cell::ClearCache() {
	std::unordered_set<Position, PositionHasher> checked_cells;
	checked_cells.insert(position_);
	if (cell_cache_.has_value()) {
		cell_cache_.reset();
	}
	if (!referenced_in_cells_.empty()) {
		ClearCaches(checked_cells, referenced_in_cells_);
	}
}

void Cell::ClearCaches(std::unordered_set<Position, PositionHasher>& checked_cells, const std::set<Cell*>& referenced_in_cells) {
	for (const auto& cell_ptr : referenced_in_cells) {
		if (!checked_cells.count(cell_ptr->position_)) {
			if (cell_ptr->cell_cache_.has_value()) {
				cell_cache_.reset();
			}
		}
		checked_cells.insert(cell_ptr->position_);
		ClearCaches(checked_cells, cell_ptr->referenced_in_cells_);
	}
}


Cell::Value Cell::EmptyImpl::GetValue(const SheetInterface& sheet) {
	return "";
}

std::string Cell::EmptyImpl::GetText() {
	return "";
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
	return std::vector<Position>();
}

Cell::Value Cell::TextImpl::GetValue(const SheetInterface& sheet) {
	if (text_[0] == ESCAPE_SIGN) {
		return text_.substr(1);
	}
	return text_;
}

std::string Cell::TextImpl::GetText() {
	return text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const
{
	return std::vector<Position>();
}

Cell::Value Cell::FormulaImpl::GetValue(const SheetInterface& sheet) {
	Value result;
	auto formula_value = formula_ptr_->Evaluate(sheet);
	if (std::holds_alternative<double>(formula_value)) {
		result = std::get<double>(formula_value);
	}
	else if (std::holds_alternative<FormulaError>(formula_value)) {
		result = std::get<FormulaError>(formula_value);
	}
	return result;
}

std::string Cell::FormulaImpl::GetText() {
	return FORMULA_SIGN + formula_ptr_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
	return formula_ptr_->GetReferencedCells();
}
