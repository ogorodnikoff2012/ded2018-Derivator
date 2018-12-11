#include "grammar_pre.h"
#include <ostream>

#include "expression.h"

DEFGRAMMAR(Calculus)
    DEFBASICNODE()
    public:
        virtual calculus::ExpressionPtr BuildExpression() = 0;

        virtual void Print(std::ostream& out) {
            out << GetName() << ": [";
            bool is_first = true;
            for (const auto& node : children_) {
                if (is_first) {
                    is_first = false;
                } else {
                    out << ", ";
                }
                node->Print(out);
            }
            out << ']';
        }
    ENDBASICNODE()

    MAINRULE(MainRule)

    DEFRULES()
        DEFRULE(MainRule)
            virtual calculus::ExpressionPtr BuildExpression() override {
                return children_[0]->BuildExpression();
            }
        BEGINRULE(MainRule)
            EXPECT(expr, RULE(Expression));
            TOKEN(Eoln);
        ENDRULE(MainRule)
/*
#define RULE_PRINT \
virtual void Print(std::ostream& out) override { \
    bool is_first = true; \
    for (const auto& [name, node] : children_) {\
        if (is_first) {\
            is_first = false;\
        } else {\
            out << ' ';\
        }\
        node->Print(out);\
    }\
}
*/

        DEFRULE(Expression)
            virtual calculus::ExpressionPtr BuildExpression() override {
                auto result = std::make_shared<calculus::Sum>();
                result->ReserveSize((children_.size() + 1) / 2);

                *result += children_[0]->BuildExpression();
                for (size_t i = 2; i < children_.size(); i += 2) {
                    auto expr = children_[i]->BuildExpression();
                    if (children_[i - 1]->GetType() == kPlusType) {
                        *result += expr;
                    } else {
                        *result -= expr;
                    }
                }

                return result;
            }
        BEGINRULE(Expression)
            EXPECT(term, RULE(Term));
            ASTERISK({
                OR(EXPECT(op, TOKEN(Plus)), EXPECT(op, TOKEN(Minus)));
                EXPECT(term, RULE(Term));
            });
        ENDRULE(Expression)

        DEFRULE(Term)
            virtual calculus::ExpressionPtr BuildExpression() override {
                auto result = std::make_shared<calculus::Product>();
                result->ReserveSize((children_.size() + 1) / 2);

                *result *= children_[0]->BuildExpression();
                for (size_t i = 2; i < children_.size(); i += 2) {
                    auto expr = children_[i]->BuildExpression();
                    if (children_[i - 1]->GetType() == kMultiplyType) {
                        *result *= expr;
                    } else {
                        *result /= expr;
                    }
                }

                return result;
            }
        BEGINRULE(Term)
            EXPECT(factor, RULE(Factor));
            ASTERISK({
                OR(EXPECT(op, TOKEN(Multiply)), EXPECT(op, TOKEN(Divide)));
                EXPECT(factor, RULE(Factor));
            });
        ENDRULE(Term)

        DEFRULE(Factor)
            virtual calculus::ExpressionPtr BuildExpression() override {
                int atom_pos = 0;
                while (children_[atom_pos]->GetType() != kAtomType) {
                    ++atom_pos;
                }

                auto result = children_[atom_pos]->BuildExpression();
                for (size_t i = atom_pos + 1; i < children_.size(); ++i) {
                    auto& op = children_[i]->GetChildren()[0];
                    switch (op->GetType()) {
                        case kDiffOpType:
                        {
                            if (op->GetChildren().empty()) {
                                result = std::make_shared<calculus::DifferentiateOp>(result, calculus::kDefaultDerivativeVariable);
                            } else {
                                const std::string& name = dynamic_cast<const IdentifierNode*>(op->GetChildren()[0].get())->GetStr();
                                if (name.size() == 1 && std::islower(name[0])) {
                                    result = std::make_shared<calculus::DifferentiateOp>(result, name[0]);
                                } else {
                                    throw SyntaxError("Bad variable name");
                                }
                            }
                            break;
                        }
                        case kCallOpType:
                        {
                            std::vector<calculus::ExpressionPtr> args;
                            args.reserve(op->GetChildren().size());
                            for (const auto& arg : op->GetChildren()[0]->GetChildren()) {
                                args.push_back(arg->BuildExpression());
                            }
                            result = std::make_shared<calculus::CallOp>(result, std::move(args));
                            break;
                        }
                        case kPowerOpType:
                        {
                            result = std::make_shared<calculus::PowerOp>(result, op->GetChildren()[0]->BuildExpression());
                            break;
                        }
                        case kSubstOpType:
                        {
                            const std::string& name = dynamic_cast<const IdentifierNode*>(op->GetChildren()[0].get())->GetStr();
                            if (name.size() != 1 || !std::islower(name[0])) {
                                throw SyntaxError("Bad variable name");
                            }
                            result = std::make_shared<calculus::SubstOp>(result, name[0], op->GetChildren()[1]->BuildExpression());
                            break;
                        }
                        default:
                            throw std::runtime_error("NOT IMPLEMENTED");
                    }
                }

                /* We now have only one type of prefix operators, so ... */
                if (atom_pos % 2 == 1) {
                    result = std::make_shared<calculus::NegateOp>(result);
                }

                return result;
            }
        BEGINRULE(Factor)
            ASTERISK({
                EXPECT(pre_op, RULE(PrefixOp));
            });
            EXPECT(atom, RULE(Atom));
            ASTERISK({
                EXPECT(post_op, RULE(PostfixOp));
            });
        ENDRULE(Factor)

