#include <expression.h>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <ostream>

namespace calculus {

#define COMPARE_CHECK_TRIVIAL                                                   \
    auto ptr = dynamic_cast<const std::decay_t<decltype(*this)>*>(other.get()); \
    if (ptr == nullptr) {                                                       \
        return false;                                                           \
    }                                                                           \
    if (ptr == this) {                                                          \
        return true;                                                            \
    }

static ExpressionPtr Inverse(const ExpressionPtr& expr) {
    auto result = std::make_shared<Product>();
    *result /= expr;
    return result;
}

const ExpressionPtr kConstantZero   = std::make_shared<Constant>(0);
const ExpressionPtr kConstantOne    = std::make_shared<Constant>(1);
const ExpressionPtr kConstantNegOne = std::make_shared<Constant>(-1);
const ExpressionPtr kConstantPi     = std::make_shared<Constant>(M_PI);

static const std::unordered_map<std::string, ExpressionPtr> kTableOfDerivatives = {
    {"sin", std::make_shared<Function>("cos")},
    {"cos", std::make_shared<NegateOp>(std::make_shared<Function>("sin"))},
    {"log", Inverse(std::make_shared<Function>("id"))},
    {"exp", std::make_shared<Function>("exp")},
    {"id",  kConstantOne},
};

static const std::unordered_map<std::string, double(*)(double)> kUnaryFunctionTable = {
    {"sin", std::sin},
    {"cos", std::cos},
    {"log", std::log},
    {"exp", std::exp},
    {"id",  [](double x) { return x; }},
};

static const std::unordered_set<std::string> kTableOfLaTeXDeclaredFunctions = {
    "sin", "cos", "log", "exp"
};

static bool IsZero(double x) {
    return std::fabs(x) < kDoubleTolerance;
}

template <class T>
static const T* As(const ExpressionPtr& ptr) {
    return dynamic_cast<const T*>(ptr.get());
}

template <class T>
static bool Is(const ExpressionPtr& ptr) {
    return As<T>(ptr) != nullptr;
}

/* ---------------------------------------- Constant ---------------------------------------- */

ExpressionPtr Constant::Simplify() {
    return shared_from_this();
}

ExpressionPtr Constant::TakeDerivative(char) {
    return kConstantZero;
}

ExpressionPtr Constant::Call(const std::vector<ExpressionPtr>&) {
    return shared_from_this();
}

ExpressionPtr Constant::Substitute(char, const ExpressionPtr&) {
    return shared_from_this();
}

void Constant::Print(std::ostream& out, int) const {
    out << value_;
}

void Constant::TexDump(std::ostream& out, int) const {
    out << value_;
}

bool Constant::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return std::fabs(value_ - ptr->value_) < kDoubleTolerance;
}

/* ---------------------------------------- Variable ---------------------------------------- */

ExpressionPtr Variable::Simplify() {
    return shared_from_this();
}

ExpressionPtr Variable::TakeDerivative(char var_name) {
    return (var_name == name_) ? kConstantOne : kConstantZero;
}

ExpressionPtr Variable::Call(const std::vector<ExpressionPtr>&) {
    return shared_from_this();
}

ExpressionPtr Variable::Substitute(char var_name, const ExpressionPtr& other) {
    return var_name == name_ ? other : shared_from_this();
}

void Variable::Print(std::ostream& out, int) const {
    out << name_;
}

void Variable::TexDump(std::ostream& out, int) const {
    out << name_;
}

bool Variable::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return name_ == ptr->name_;
}

/* ---------------------------------------- Function ---------------------------------------- */

ExpressionPtr Function::Simplify() {
    return shared_from_this();
}

ExpressionPtr Function::TakeDerivative(char) {
    auto iter = kTableOfDerivatives.find(name_);
    if (iter == kTableOfDerivatives.end()) {
        return std::make_shared<DifferentiateOp>(shared_from_this(), kDefaultDerivativeVariable);
    }
    return iter->second;//->Call(std::vector<ExpressionPtr>{kVariableStub, std::make_shared<Variable>(var_name)});
}

