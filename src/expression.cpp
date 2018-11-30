#include <expression.h>
#include <unordered_map>
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

const ExpressionPtr kConstantZero = std::make_shared<Constant>(0);
const ExpressionPtr kConstantOne  = std::make_shared<Constant>(1);
const ExpressionPtr kConstantPi   = std::make_shared<Constant>(M_PI);

constexpr char kVariableStubName = '_';
const ExpressionPtr kVariableStub = std::make_shared<Variable>(kVariableStubName);

static const std::unordered_map<std::string, ExpressionPtr> kTableOfDerivatives = {
    {"sin", std::make_shared<Function>("cos")},
    {"cos", std::make_shared<NegateOp>(std::make_shared<Function>("sin"))},
    {"log", Inverse(kVariableStub)},
    {"exp", std::make_shared<Function>("exp")},
};

static const std::unordered_map<std::string, double(*)(double)> kUnaryFunctionTable = {
    {"sin", std::sin},
    {"cos", std::cos},
    {"log", std::log},
    {"exp", std::exp},
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

ExpressionPtr Constant::Simplify() {
    return shared_from_this();
}

ExpressionPtr Constant::TakeDerivative(char) {
    return kConstantZero;
}

ExpressionPtr Constant::Call(const std::vector<ExpressionPtr>&) {
    return shared_from_this();
}

void Constant::Print(std::ostream& out, int) const {
    out << value_;
}

bool Constant::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return std::fabs(value_ - ptr->value_) < kDoubleTolerance;
}

ExpressionPtr Variable::Simplify() {
    return shared_from_this();
}

ExpressionPtr Variable::TakeDerivative(char var_name) {
    return (var_name == name_ || name_ == kVariableStubName) ? kConstantOne : kConstantZero;
}

ExpressionPtr Variable::Call(const std::vector<ExpressionPtr>& args) {
    if (args.size() == 1) {
        return name_ == kVariableStubName ? args[0] : shared_from_this();
    }
    if (args.size() != 2) {
        std::string what = "Argument count mismatch: expected 2, got ";
        what += std::to_string(args.size());
        throw RuntimeError(what);
    }
    if (!Is<Variable>(args[0])) {
        throw RuntimeError("Bad first argument, expected variable");
    }
    return As<Variable>(args[0])->GetName() == name_ ? args[1] : shared_from_this();
}

void Variable::Print(std::ostream& out, int) const {
    out << name_;
}

bool Variable::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return name_ == ptr->name_;
}

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
    auto iter = kUnaryFunctionTable.find(name_);
    if (!Is<Constant>(arg) || iter == kUnaryFunctionTable.end()) {
        return std::make_shared<CallOp>(shared_from_this(), args);
    }

    double result = iter->second(As<Constant>(arg)->GetValue());
    return BuildConstant(result);
}

void Function::Print(std::ostream& out, int) const {
    out << name_;
}

bool Function::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return name_ == ptr->name_;
}

template <class T>
void AssociativeOpAlignment(const std::vector<ExpressionPtr>& operands, const std::vector<bool>& inverse, bool global_inverse,
        std::vector<ExpressionPtr>* op_result, std::vector<bool>* inv_result) {
    for (size_t i = 0; i < operands.size(); ++i) {
        if (Is<T>(operands[i])) {
            auto op = As<T>(operands[i]);
            AssociativeOpAlignment<T>(op->GetOperands(), op->GetInverses(), inverse[i] ^ global_inverse, op_result, inv_result);
        } else {
            op_result->push_back(operands[i]);
            inv_result->push_back(inverse[i] ^ global_inverse);
        }
    }
}