#define NON_CALLABLE virtual calculus::ExpressionPtr BuildExpression() override {               \
    std::string what = "This method shouldn't be called: ";                                     \
    what += __PRETTY_FUNCTION__;                                                                \
    throw SyntaxError(what);                                                                    \
}

        DEFRULE(PrefixOp)
            NON_CALLABLE
        BEGINRULE(PrefixOp)
            EXPECT(neg_op, RULE(NegateOp));
        ENDRULE(PrefixOp)

        DEFRULE(NegateOp)
            NON_CALLABLE
        BEGINRULE(NegateOp)
            TOKEN(Minus);
        ENDRULE(NegateOp)

        DEFRULE(Atom)
            virtual calculus::ExpressionPtr BuildExpression() override {
                return children_[0]->BuildExpression();
            }
        BEGINRULE(Atom)
            OR3(
                EXPECT(number, TOKEN(Number)),
                EXPECT(identifier, TOKEN(Identifier)),
                {
                    TOKEN(LeftParen);
                    EXPECT(expr, RULE(Expression));
                    TOKEN(RightParen);
                }
            );
        ENDRULE(Atom)

        DEFRULE(PostfixOp)
            NON_CALLABLE
        BEGINRULE(PostfixOp)
            OR4(
                EXPECT(diff_op,  RULE(DiffOp)),
                EXPECT(call_op,  RULE(CallOp)),
                EXPECT(power_op, RULE(PowerOp)),
                EXPECT(subst_op, RULE(SubstOp))
            );
        ENDRULE(PostfixOp)

        DEFRULE(DiffOp)
            NON_CALLABLE
        BEGINRULE(DiffOp)
            OR(
                {
                    TOKEN(QuoteAndUnderscore);
                    EXPECT(var, TOKEN(Identifier))
                },
                TOKEN(Quote)
            );
        ENDRULE(DiffOp)

        DEFRULE(CallOp)
            NON_CALLABLE
        BEGINRULE(CallOp)
            TOKEN(LeftParen);
            EXPECT(args, RULE(ArgList));
            TOKEN(RightParen);
        ENDRULE(CallOp)

        DEFRULE(PowerOp)
            NON_CALLABLE
        BEGINRULE(PowerOp)
            TOKEN(Power);
            EXPECT(exponent, RULE(Atom));
        ENDRULE(PowerOp)

        DEFRULE(SubstOp)
            NON_CALLABLE
        BEGINRULE(SubstOp)
            TOKEN(LeftBracket);
            EXPECT(name, TOKEN(Identifier));
            TOKEN(Assign);
            EXPECT(value, RULE(Expression));
            TOKEN(RightBracket);
        ENDRULE(SubstOp)

        DEFRULE(ArgList)
            NON_CALLABLE
        BEGINRULE(ArgList)
            MAYBE({
                EXPECT(first_arg, RULE(Expression));
                ASTERISK({
                    TOKEN(Comma);
                    EXPECT(second_arg, RULE(Expression));
                });
            });
        ENDRULE(ArgList)
    ENDRULES()

#define TOKEN_PRINT virtual void Print(std::ostream& out) override {        \
    out << GetName() << ": " << str_;                                       \
}

    DEFTOKENS()
        DEFTOKEN(Comma, ",")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Divide, "/")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Eoln, "$")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Identifier, "[[:alpha:]][[:alnum:]_]*")
            virtual calculus::ExpressionPtr BuildExpression() override {
                if (str_.size() == 1 && std::islower(str_[0])) {
                    return std::make_shared<calculus::Variable>(str_[0]);
                } else if (str_ == "pi") {
                    return calculus::kConstantPi;
                } else {
                    return std::make_shared<calculus::Function>(str_);
                }
            }
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(LeftParen, "\\(")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Minus, "-")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Multiply, "\\*")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Number, "([0-9]+(\\.[0-9]*)?|[0-9]*\\.[0-9]+)([eE][-\\+]?[0-9]+)?")
            virtual calculus::ExpressionPtr BuildExpression() override {
                return std::make_shared<calculus::Constant>(std::stod(str_));
            }
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Plus, "\\+")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Quote, "'")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(QuoteAndUnderscore, "'_")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(RightParen, "\\)")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Assign, "=")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(Power, "\\^")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(LeftBracket, "\\[")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()

        DEFTOKEN(RightBracket, "\\]")
            NON_CALLABLE
            TOKEN_PRINT
        ENDTOKEN()
    ENDTOKENS()

#undef NON_CALLABLE
#undef TOKEN_PRINT

ENDGRAMMAR(Calculus)

#include "grammar_post.h"
