#include "sheet.h"

#include "cell.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }
    auto& cell_ptr = cells_[pos];
    if (!cell_ptr) {
        cell_ptr = std::make_unique<Cell>(*this);
    }
    auto cell = dynamic_cast<Cell *>(cell_ptr.get());
    cell->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }
    const auto iter = cells_.find(pos);
    if (iter != cells_.end()) {
        return iter->second.get();
    }
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }
    const auto iter = cells_.find(pos);
    if (iter != cells_.end()) {
        return iter->second.get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect position");
    }
    auto iter = cells_.find(pos);
    if (iter != cells_.end()) {
        dynamic_cast<Cell*>(iter->second.get())->Clear();
        cells_.erase(iter);
    }
}

Size Sheet::GetPrintableSize() const {
    if (cells_.size() == 0) {
        return { 0,0 };
    }
    Size size{};
    for (const auto& cell : cells_) {
        if (cell.second && cell.second->GetText().size() != 0) {
            size.rows = std::max(cell.first.row + 1, size.rows);
            size.cols = std::max(cell.first.col + 1, size.cols);
        }
    }
    return size;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int r = 0; r < size.rows; r++) {
        for (int c = 0; c < size.cols; c++) {
            if (c > 0) {
                output << "\t";
            }
            Position pos = {r, c};
            auto iter = cells_.find(pos);
            if (iter != cells_.end() && iter->second != nullptr && !iter->second->GetText().empty()) {
                auto val = cells_.at(pos)->GetValue();
                std::visit([&](auto&& arg) {
                    output << arg; 
                }, val);
            }
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int r = 0; r < size.rows; r++) {
        for (int c = 0; c < size.cols; c++) {
            if (c > 0) {
                output << "\t";
            }
            Position pos = {r, c};
            auto iter = cells_.find(pos);
            if (iter != cells_.end() && iter->second != nullptr && !iter->second->GetText().empty()) {
                output << cells_.at(pos)->GetText();
            }
        }
        output << "\n";
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}