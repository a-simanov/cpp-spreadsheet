#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(Sheet& sheet)
    :impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {
}

Cell::~Cell() = default;

void Cell::ProcessImpl (std::string text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        std::unique_ptr<FormulaImpl> temp_impl;
        try {
            temp_impl = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
        } catch (std::exception &e) {
            throw FormulaException("Formula error");
        }
        auto referenced_cells = temp_impl->GetReferencedCells();
        CheckCicleDependency(referenced_cells);
        impl_ = std::move(temp_impl);
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
}

void Cell::Set(std::string text) {
    ProcessImpl(text);
    for (const auto& pos : referenced_cells_) {
        pos->dependent_cells_.erase(this);
    }    
    referenced_cells_.clear();
    for (const auto &pos : GetReferencedCells()) {
        auto *cell_ptr = sheet_.GetCell(pos);
        if (cell_ptr == nullptr) {
            sheet_.SetCell(pos, "");
            cell_ptr = sheet_.GetCell(pos);
        }
        static_cast<Cell *>(cell_ptr)->dependent_cells_.insert(this);
        referenced_cells_.insert(static_cast<Cell *>(cell_ptr));
    }
    impl_->InvalidateCache();
    for (auto dep_cell : dependent_cells_) {
        dep_cell->Clear();
    }
}

void Cell::Clear() {
    Set("");
}

void Cell::CheckCicleDependency(const std::vector<Position>& referenced_cells) const {
    std::queue<Position> positions ;
    for (const Position& pos : referenced_cells) {
        positions.push(pos);
    }
    std::map<Position, bool> visited;

    while (!positions.empty()) {
        Position tmp_pos = positions.front();
        positions.pop();
        if (visited[tmp_pos]) {
            continue;
        }
        visited[tmp_pos] = true;
        const CellInterface* tmp_cell = sheet_.GetCell(tmp_pos);
        if (tmp_cell == this) {
            throw CircularDependencyException{"Circular dependency detected"};
        }
        if (tmp_cell == nullptr) {
            sheet_.SetCell(tmp_pos, "");
            tmp_cell = sheet_.GetCell(tmp_pos);
        }
        for (const Position& pos : tmp_cell->GetReferencedCells()) {
            positions.push(pos);
        }
    }
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

bool Cell::Impl::IsCacheValid() const {
    return true;
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return {};
}

std::string Cell::EmptyImpl::GetText() const {
    return {};
}

Cell::TextImpl::TextImpl(std::string value)
    :value_(std::move(value)) {
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (value_.empty()) {
        return {};
    }
    if (value_[0] == ESCAPE_SIGN) {
        return value_.substr(1);
    }
    return value_;
}

std::string Cell::TextImpl::GetText() const {
    return value_;
}

Cell::FormulaImpl::FormulaImpl(std::string expression, const SheetInterface& sheet)
    : ptr_(ParseFormula(expression))
    , sheet_(sheet) {
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = ptr_->Evaluate(sheet_);
    }

    auto tmp_value = ptr_->Evaluate(sheet_);
    if (std::holds_alternative<double>(tmp_value)) {
        return std::get<double>(tmp_value);
    }
    return std::get<FormulaError>(tmp_value);
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + ptr_->GetExpression();
}

void Cell::FormulaImpl::InvalidateCache() {
    cache_.reset();
};

bool Cell::FormulaImpl::IsCacheValid() const {
    return cache_.has_value();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return ptr_->GetReferencedCells();
}