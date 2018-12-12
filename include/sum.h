#pragma once

#include "expression.h"

namespace calculus {

class Sum : public Expression {
public:
    Sum() = default;

    template <class Vector>
    explicit Sum(Vector&& summands) : summands_(std::forward<Vector>(summands)) {
    }

    virtual ExpressionPtr Simplify() override;
    virtual ExpressionPtr TakeDerivative(char var_name) override;
    virtual ExpressionPtr Call(const std::vector<ExpressionPtr>& args) override;
    virtual ExpressionPtr Substitute(char var_name, const ExpressionPtr& value) override;
    virtual void Print(std::ostream& out, int cur_priority_level) const override;
    virtual void TexDump(std::ostream& out, int cur_priority_level = -1) const override;
    virtual bool DeepCompare(const ExpressionPtr& other) const override;

    Sum& operator+=(const ExpressionPtr& expr);
    Sum& operator-=(const ExpressionPtr& expr);
    void ReserveSize(int size);

    const std::vector<AssociativeOperand>& GetOperands() const {
        return summands_;
    }

private:
    std::vector<AssociativeOperand> summands_;
};

}  /* namespace calculus */