static ExpressionPtr BuildConstant(double x) {
    return  IsZero(x) ? kConstantZero :
            IsZero(x - 1) ? kConstantOne :
            IsZero(x - M_PI) ? kConstantPi :
                std::make_shared<Constant>(x);
}

ExpressionPtr Function::Call(const std::vector<ExpressionPtr>& args) {
    // Now we have only unary functions
    if (args.size() != 1) {
        std::string what = "Argument count mismatch: expected 1, got ";
        what += std::to_string(args.size());
        throw RuntimeError(what);
    }

    const auto& arg = args[0]->Simplify();

    if (name_ == "id") {
        return arg;
    }

    auto iter = kUnaryFunctionTable.find(name_);
    if (!Is<Constant>(arg) || iter == kUnaryFunctionTable.end()) {
        return std::make_shared<CallOp>(shared_from_this(), args);
    }

    double result = iter->second(As<Constant>(arg)->GetValue());
    return BuildConstant(result);
}

ExpressionPtr Function::Substitute(char, const ExpressionPtr&) {
    return shared_from_this();
}

void Function::Print(std::ostream& out, int) const {
    out << name_;
}

void Function::TexDump(std::ostream& out, int) const {
    if (kTableOfLaTeXDeclaredFunctions.find(name_) != kTableOfLaTeXDeclaredFunctions.end()) {
        out << '\\';
    }
    out << name_;
}

bool Function::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return name_ == ptr->name_;
}

/* ---------------------------------------- Sum ---------------------------------------- */

template <class T>
void AssociativeOpAlign(const std::vector<AssociativeOperand>& operands, bool global_inverse, std::vector<AssociativeOperand>* result) {
    for (size_t i = 0; i < operands.size(); ++i) {
        if (Is<T>(operands[i].expr)) {
            auto op = As<T>(operands[i].expr);
            AssociativeOpAlign<T>(op->GetOperands(), operands[i].inverse ^ global_inverse, result);
        } else {
            result->push_back(operands[i]);
            result->back().inverse ^= global_inverse;
        }
    }
}

static void MoveConstantsToEnd(std::vector<AssociativeOperand>* operands_ptr, int* last_nonconstant_ptr) {
    auto& operands = *operands_ptr;
    int& last_nonconstant = *last_nonconstant_ptr;

    last_nonconstant = operands.size() - 1;
    for (int i = operands.size() - 1; i >= 0; --i) {
        if (Is<Constant>(operands[i].expr)) {
            if (i != last_nonconstant) {
                std::swap(operands[i], operands[last_nonconstant]);
            }
            --last_nonconstant;
        }
    }
}

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

/* ---------------------------------------- Product ---------------------------------------- */

