#include <calculus_grammar.h>
#include <iostream>

int main() {
    std::string input;
    CalculusGrammar::Parser parser;
    std::cout << "Very clever Vova calculator\n";

    std::cout.precision(20);
    std::cerr.precision(20);

    while (std::cout << ">> ", std::getline(std::cin, input)) {
        try {
            auto ast = parser.Parse(input);
            std::cerr << "AST: ";
            ast->Print(std::cerr);
            std::cerr << std::endl;

            int simplify_counter = 0;
            auto expr = ast->BuildExpression();
            while (true) {
                std::cerr << "Expression, try #" << simplify_counter << ": ";
                ++simplify_counter;
                expr->Print(std::cerr);
                std::cerr << std::endl;

                auto new_expr = expr->Simplify();
                if (expr->DeepCompare(new_expr)) {
                    break;
                }
                expr = new_expr;
            }
            expr->Print(std::cout);
            std::cout << std::endl;

        } catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }
    return 0;
}
