#include <iostream>

int main() {
#define DEFGRAMMAR(x)
#define ENDGRAMMAR(x)
#define DEFRULE(x)
#define ENDRULE(x)
#define DEFRULES()
#define ENDRULES()
#define EXPECT(name, value) value
#define ASTERISK(body) body
#define OR(left, right) { left; } { right; }
#define MAYBE(body) body
#define DEFTOKENS()
#define ENDTOKENS()
#define DEFTOKEN(x)
#define MAINRULE(x)
#define RULE(x)     std::cout << "Rule:  " << ( #x ) << std::endl;
#define TOKEN(x)    std::cout << "Token: " << ( #x ) << std::endl;
    #include <expr_grammar.h>
    return 0;
}