ExpressionPtr Product::Simplify() {
    if (multipliers_.size() == 1 && !multipliers_[0].inverse) {
        return multipliers_[0].expr->Simplify();
    }
    std::vector<AssociativeOperand> multipliers_copy;

    AssociativeOpAlign<Product>(multipliers_, false, &multipliers_copy);

    bool need_to_be_negated = false;

    for (size_t i = 0; i < multipliers_copy.size(); ++i) {
        multipliers_copy[i].expr = multipliers_copy[i].expr->Simplify();
        if (Is<NegateOp>(multipliers_copy[i].expr)) {
            need_to_be_negated ^= true;
            multipliers_copy[i].expr = As<NegateOp>(multipliers_copy[i].expr)->GetInnerExpr();
        }
        if (multipliers_copy[i].inverse && Is<PowerOp>(multipliers_copy[i].expr)) {
            auto expr = As<PowerOp>(multipliers_copy[i].expr);
            if (Is<Constant>(expr->GetExp()) && As<Constant>(expr->GetExp())->GetValue() - kDoubleTolerance > 0) {
                continue;
            }
            multipliers_copy[i].inverse = false;
            multipliers_copy[i].expr = std::make_shared<PowerOp>(expr->GetBase(), std::make_shared<NegateOp>(expr->GetExp()));
        }
    }

    // Constant folding
    int last_nonconstant = 0;
    MoveConstantsToEnd(&multipliers_copy, &last_nonconstant);

    double value = 1;
    for (size_t i = last_nonconstant + 1; i < multipliers_copy.size(); ++i) {
        double multiplier = As<Constant>(multipliers_copy[i].expr)->GetValue();
        if (multipliers_copy[i].inverse) {
            if (IsZero(multiplier)) {
                throw RuntimeError("Division by zero");
            }
            value /= multiplier;
        } else {
            value *= multiplier;
        }
    }

    if (IsZero(value)) {
        return kConstantZero;
    }

    if (last_nonconstant < 0) {
        return BuildConstant(need_to_be_negated ? -value : value);
    }

    if (IsZero(value - 1) || IsZero(value + 1)) {
        multipliers_copy.resize(last_nonconstant + 1);
        need_to_be_negated ^= IsZero(value + 1);
    } else {
        multipliers_copy.resize(last_nonconstant + 2);
        multipliers_copy.back().expr = BuildConstant(value);
        multipliers_copy.back().inverse = false;
    }

    // Power folding

    for (size_t i = 0; i < multipliers_copy.size(); ++i) {
        if (Is<Constant>(multipliers_copy[i].expr)) {
            continue;
        }

        auto total_power = std::make_shared<Sum>();

        auto simple = multipliers_copy[i].expr;
        if (Is<PowerOp>(simple)) {
            auto exp = As<PowerOp>(simple)->GetExp();
            simple = As<PowerOp>(simple)->GetBase();
            *total_power += exp;
        } else {
            *total_power += kConstantOne;
        }

        bool found_similars = false;
        for (size_t j = i + 1; j < multipliers_copy.size(); ++j) {
            if (Is<Constant>(multipliers_copy[j].expr)) {
                continue;
            }
            bool not_equal_inverse = multipliers_copy[i].inverse != multipliers_copy[j].inverse;

            auto other_simple = multipliers_copy[j].expr;
            ExpressionPtr other_exp = kConstantOne;
            if (Is<PowerOp>(other_simple)) {
                other_exp = As<PowerOp>(other_simple)->GetExp();
                other_simple = As<PowerOp>(other_simple)->GetBase();
            }

            double ratio = Ratio(simple, other_simple);
            if (std::isnan(ratio)) {
                continue;
            }
            found_similars = true;

            if (not_equal_inverse) {
                *total_power -= other_exp;
            } else {
                *total_power += other_exp;
            }

            multipliers_copy[j].expr = std::make_shared<PowerOp>(BuildConstant(ratio), other_exp);

/*            if (multipliers_copy[i].inverse != multipliers_copy[j].inverse) {
                double ratio = Ratio(multipliers_copy[i].expr, multipliers_copy[j].expr);
                if (!std::isnan(ratio)) {
                    multipliers_copy[i].expr = BuildConstant(ratio);
                    multipliers_copy[j].expr = kConstantOne;
                    break;
                }
            }*/
        }

        if (found_similars) {
            multipliers_copy[i].expr = std::make_shared<PowerOp>(simple, total_power);
        }
    }

    auto result = std::make_shared<Product>(std::move(multipliers_copy));
    if (need_to_be_negated) {
        return std::make_shared<NegateOp>(result);
    }
    return result;
}

ExpressionPtr Product::TakeDerivative(char var_name) {
    auto result = std::make_shared<Sum>();
    result->ReserveSize(multipliers_.size());

    for (size_t i = 0; i < multipliers_.size(); ++i) {
        auto multipliers_copy = multipliers_;
        multipliers_copy[i].expr = multipliers_copy[i].expr->TakeDerivative(var_name);
        multipliers_copy[i].inverse = false;

        auto summand = std::make_shared<Product>(std::move(multipliers_copy));
        if (multipliers_[i].inverse) {
            *summand /= multipliers_[i].expr;
            *summand /= multipliers_[i].expr;
            *result -= summand;
        } else {
            *result += summand;
        }
    }

    return result->Simplify();
}

ExpressionPtr Product::Call(const std::vector<ExpressionPtr>& args) {
    decltype(multipliers_) multipliers;
    multipliers.reserve(multipliers_.size());
    for (const auto& multiplier : multipliers_) {
        multipliers.emplace_back(multiplier.expr->Call(args), multiplier.inverse);
    }

    return std::make_shared<Product>(std::move(multipliers))->Simplify();
}

