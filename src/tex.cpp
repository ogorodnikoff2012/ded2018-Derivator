#include <calculus_grammar.h>
#include <iostream>
#include <errno.h>
#include <error.h>
#include <tex_phrases.h>

static constexpr int kMaxSteps = 100;

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
        std::cout << "\\section{}\n";
        std::cout << R"(
\textbf{Input:}
\begin{tcolorbox}[colback=yellow!40]
\begin{minipage}{0.9\textwidth}\begin{verbatim}
)" << line << R"(
\end{verbatim}
\end{minipage}
\end{tcolorbox}
)";
        try {
            auto expr = parser.Parse(line)->BuildExpression();
            int step_counter = 0;
            while (true) {
                if (++step_counter > kMaxSteps) {
                    throw std::runtime_error("The maximum iterations number has been exceeded.");
                }
                std::cout << "\n\nStep \\#" << step_counter;
                std::cout << kTexMathBegin;
                expr->TexDump(std::cout);
                std::cout << kTexMathEnd;

                auto new_expr = expr->Simplify();
                if (new_expr->DeepCompare(expr)) {
                    std::cout << R"(\textbf{Result:} \begin{tcolorbox}[colback=green!40])" << kTexMathBegin;
                    new_expr->TexDump(std::cout);
                    std::cout << kTexMathEnd << "\\end{tcolorbox}\n";
                    break;
                }
                expr = new_expr;
            }
        } catch (const std::exception& e) {
            std::cout << R"(\textbf{Result:} \begin{tcolorbox}[colback=red!40])";
            std::cout << kTexError << "\\texttt{" << e.what() << "}";
            std::cout << "\\end{tcolorbox}\n";
        }
    }

    std::cout << kTexEnd << std::endl;

    return 0;
}
