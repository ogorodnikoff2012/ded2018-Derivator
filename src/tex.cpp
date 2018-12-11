#include <calculus_grammar.h>
#include <iostream>
#include <errno.h>
#include <error.h>
#include <tex_phrases.h>

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        std::cerr << "Usage: " << argv[0] << " <input-file> <output-file>\n";
        return 1;
    }

    if (std::strcmp(argv[1], "-") != 0) {
        if (!std::freopen(argv[1], "r", stdin)) {
            perror("Cannot open input file");
            return 1;
        }
    }

    if (std::strcmp(argv[2], "-") != 0) {
        if (!std::freopen(argv[2], "w", stdout)) {
            perror("Cannot open output file");
            return 1;
        }
    }

    std::cout << kTexPreamble << std::endl;

    std::string line;
    std::cout.precision(20);
    CalculusGrammar::Parser parser;

    while (std::getline(std::cin, line)) {
        std::cout << "\\section{}";
        auto expr = parser.Parse(line)->BuildExpression();
        try {
            while (true) {
                std::cout << "$$";
                expr->TexDump(std::cout);
                std::cout << "$$";

                auto new_expr = expr->Simplify();
                if (new_expr->DeepCompare(expr)) {
                    break;
                }
            }
        } catch (const std::exception& e) {
            std::cout << kTexError << "\\texttt{" << e.what() << "}";
        }
    }

    std::cout << kTexEnd << std::endl;

    return 0;
}