ExpressionPtr Product::Substitute(char var_name, const ExpressionPtr& expr) {
    decltype(multipliers_) multipliers;
    multipliers.reserve(multipliers_.size());
    for (const auto& multiplier : multipliers_) {
        multipliers.emplace_back(multiplier.expr->Substitute(var_name, expr), multiplier.inverse);
    }

    return std::make_shared<Product>(std::move(multipliers))->Simplify();
}

void Product::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kProdPriorityLevel) {
        out << '(';
    }

    bool skip_last = false;

    if (multipliers_[0].inverse) {
        if (Is<Constant>(multipliers_.back().expr) && !multipliers_.back().inverse) {
            out << As<Constant>(multipliers_.back().expr)->GetValue() << " / ";
            skip_last = true;
        } else {
            out << "1 / ";
        }
    }
    multipliers_[0].expr->Print(out, kProdPriorityLevel + (multipliers_[0].inverse ? 1 : 0));

    for (size_t i = 1, n = multipliers_.size() - (skip_last ? 1 : 0); i < n; ++i) {
        out << (multipliers_[i].inverse ? " / " : " * ");
        multipliers_[i].expr->Print(out, kProdPriorityLevel + (multipliers_[i].inverse ? 1 : 0));
    }
    if (cur_priority_level > kProdPriorityLevel) {
        out << ')';
    }
}

static void PrintProduct(std::ostream& out, const std::vector<const ExpressionPtr*>& multipliers) {
    bool was_prev_a_number = false;
    for (size_t i = 0; i < multipliers.size(); ++i) {
        bool is_number = Is<Constant>(*multipliers[i]);
        if ((i > 0 && is_number) || was_prev_a_number) {
            out << "\\cdot ";
        }
        multipliers[i]->get()->TexDump(out, kProdPriorityLevel);
        was_prev_a_number = is_number;
    }
}

void Product::TexDump(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kProdPriorityLevel) {
        out << "\\left(";
    }

    std::vector<const ExpressionPtr*> numerator;
    std::vector<const ExpressionPtr*> denominator;

    for (const auto& multiplier : multipliers_) {
        if (multiplier.inverse) {
            denominator.push_back(&multiplier.expr);
        } else {
            numerator.push_back(&multiplier.expr);
        }
    }

    if (!denominator.empty()) {
        out << "\\frac{";
    }
    if (numerator.empty()) {
        out << "1";
    } else {
        PrintProduct(out, numerator);
    }
    if (!denominator.empty()) {
        out << "}{";
        PrintProduct(out, denominator);
        out << "}";
    }

    if (cur_priority_level > kProdPriorityLevel) {
        out << "\\right)";
    }
}

bool Product::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    if (multipliers_.size() != ptr->multipliers_.size()) {
        return false;
    }
    for (size_t i = 0; i < multipliers_.size(); ++i) {
        if (multipliers_[i].inverse != ptr->multipliers_[i].inverse ||
                !multipliers_[i].expr->DeepCompare(ptr->multipliers_[i].expr)) {
            return false;
        }
    }
    return true;
}

Product& Product::operator*=(const ExpressionPtr& expr) {
    multipliers_.emplace_back(expr, false);
    return *this;
}

Product& Product::operator/=(const ExpressionPtr& expr) {
    multipliers_.emplace_back(expr, true);
    return *this;
}

void Product::ReserveSize(int size) {
    multipliers_.reserve(size);
}

/* ---------------------------------------- NegateOp ---------------------------------------- */

ExpressionPtr NegateOp::Simplify() {
    if (Is<Constant>(expr_)) {
        return BuildConstant(-As<Constant>(expr_)->GetValue());
    }
    if (Is<NegateOp>(expr_)) {
        return As<NegateOp>(expr_)->GetInnerExpr()->Simplify();
    }
    if (Is<Sum>(expr_)) {
        auto summands_copy = As<Sum>(expr_)->GetOperands();
        for (size_t i = 0; i < summands_copy.size(); ++i) {
            summands_copy[i].inverse ^= true;
        }
        return std::make_shared<Sum>(std::move(summands_copy));
    }
    return std::make_shared<NegateOp>(expr_->Simplify());
}

