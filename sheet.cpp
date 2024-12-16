#include "sheet.h"

#include "cell.h"
#include "common.h"

//#include <algorithm>
//#include <functional>
#include <iostream>


using namespace std::literals;

Sheet::~Sheet() = default;

std::ostream& operator<<(std::ostream& output, Cell::Value value) {
    if (std::holds_alternative<double>(value)) {
        output << std::get<double>(value);
    }
    else if (std::holds_alternative<std::string>(value)) {
        output << std::get<std::string>(value);
    }
    else if (std::holds_alternative<FormulaError>(value)) {
        output << std::get<FormulaError>(value);
    }
    return output;
}

void Sheet::SetCell(Position pos, std::string text) {
    ThrowIfInvalidPosition(pos);

    if (printable_size_.rows - 1 < pos.row) {
        printable_size_.rows = pos.row + 1;
    }

    if (printable_size_.cols - 1 < pos.col) {
        printable_size_.cols = pos.col + 1;
    }

    auto cell_pos = table_.find(pos);
    if (cell_pos == table_.end()) {
        table_[pos] = std::make_unique<Cell>(GetSheetReference(), pos);
        table_[pos]->Set(std::move(text));
    }
    else {
        cell_pos->second->Set(std::move(text));
    }

}

const CellInterface* Sheet::GetCell(Position pos) const {
    ThrowIfInvalidPosition(pos);
    auto cell_pos = table_.find(pos);
    if (cell_pos != table_.end()) {
        return cell_pos->second.get();
    }

    return nullptr;
}


CellInterface* Sheet::GetCell(Position pos) {
    ThrowIfInvalidPosition(pos);
    auto cell_pos = table_.find(pos);
    if (cell_pos != table_.end()) {
        return cell_pos->second.get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    ThrowIfInvalidPosition(pos);
    auto cell_pos = table_.find(pos);
    if (cell_pos != table_.end()) {
        if (cell_pos->second) {
            cell_pos->second.reset();
        }
    }

    if (pos.col == printable_size_.cols - 1 || pos.row == printable_size_.rows - 1) {
        ChangePrintableSize();
    }

}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}


void Sheet::PrintValues(std::ostream& output) const {
    PrintWithFunction(output, "GetValue"sv);
}
void Sheet::PrintTexts(std::ostream& output) const {
    PrintWithFunction(output, "GetText"sv);
}

void Sheet::PrintWithFunction(std::ostream& output, const std::string_view function_name) const {
    for (int row = 0; row < printable_size_.rows; row++) {
        for (int col = 0; col < printable_size_.cols; col++) {
            Position find_on_pos;
            find_on_pos.row = row;
            find_on_pos.col = col;
            auto cell_pos = table_.find(find_on_pos);
            if (cell_pos != table_.end() && cell_pos->second) {
                if (function_name == "GetValue"sv) {
                    output << cell_pos->second->GetValue();
                }
                else if (function_name == "GetText"sv) {
                    output << cell_pos->second->GetText();
                }
            }
            if (col < printable_size_.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

Sheet& Sheet::GetSheetReference() {
    return *this;
}

void Sheet::ThrowIfInvalidPosition(Position pos) const {
    if (pos.col < 0 ||
        pos.row < 0 ||
        pos.col >= Position::MAX_COLS ||
        pos.row >= Position::MAX_ROWS) {

        throw InvalidPositionException("Position isn't correct");
    }
}

void Sheet::ChangePrintableSize() {
    int max_col = 0, max_row = 0;
    Size new_size;

    for (int row = 0; row < printable_size_.rows; row++) {
        for (int col = 0; col < printable_size_.cols; col++) {
            Position find_on_pos;
            find_on_pos.col = col;
            find_on_pos.row = row;
            auto cell_pos = table_.find(find_on_pos);
            if (cell_pos != table_.end() && cell_pos->second) {
                max_col = std::max(col, max_col);
                max_row = std::max(row, max_row);
                new_size.cols = max_col + 1;
                new_size.rows = max_row + 1;
            }
        }
    }
    printable_size_ = new_size;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

