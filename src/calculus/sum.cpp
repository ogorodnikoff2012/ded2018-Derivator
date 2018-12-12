#include <sum.h>
#include <negate_op.h>
#include <product.h>
#include "calculus_internal.h"

namespace calculus {


ExpressionPtr Sum::Simplify() {
    if (summands_.size() == 1) {
        if (summands_[0].inverse) {
            return std::make_shared<NegateOp>(summands_[0].expr)->Simplify();
        } else {
            return summands_[0].expr->Simplify();
        }
    }

    std::vector<AssociativeOperand> summands_copy;

    AssociativeOpAlign<Sum>(summands_, false, &summands_copy);

    for (size_t i = 0; i < summands_copy.size(); ++i) {
        summands_copy[i].expr = summands_copy[i].expr->Simplify();
        if (Is<NegateOp>(summands_copy[i].expr)) {
            summands_copy[i].inverse ^= true;
            summands_copy[i].expr = As<NegateOp>(summands_copy[i].expr)->GetInnerExpr();
        }
    }

    // Constant folding
    int last_nonconstant = 0;
    MoveConstantsToEnd(&summands_copy, &last_nonconstant);

    double value = 0;
    for (size_t i = last_nonconstant + 1; i < summands_copy.size(); ++i) {
        double summand = As<Constant>(summands_copy[i].expr)->GetValue();
        if (summands_copy[i].inverse) {
            value -= summand;
        } else {
            value += summand;
        }
    }

    if (!IsZero(value) || last_nonconstant < 0) {
        summands_copy.resize(last_nonconstant + 2);
        summands_copy.back().expr = BuildConstant(std::abs(value));
        summands_copy.back().inverse = value < 0;
    } else {
        summands_copy.resize(last_nonconstant + 1);
    }


    if (summands_copy.size() == 1) {
        if (summands_copy[0].inverse) {
            return std::make_shared<NegateOp>(summands_copy[0].expr)->Simplify();
        } else {
            return summands_copy[0].expr->Simplify();
        }
    }

    for (size_t i = 0; i < summands_copy.size(); ++i) {
        if (Is<Constant>(summands_copy[i].expr)) {
            continue;
        }
        double final_ratio = 1;
        bool changed = false;
        for (size_t j = i + 1; j < summands_copy.size(); ++j) {
            if (Is<Constant>(summands_copy[j].expr)) {
                continue;
            }
            double ratio = Ratio(summands_copy[j].expr, summands_copy[i].expr);
            if (summands_copy[i].inverse ^ summands_copy[j].inverse) {
                ratio *= -1;
            }
            if (!std::isnan(ratio)) {
                summands_copy[j].expr = kConstantZero;
                changed = true;
                final_ratio += ratio;
            }
        }
        if (changed) {
            auto prod = std::make_shared<Product>();
            *prod *= summands_copy[i].expr;
            *prod *= BuildConstant(final_ratio);
            summands_copy[i].expr = prod;
        }
    }

    return std::make_shared<Sum>(std::move(summands_copy));
}

ExpressionPtr Sum::TakeDerivative(char var_name) {
    decltype(summands_) summands;
    summands.reserve(summands_.size());
    for (const auto& summand : summands_) {
        summands.emplace_back(summand.expr->TakeDerivative(var_name), summand.inverse);
    }

    return std::make_shared<Sum>(std::move(summands))->Simplify();
}

ExpressionPtr Sum::Call(const std::vector<ExpressionPtr>& args) {
    decltype(summands_) summands;
    summands.reserve(summands_.size());
    for (const auto& summand : summands_) {
        summands.emplace_back(summand.expr->Call(args), summand.inverse);
    }

    return std::make_shared<Sum>(std::move(summands));
}

ExpressionPtr Sum::Substitute(char var_name, const ExpressionPtr& expr) {
    decltype(summands_) summands;
    summands.reserve(summands_.size());
    for (const auto& summand : summands_) {
        summands.emplace_back(summand.expr->Substitute(var_name, expr), summand.inverse);
    }

    return std::make_shared<Sum>(std::move(summands));
}

void Sum::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kSumPriorityLevel) {
        out << '(';
    }

    if (summands_[0].inverse) {
        out << '-';
    }
    summands_[0].expr->Print(out, kSumPriorityLevel + (summands_[0].inverse ? 1 : 0));

    for (size_t i = 1; i < summands_.size(); ++i) {
        out << (summands_[i].inverse ? " - " : " + ");
        summands_[i].expr->Print(out, kSumPriorityLevel + (summands_[i].inverse ? 1 : 0));
    }

    if (cur_priority_level > kSumPriorityLevel) {
        out << ')';
    }
}

void Sum::TexDump(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kSumPriorityLevel) {
        out << "\\left(";
    }

    if (summands_[0].inverse) {
        out << '-';
    }
    summands_[0].expr->TexDump(out, kSumPriorityLevel + (summands_[0].inverse ? 1 : 0));

    for (size_t i = 1; i < summands_.size(); ++i) {
        out << (summands_[i].inverse ? " - " : " + ");
        summands_[i].expr->TexDump(out, kSumPriorityLevel + (summands_[i].inverse ? 1 : 0));
    }

    if (cur_priority_level > kSumPriorityLevel) {
        out << "\\right)";
    }
}

bool Sum::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    if (summands_.size() != ptr->summands_.size()) {
        return false;
    }
    for (size_t i = 0; i < summands_.size(); ++i) {
        if (summands_[i].inverse != ptr->summands_[i].inverse || !summands_[i].expr->DeepCompare(ptr->summands_[i].expr)) {
            return false;
        }
    }
    return true;
}

Sum& Sum::operator+=(const ExpressionPtr& expr) {
    summands_.emplace_back(expr, false);
    return *this;
}

Sum& Sum::operator-=(const ExpressionPtr& expr) {
    summands_.emplace_back(expr, true);
    return *this;
}

void Sum::ReserveSize(int size) {
    summands_.reserve(size);
}


}  /* namespace calculus */