ExpressionPtr Sum::Simplify() {
    if (summands_.size() == 1) {
        if (negate_[0]) {
            return std::make_shared<NegateOp>(summands_[0]->Simplify())->Simplify();
        } else {
            return summands_[0]->Simplify();
        }
    }

    std::vector<ExpressionPtr> summands_copy;
    std::vector<bool> negate_copy;

    AssociativeOpAlignment<Sum>(summands_, negate_, false, &summands_copy, &negate_copy);

    for (size_t i = 0; i < summands_copy.size(); ++i) {
        summands_copy[i] = summands_copy[i]->Simplify();
        if (Is<NegateOp>(summands_copy[i])) {
            negate_copy[i] = !negate_copy[i];
            summands_copy[i] = As<NegateOp>(summands_copy[i])->GetInnerExpr();
        }
    }

    // Constant folding
    int last_nonconstant = summands_copy.size() - 1;
    for (int i = summands_copy.size() - 1; i >= 0; --i) {
        if (Is<Constant>(summands_copy[i])) {
            if (i != last_nonconstant) {
                summands_copy[i].swap(summands_copy[last_nonconstant]);
#ifdef _GLIBCXX_DEBUG
                bool t = negate_copy[i];
                negate_copy[i] = negate_copy[last_nonconstant];
                negate_copy[last_nonconstant] = t;
#else
                std::swap(negate_copy[i], negate_copy[last_nonconstant]);
#endif
            }
            --last_nonconstant;
        }
    }

    double value = 0;
    for (size_t i = last_nonconstant + 1; i < summands_copy.size(); ++i) {
        double summand = As<Constant>(summands_copy[i])->GetValue();
        if (negate_copy[i]) {
            value -= summand;
        } else {
            value += summand;
        }
    }

    if (!IsZero(value) || last_nonconstant < 0) {
        summands_copy.resize(last_nonconstant + 2);
        negate_copy.resize(last_nonconstant + 2);
        summands_copy.back() = BuildConstant(value);
        negate_copy.back() = false;
    } else {
        summands_copy.resize(last_nonconstant + 1);
        negate_copy.resize(last_nonconstant + 1);
    }


    if (summands_copy.size() == 1) {
        if (negate_copy[0]) {
            return std::make_shared<NegateOp>(summands_copy[0]->Simplify())->Simplify();
        } else {
            return summands_copy[0]->Simplify();
        }
    }
    return std::make_shared<Sum>(std::move(summands_copy), std::move(negate_copy));
}

ExpressionPtr Sum::TakeDerivative(char var_name) {
    std::vector<ExpressionPtr> summands;
    summands.reserve(summands_.size());
    for (const auto& summand : summands_) {
        summands.emplace_back(summand->TakeDerivative(var_name));
    }

    return std::make_shared<Sum>(std::move(summands), negate_);
}

ExpressionPtr Sum::Call(const std::vector<ExpressionPtr>& args) {
    std::vector<ExpressionPtr> summands;
    summands.reserve(summands_.size());
    for (const auto& summand : summands_) {
        summands.emplace_back(summand->Call(args));
    }

    return std::make_shared<Sum>(std::move(summands), negate_);
}

void Sum::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kSumPriorityLevel) {
        out << '(';
    }

    if (negate_[0]) {
        out << '-';
    }
    summands_[0]->Print(out, kSumPriorityLevel + (negate_[0] ? 1 : 0));

    for (size_t i = 1; i < summands_.size(); ++i) {
        out << (negate_[i] ? " - " : " + ");
        summands_[i]->Print(out, kSumPriorityLevel + (negate_[i] ? 1 : 0));
    }

    if (cur_priority_level > kSumPriorityLevel) {
        out << ')';
    }
}

bool Sum::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    if (summands_.size() != ptr->summands_.size()) {
        return false;
    }
    for (size_t i = 0; i < summands_.size(); ++i) {
        if (negate_[i] != ptr->negate_[i] || !summands_[i]->DeepCompare(ptr->summands_[i])) {
            return false;
        }
    }
    return true;
}

Sum& Sum::operator+=(const ExpressionPtr& expr) {
    summands_.emplace_back(expr);
    negate_.push_back(false);
    return *this;
}

Sum& Sum::operator-=(const ExpressionPtr& expr) {
    summands_.emplace_back(expr);
    negate_.push_back(true);
    return *this;
}

void Sum::ReserveSize(int size) {
    summands_.reserve(size);
    negate_.reserve(size);
}