ExpressionPtr NegateOp::TakeDerivative(char var_name) {
    return std::make_shared<NegateOp>(expr_->TakeDerivative(var_name));
}

ExpressionPtr NegateOp::Call(const std::vector<ExpressionPtr>& args) {
    return std::make_shared<NegateOp>(expr_->Call(args));
}

ExpressionPtr NegateOp::Substitute(char var_name, const ExpressionPtr& value) {
    return std::make_shared<NegateOp>(expr_->Substitute(var_name, value));
}

void NegateOp::Print(std::ostream& out, int cur_priority_level) const {
    out << '-';

    if (cur_priority_level > kPrefixOpPriorityLevel) {
        out << '(';
    }
    expr_->Print(out, kPrefixOpPriorityLevel);
    if (cur_priority_level > kPrefixOpPriorityLevel) {
        out << ')';
    }
}

void NegateOp::TexDump(std::ostream& out, int cur_priority_level) const {
    out << '-';

    if (cur_priority_level > kPrefixOpPriorityLevel) {
        out << "\\left(";
    }
    expr_->TexDump(out, kPrefixOpPriorityLevel);
    if (cur_priority_level > kPrefixOpPriorityLevel) {
        out << "\\right)";
    }
}

bool NegateOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return expr_->DeepCompare(ptr->expr_);
}

/* ---------------------------------------- Differentiate Op ---------------------------------------- */

ExpressionPtr DifferentiateOp::Simplify() {
    return expr_->Simplify()->TakeDerivative(var_name_);
}

ExpressionPtr DifferentiateOp::TakeDerivative(char var_name) {
    return std::make_shared<DifferentiateOp>(expr_->TakeDerivative(var_name_), var_name);
}

ExpressionPtr DifferentiateOp::Call(const std::vector<ExpressionPtr>& args) {
    auto func = expr_->TakeDerivative(var_name_);
    if (Is<DifferentiateOp>(func)) {
        return std::make_shared<CallOp>(func, args);
    } else {
        return func->Call(args);
    }
}

ExpressionPtr DifferentiateOp::Substitute(char var_name, const ExpressionPtr& expr) {
    return std::make_shared<DifferentiateOp>(expr_->Substitute(var_name, expr), var_name_);
}

void DifferentiateOp::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << '(';
    }
    expr_->Print(out, kPostfixOpPriorityLevel);
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << ')';
    }
    out << "'_" << var_name_;
}

void DifferentiateOp::TexDump(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel + 1) {
        out << "\\left(";
    }
    out << '{';
    expr_->TexDump(out, kPostfixOpPriorityLevel + 1);
    out << '}';
    if (cur_priority_level > kPostfixOpPriorityLevel + 1) {
        out << "\\right)";
    }
    out << "'_" << var_name_;
}

bool DifferentiateOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return var_name_ == ptr->var_name_ && expr_->DeepCompare(ptr->expr_);
}

/* ---------------------------------------- Call Op ---------------------------------------- */

ExpressionPtr CallOp::Simplify() {
    std::vector<ExpressionPtr> simplified_args;
    simplified_args.reserve(args_.size());
    for (const auto& arg : args_) {
        simplified_args.push_back(arg->Simplify());
    }
    return func_->Simplify()->Call(simplified_args);
}

ExpressionPtr CallOp::TakeDerivative(char var_name) {
    if (args_.size() == 1) {
        auto result = std::make_shared<Product>();
        result->ReserveSize(2);
        *result *= args_[0]->TakeDerivative(var_name);
        *result *= std::make_shared<CallOp>(func_->TakeDerivative(var_name), args_);
        return result;
    }
    return std::make_shared<DifferentiateOp>(shared_from_this(), var_name);
}

ExpressionPtr CallOp::Call(const std::vector<ExpressionPtr>&/* args*/) {
    return shared_from_this();
    /*
    std::vector<ExpressionPtr> called_args;
    called_args.reserve(args_.size());

    for (const auto& arg : args_) {
        called_args.push_back(arg->Call(args));
    }

    return std::make_shared<CallOp>(func_->Call(called_args), args);
    */
}

