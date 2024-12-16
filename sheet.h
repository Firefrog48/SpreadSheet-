#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Можете дополнить ваш класс нужными полями и методами

private:
    Sheet& GetSheetReference();
    void ThrowIfInvalidPosition(Position pos) const;
    void ChangePrintableSize();
    void PrintWithFunction(std::ostream& output, const std::string_view function) const;

    std::unordered_map <Position, std::unique_ptr<Cell>, PositionHasher> table_;
    Size printable_size_;
};