ExpressionPtr Product::Simplify() {
    if (multipliers_.size() == 1 && inverse_[0] == false) {
        return multipliers_[0]->Simplify();
    }
    std::vector<ExpressionPtr> multipliers_copy;
    std::vector<bool> inverse_copy;

    AssociativeOpAlignment<Product>(multipliers_, inverse_, false, &multipliers_copy, &inverse_copy);

    bool need_to_be_negated = false;

    for (size_t i = 0; i < multipliers_copy.size(); ++i) {
        multipliers_copy[i] = multipliers_copy[i]->Simplify();
        if (Is<NegateOp>(multipliers_copy[i])) {
            need_to_be_negated ^= true;
            multipliers_copy[i] = As<NegateOp>(multipliers_copy[i])->GetInnerExpr();
        }
    }

    // Constant folding
    int last_nonconstant = multipliers_copy.size() - 1;
    for (int i = multipliers_copy.size() - 1; i >= 0; --i) {
        if (Is<Constant>(multipliers_copy[i])) {
            if (i != last_nonconstant) {
                multipliers_copy[i].swap(multipliers_copy[last_nonconstant]);
#ifdef _GLIBCXX_DEBUG
                bool t = inverse_copy[i];
                inverse_copy[i] = inverse_copy[last_nonconstant];
                inverse_copy[last_nonconstant] = t;
#else
                std::swap(inverse_copy[i], inverse_copy[last_nonconstant]);
#endif
            }
            --last_nonconstant;
        }
    }

    double value = 1;
    for (size_t i = last_nonconstant + 1; i < multipliers_copy.size(); ++i) {
        double multiplier = As<Constant>(multipliers_copy[i])->GetValue();
        if (inverse_copy[i]) {
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
        inverse_copy.resize(last_nonconstant + 1);
        ExpressionPtr result = std::make_shared<Product>(std::move(multipliers_copy), std::move(inverse_copy));
        if (IsZero(value + 1) ^ need_to_be_negated) {
            return std::make_shared<NegateOp>(result);
        } else {
            return result;
        }
    }

    multipliers_copy.resize(last_nonconstant + 2);
    inverse_copy.resize(last_nonconstant + 2);
    multipliers_copy.back() = BuildConstant(value);
    inverse_copy.back() = false;

    auto result = std::make_shared<Product>(std::move(multipliers_copy), std::move(inverse_copy));
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
        auto inverse_copy = inverse_;
        multipliers_copy[i] = multipliers_copy[i]->TakeDerivative(var_name);
        inverse_copy[i] = false;

        auto summand = std::make_shared<Product>(std::move(multipliers_copy), std::move(inverse_copy));
        if (inverse_[i]) {
            *summand /= multipliers_[i];
            *summand /= multipliers_[i];
            *result -= summand;
        } else {
            *result += summand;
        }
    }

    return result;
}

ExpressionPtr Product::Call(const std::vector<ExpressionPtr>& args) {
    std::vector<ExpressionPtr> multipliers;
    multipliers.reserve(multipliers_.size());
    for (const auto& multiplier : multipliers_) {
        multipliers.emplace_back(multiplier->Call(args));
    }

    return std::make_shared<Product>(std::move(multipliers), inverse_);
}

void Product::Print(std::ostream& out, int cur_priority_level) const {
    if (cur_priority_level > kProdPriorityLevel) {
        out << '(';
    }

    if (inverse_[0]) {
        out << "1 / ";
    }
    multipliers_[0]->Print(out, kProdPriorityLevel + (inverse_[0] ? 1 : 0));

    for (size_t i = 1; i < multipliers_.size(); ++i) {
        out << (inverse_[i] ? " / " : " * ");
        multipliers_[i]->Print(out, kProdPriorityLevel + (inverse_[i] ? 1 : 0));
    }
    if (cur_priority_level > kProdPriorityLevel) {
        out << ')';
    }
}

bool Product::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    if (multipliers_.size() != ptr->multipliers_.size()) {
        return false;
    }
    for (size_t i = 0; i < multipliers_.size(); ++i) {
        if (inverse_[i] != ptr->inverse_[i] || !multipliers_[i]->DeepCompare(ptr->multipliers_[i])) {
            return false;
        }
    }
    return true;
}

Product& Product::operator*=(const ExpressionPtr& expr) {
    multipliers_.emplace_back(expr);
    inverse_.push_back(false);
    return *this;
}

Product& Product::operator/=(const ExpressionPtr& expr) {
    multipliers_.emplace_back(expr);
    inverse_.push_back(true);
    return *this;
}

void Product::ReserveSize(int size) {
    multipliers_.reserve(size);
    inverse_.reserve(size);
}

ExpressionPtr NegateOp::Simplify() {
    if (Is<Constant>(expr_)) {
        return BuildConstant(-As<Constant>(expr_)->GetValue());
    }
    if (Is<NegateOp>(expr_)) {
        return As<NegateOp>(expr_)->GetInnerExpr();
    }
    return shared_from_this();
}

ExpressionPtr NegateOp::TakeDerivative(char var_name) {
    return std::make_shared<NegateOp>(expr_->TakeDerivative(var_name));
}

ExpressionPtr NegateOp::Call(const std::vector<ExpressionPtr>& args) {
    return std::make_shared<NegateOp>(expr_->Call(args));
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

bool NegateOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return expr_->DeepCompare(ptr->expr_);
}

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

bool DifferentiateOp::DeepCompare(const ExpressionPtr& other) const {
    COMPARE_CHECK_TRIVIAL
    return var_name_ == ptr->var_name_ && expr_->DeepCompare(ptr->expr_);
}

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

ExpressionPtr CallOp::Call(const std::vector<ExpressionPtr>& args) {
    std::vector<ExpressionPtr> called_args;
    called_args.reserve(args_.size());

    for (const auto& arg : args_) {
        called_args.push_back(arg->Call(args));
    }

    return std::make_shared<CallOp>(func_->Call(called_args), args);
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

}
