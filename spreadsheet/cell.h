#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>
#include <queue>
#include <map>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    
    void CheckCicleDependency(const std::vector<Position>& referenced_cells) const;
    void ProcessImpl (std::string text);

    std::unique_ptr<Impl> impl_;
    std::unordered_set<Cell*> dependent_cells_;
    std::unordered_set<Cell*> referenced_cells_;
    Sheet& sheet_;
};

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual void InvalidateCache() {};
    virtual std::vector<Position> GetReferencedCells() const;
    virtual bool IsCacheValid() const;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    Value GetValue() const override;
    std::string GetText() const override;
};

class Cell::TextImpl : public Cell::Impl {
public:
    TextImpl(std::string value);
    Value GetValue() const override;
    std::string GetText() const override;
private:
    std::string value_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    FormulaImpl(std::string expression, const SheetInterface& sheet);
    Value GetValue() const override;
    std::string GetText() const override;
    bool IsCacheValid() const override;
    void InvalidateCache() override;
    std::vector<Position> GetReferencedCells() const override;
private:
    std::unique_ptr<FormulaInterface> ptr_;
    mutable std::optional<FormulaInterface::Value> cache_;
    const SheetInterface& sheet_;
};