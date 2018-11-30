#ifndef PREPROC_ONLY
#include <vector>
#include <memory>
#include <string>
#include <regex>
#endif

#define DEFGRAMMAR(name) namespace name##Grammar {
#define DEFBASICNODE() \
class ASTNodeBasic : public std::enable_shared_from_this<ASTNodeBasic> { \
public: \
    using NodePtr = std::shared_ptr<ASTNodeBasic>; \
    void Tx() { txs_.push_back(children_.size()); } \
    void Rollback() { children_.resize(txs_.back()); txs_.pop_back(); } \
    void Commit() {txs_.pop_back(); } \
    const auto& GetChildren() const { return children_; } \
    void Append(const char* name, std::shared_ptr<ASTNodeBasic>&& node) { \
        children_.emplace_back(name, std::move(node)); \
    }\
    virtual ~ASTNodeBasic() = default; \
protected:\
    std::vector<std::pair<const char*, std::shared_ptr<ASTNodeBasic>>> children_; \
    std::vector<std::size_t> txs_; \

#define ENDBASICNODE() }; \
class SyntaxError : public std::runtime_error {\
public: explicit SyntaxError(const std::string& s) : std::runtime_error(s) {} \
}; \
class Parser { \
    using NodePtr = std::shared_ptr<ASTNodeBasic>;

#define ENDGRAMMAR(name) \
    private: const std::string* str_; std::string::const_iterator pos_; \
};}

#define DEFRULE(name) \
public: class name##Node : public ASTNodeBasic {

#define BEGINRULE(name) }; \
private: NodePtr Parse##name() { NodePtr result = std::make_shared< name##Node >();

#define ENDRULE(name) return result; }

#define DEFRULES()
#define ENDRULES()

#define EXPECT(name, body) { NodePtr child = body; result->Append( #name, std::move(child)); }

#define TX result->Tx(); auto old_pos = pos_;
#define COMMIT result->Commit();
#define ROLLBACK result->Rollback(); pos_ = old_pos;

#define ASTERISK(body) while (true) {\
    TX; \
    try {\
        body; \
        COMMIT; \
    } catch (SyntaxError&) {\
        ROLLBACK;\
        break;\
    }\
}
#define OR(left, right) {\
    TX;\
    try {\
        left;\
        COMMIT;\
    } catch (SyntaxError) {\
        ROLLBACK;\
        right;\
    }\
}

#define OR3(a, b, c) OR(a, OR(b, c))
#define MAYBE(body) OR(body, )
#define DEFTOKENS()

#define ENDTOKENS()
#define DEFTOKEN(name, regexp) \
    private: std::regex Regex##name {"[[:s:]]*(" regexp ").*"}; \
NodePtr NextToken##name() { \
    std::smatch match; \
    bool success = std::regex_match(pos_, str_->cend(), match, Regex##name );\
    if (!success) {\
        std::string what = __func__;\
        what += ": Bad token at pos ";\
        what += std::to_string(pos_ - str_->cbegin());\
        throw SyntaxError(what); \
    }\
    auto submatch = match[1]; \
    pos_ += match.position(1) + submatch.length(); \
    return std::make_shared<name##Node>(submatch.str());\
}\
    public: class name##Node : public ASTNodeBasic { \
        public: explicit name##Node(const std::string& str) : str_(str) {}\
                const std::string& GetStr() const { return str_; }\
        private: std::string str_;

#define ENDTOKEN() };

#define MAINRULE(name) public: NodePtr Parse(const std::string& str) { str_ = &str; pos_ = str.begin(); return Parse##name(); }
#define RULE(name) Parse##name()
#define TOKEN(name) NextToken##name()