ExpressionPtr CallOp::Substitute(char var_name, const ExpressionPtr& value) {
    std::vector<ExpressionPtr> new_args;
    new_args.reserve(args_.size());

    for (const auto& arg : args_) {
        new_args.push_back(arg->Substitute(var_name, value));
    }

    return std::make_shared<CallOp>(func_->Substitute(var_name, value), new_args);

}

void CallOp::Print(std::ostream& out, int cur_priority_level) const {
    func_->Print(out, cur_priority_level);
    out << '(';
    if (!args_.empty()) {
        args_[0]->Print(out, kSumPriorityLevel);
        for (size_t i = 1; i < args_.size(); ++i) {
            out << ", ";
            args_[i]->Print(out, kSumPriorityLevel);
        }
    }
    out << ')';
}

void CallOp::TexDump(std::ostream& out, int cur_priority_level) const {
    func_->TexDump(out, cur_priority_level);
    out << "\\left(";
    if (!args_.empty()) {
        args_[0]->TexDump(out, kSumPriorityLevel);
        for (size_t i = 1; i < args_.size(); ++i) {
            out << ", ";
            args_[i]->TexDump(out, kSumPriorityLevel);
        }
    }
    out << "\\right)";
}

bool CallOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    if (args_.size() != ptr->args_.size()) {
        return false;
    }
    if (!func_->DeepCompare(ptr->func_)) {
        return false;
    }
    for (size_t i = 0; i < args_.size(); ++i) {
        if (!args_[i]->DeepCompare(ptr->args_[i])) {
            return false;
        }
    }
    return true;
}

/* ---------------------------------------- PowerOp ---------------------------------------- */

ExpressionPtr PowerOp::Simplify() {
    if (Is<Constant>(base_)) {
        double base = As<Constant>(base_)->GetValue();
        if (IsZero(base)) {
            return kConstantZero;
        }
        if (IsZero(base - 1)) {
            return kConstantOne;
        }
    }

    if (Is<Constant>(exp_)) {
        double exp = As<Constant>(exp_)->GetValue();
        if (IsZero(exp)) {
            return kConstantOne;
        }
        if (IsZero(exp - 1)) {
            return base_;
        }
        if (Is<Constant>(base_)) {
            return BuildConstant(std::pow(As<Constant>(base_)->GetValue(), exp));
        }
    }
    if (Is<PowerOp>(base_)) {
        auto new_exp = std::make_shared<Product>();
        *new_exp *= exp_;
        *new_exp *= As<PowerOp>(base_)->exp_;
        return std::make_shared<PowerOp>(As<PowerOp>(base_)->base_, new_exp)->Simplify();
    }
    if (Is<Product>(base_)) {
        auto multipliers_copy = As<Product>(base_)->GetOperands();
        for (auto& multiplier : multipliers_copy) {
            multiplier.expr = std::make_shared<PowerOp>(multiplier.expr, exp_);
        }
        return std::make_shared<Product>(std::move(multipliers_copy))->Simplify();
    }
    return std::make_shared<PowerOp>(base_->Simplify(), exp_->Simplify());
}

ExpressionPtr PowerOp::TakeDerivative(char var_name) {
    auto prod1 = std::make_shared<Product>();
    auto sum = std::make_shared<Sum>();
    auto prod2 = std::make_shared<Product>();
    auto prod3 = std::make_shared<Product>();

    *prod2 *= exp_->TakeDerivative(var_name);
    *prod2 *= std::make_shared<CallOp>(std::make_shared<Function>("log"), std::vector<ExpressionPtr>{base_});

    *prod3 *= exp_;
    *prod3 *= base_->TakeDerivative(var_name);
    *prod3 /= base_;

    *sum += prod2;
    *sum += prod3;

    *prod1 *= sum;
    *prod1 *= shared_from_this();
    return prod1;
}

ExpressionPtr PowerOp::Call(const std::vector<ExpressionPtr>& args) {
    return std::make_shared<PowerOp>(base_->Call(args), exp_->Call(args));
}

ExpressionPtr PowerOp::Substitute(char var_name, const ExpressionPtr& value) {
    return std::make_shared<PowerOp>(base_->Substitute(var_name, value), exp_->Substitute(var_name, value));
}

