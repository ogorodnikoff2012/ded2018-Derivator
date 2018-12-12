#pragma once

#include "expression.h"

namespace calculus {

class Product : public Expression {
public:
    Product() = default;

    template <class Vector>
    explicit Product(Vector&& multipliers) : multipliers_(std::forward<Vector>(multipliers)) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    Product& operator*=(const ExpressionPtr& expr);
    Product& operator/=(const ExpressionPtr& expr);
    void ReserveSize(int size);

    const std::vector<AssociativeOperand>& GetOperands() const {
        return multipliers_;
    }

private:
    std::vector<AssociativeOperand> multipliers_;
};

}  /* namespace calculus */