void PowerOp::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << '(';
    }
    base_->Print(out, kPostfixOpPriorityLevel);
    out << " ^ ";
    exp_->Print(out, kPostfixOpPriorityLevel);
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << ')';
    }
}

void PowerOp::TexDump(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << "\\left(";
    }
    if (Is<Constant>(exp_) && IsZero(As<Constant>(exp_)->GetValue() - 0.5)) {
        out << "\\sqrt{";
        base_->TexDump(out, kPostfixOpPriorityLevel);
        out << '}';
    } else {
        base_->TexDump(out, kPostfixOpPriorityLevel);
        out << "^{";
        exp_->TexDump(out, kPostfixOpPriorityLevel);
        out << '}';
    }
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << "\\right)";
    }
}

bool PowerOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return base_->DeepCompare(ptr->base_) && exp_->DeepCompare(ptr->exp_);
}

/* ---------------------------------------- SubstOp ---------------------------------------- */

ExpressionPtr SubstOp::Simplify() {
    return target_->Simplify()->Substitute(var_name_, value_->Simplify());
}

ExpressionPtr SubstOp::TakeDerivative(char var_name) {
    return Simplify()->TakeDerivative(var_name);
}

ExpressionPtr SubstOp::Call(const std::vector<ExpressionPtr>& args) {
    return Simplify()->Call(args);
}

ExpressionPtr SubstOp::Substitute(char var_name, const ExpressionPtr& value) {
    return Simplify()->Substitute(var_name, value);
}

void SubstOp::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << '(';
    }
    target_->Print(out, kPostfixOpPriorityLevel);
    out << '[' << var_name_ << " = ";
    value_->Print(out, kSumPriorityLevel);
    out << ']';
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << ')';
    }
}

void SubstOp::TexDump(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << "\\left(";
    }
    out << "\\left.";
    target_->TexDump(out, kPostfixOpPriorityLevel);
    out << "\\right|_{" << var_name_ << " = ";
    value_->TexDump(out, kSumPriorityLevel);
    out << "}";
    if (cur_priority_level > kPostfixOpPriorityLevel) {
        out << "\\right)";
    }
}

bool SubstOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return var_name_ == ptr->var_name_ && target_->DeepCompare(ptr->target_) && value_->DeepCompare(ptr->value_);
}

/* ---------------------------------------- Misc ---------------------------------------- */

static double RatioOfSums(const std::vector<AssociativeOperand>& l_summands, const std::vector<AssociativeOperand>& r_summands) {
    std::vector<bool> l_used(l_summands.size(), false);
    std::vector<bool> r_used(r_summands.size(), false);
    std::vector<std::vector<double>> ratios(l_summands.size(), std::vector<double>(r_summands.size(), std::nan("")));

    double final_ratio = std::nan("");

    for (size_t l_index = 0; l_index < l_summands.size(); ++l_index) {
        if (l_used[l_index]) {
            continue;
        }

        l_used[l_index] = true;

        std::vector<int> r_active;
        for (size_t r_index = 0; r_index < r_summands.size(); ++r_index) {
            if (r_used[r_index]) {
                continue;
            }

            ratios[l_index][r_index] = Ratio(l_summands[l_index].expr, r_summands[r_index].expr);
            if (l_summands[l_index].inverse ^ r_summands[r_index].inverse) {
                ratios[l_index][r_index] *= -1;
            }

            if (!std::isnan(ratios[l_index][r_index])) {
                r_used[r_index] = true;
                r_active.push_back(r_index);
            }
        }

        if (r_active.empty()) {
            return std::nan("");
        }

        std::vector<size_t> l_active{l_index};

        for (size_t l_candidate = l_index + 1; l_candidate < l_summands.size(); ++l_candidate) {
            bool success = true;
            for (int r_index : r_active) {
                ratios[l_candidate][r_index] = Ratio(l_summands[l_candidate].expr, r_summands[r_index].expr);
                if (l_summands[l_candidate].inverse ^ r_summands[r_index].inverse) {
                    ratios[l_candidate][r_index] *= -1;
                }
                if (std::isnan(ratios[l_candidate][r_index])) {
                    success = false;
                    break;
                }
            }
            if (success) {
                l_active.push_back(l_candidate);
                l_used[l_candidate] = true;
            }
        }

        double current_ratio = 0;
        for (auto r_index : r_active) {
            double sum = 0;
            for (auto l_index : l_active) {
                sum += ratios[l_index][r_index];
            }
            current_ratio += 1 / sum;
        }
        current_ratio = 1 / current_ratio;

        if (!std::isnan(final_ratio) && !IsZero(final_ratio - current_ratio)) {
            return std::nan("");
        }
        final_ratio = current_ratio;
    }

    return final_ratio;
}

static double RatioOfProducts(const std::vector<AssociativeOperand>& l_multipliers, const std::vector<AssociativeOperand>& r_multipliers) {
    std::vector<bool> used(r_multipliers.size(), false);

    double const_ratio = 1;

    for (size_t i = 0; i < r_multipliers.size(); ++i) {
        if (Is<Constant>(r_multipliers[i].expr)) {
            used[i] = true;
            double value = As<Constant>(r_multipliers[i].expr)->GetValue();
            if (r_multipliers[i].inverse) {
                const_ratio *= value;
            } else {
                const_ratio /= value;
            }
        }
    }

    for (size_t l_index = 0; l_index < l_multipliers.size(); ++l_index) {
        if (Is<Constant>(l_multipliers[l_index].expr)) {
            double value = As<Constant>(l_multipliers[l_index].expr)->GetValue();
            if (l_multipliers[l_index].inverse) {
                const_ratio /= value;
            } else {
                const_ratio *= value;
            }
            continue;
        }

        bool success = false;
        for (size_t r_index = 0; r_index < r_multipliers.size(); ++r_index) {
            if (!used[r_index] && l_multipliers[l_index].inverse == r_multipliers[r_index].inverse &&
                    l_multipliers[l_index].expr->DeepCompare(r_multipliers[r_index].expr)) {
                success = true;
                used[r_index] = true;
                break;
            }
        }

        if (!success) {
            return std::nan("");
        }
    }

    for (bool flag : used) {
        if (!flag) {
            return std::nan("");
        }
    }
    return const_ratio;
}

double Ratio(const ExpressionPtr& lhs, const ExpressionPtr& rhs) {
    if (Is<NegateOp>(lhs)) {
        return -Ratio(As<NegateOp>(lhs)->GetInnerExpr(), rhs);
    }
    if (Is<NegateOp>(rhs)) {
        return -Ratio(lhs, As<NegateOp>(rhs)->GetInnerExpr());
    }

    if (Is<Constant>(lhs) && Is<Constant>(rhs)) {
        return As<Constant>(lhs)->GetValue() / As<Constant>(rhs)->GetValue();
    }

    if (lhs->DeepCompare(rhs)) {
        return 1;
    }

    if (Is<Product>(lhs)) {
        const auto& l_multipliers = As<Product>(lhs)->GetOperands();
        if (l_multipliers.size() == 2) {
            if (Is<Constant>(l_multipliers[0].expr) && !l_multipliers[1].inverse) {
                double r = Ratio(l_multipliers[1].expr, rhs);
                double alpha = As<Constant>(l_multipliers[0].expr)->GetValue();
                return l_multipliers[0].inverse ? r / alpha : r * alpha;
            }
            if (Is<Constant>(l_multipliers[1].expr) && !l_multipliers[0].inverse) {
                double r = Ratio(l_multipliers[0].expr, rhs);
                double alpha = As<Constant>(l_multipliers[1].expr)->GetValue();
                return l_multipliers[1].inverse ? r / alpha : r * alpha;
            }
        }

        if (Is<Product>(rhs)) {
            const auto& r_multipliers = As<Product>(rhs)->GetOperands();
            return RatioOfProducts(l_multipliers, r_multipliers);
        }
    } else if (Is<Product>(rhs)) {
        return 1 / Ratio(rhs, lhs);
    }

    if (Is<Sum>(lhs) && Is<Sum>(rhs)) {
        const auto& l_summands = As<Sum>(lhs)->GetOperands();
        const auto& r_summands = As<Sum>(rhs)->GetOperands();
        return RatioOfSums(l_summands, r_summands);
    }

    return std::nan("");
}

}
