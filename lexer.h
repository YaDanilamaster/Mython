#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_map>

namespace parse {
    using namespace std::string_view_literals;

    namespace token_type {
        struct Number {  // Лексема «число»
            int value;   // число
        };

        struct Id {             // Лексема «идентификатор»
            std::string value;  // Имя идентификатора
        };

        struct Char {    // Лексема «символ»
            char value;  // код символа
        };

        struct String {  // Лексема «строковая константа»
            std::string value;
        };

        struct Class {};    // Лексема «class»
        struct Return {};   // Лексема «return»
        struct If {};       // Лексема «if»
        struct Else {};     // Лексема «else»
        struct Def {};      // Лексема «def»
        struct Newline {};  // Лексема «конец строки»
        struct Print {};    // Лексема «print»
        struct Indent {};  // Лексема «увеличение отступа», соответствует двум пробелам
        struct Dedent {};  // Лексема «уменьшение отступа»
        struct Eof {};     // Лексема «конец файла»
        struct And {};     // Лексема «and»
        struct Or {};      // Лексема «or»
        struct Not {};     // Лексема «not»
        struct Eq {};      // Лексема «==»
        struct NotEq {};   // Лексема «!=»
        struct LessOrEq {};     // Лексема «<=»
        struct GreaterOrEq {};  // Лексема «>=»
        struct None {};         // Лексема «None»
        struct True {};         // Лексема «True»
        struct False {};        // Лексема «False»
    }  // namespace token_type

    using TokenBase
        = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
        token_type::Class, token_type::Return, token_type::If, token_type::Else,
        token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
        token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
        token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
        token_type::None, token_type::True, token_type::False, token_type::Eof>;

    struct Token : TokenBase {
        using TokenBase::TokenBase;

        template <typename T>
        [[nodiscard]] bool Is() const {
            return std::holds_alternative<T>(*this);
        }

        template <typename T>
        [[nodiscard]] const T& As() const {
            return std::get<T>(*this);
        }

        template <typename T>
        [[nodiscard]] const T* TryAs() const {
            return std::get_if<T>(this);
        }
    };

    bool operator==(const Token& lhs, const Token& rhs);
    bool operator!=(const Token& lhs, const Token& rhs);

    std::ostream& operator<<(std::ostream& os, const Token& rhs);

    class LexerError : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    class Lexer {
    public:
        explicit Lexer(std::istream& input);

        // Возвращает ссылку на текущий токен или token_type::Eof, если поток токенов закончился
        [[nodiscard]] const Token& CurrentToken() const;

        // Возвращает следующий токен, либо token_type::Eof, если поток токенов закончился
        Token NextToken();

        // Если текущий токен имеет тип T, метод возвращает ссылку на него.
        // В противном случае метод выбрасывает исключение LexerError
        template <typename T>
        const T& Expect() const {
            using namespace std::literals;
            if (CurrentToken().Is<T>()) {
                return CurrentToken().As<T>();
            }
            throw LexerError("Not implemented"s);
        }

        // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит значение value.
        // В противном случае метод выбрасывает исключение LexerError
        template <typename T, typename U>
        void Expect(const U& value) const {
            using namespace std::literals;
            if (!CurrentToken().Is<T>() || CurrentToken().As<T>().value != value) {
                throw LexerError("Not implemented"s);
            }
        }

        // Если следующий токен имеет тип T, метод возвращает ссылку на него.
        // В противном случае метод выбрасывает исключение LexerError
        template <typename T>
        const T& ExpectNext() {
            NextToken();
            return Expect<T>();
        }

        // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит значение value.
        // В противном случае метод выбрасывает исключение LexerError
        template <typename T, typename U>
        void ExpectNext(const U& value) {
            using namespace std::literals;
            const Token& next = NextToken();
            if (!next.Is<T>() || next.As<T>().value != value) {
                throw LexerError("Not implemented"s);
            }
        }

    private:

        // Реализуйте приватную часть самостоятельно
        std::istreambuf_iterator<char> isit_, end_;
        int curent_indent_;                     // счетчик текущего отступа
        std::vector<Token> tokens_;             // буфер с токенами
        size_t curent_tiken_id_;                // id текущего токена
        size_t line_count_;                     // количество строк

        // Парсим входную строку
        bool ParseString();

        // Проверяем на комментарий
        bool ExpectComment();

        // Пропускаем пробелы
        bool SkipSpace();

        // Ожидаем ключевое слово
        bool ExpectKeyWord(const std::string_view str);

        // Ожидаем отступы. Считаем количество токенов с отступами
        bool ExpectIndent();

        // Ожидаем идентификатор или ключевое слово
        bool ExpectIdKeyWord();

        // Ожидаем символы
        bool ExpectChars();

        // Ожидаем число
        bool ExpectNums();

        // Ожидаем строку
        bool ExpectString();

        // или числа или строки или комментарий


        const std::unordered_map <std::string_view, Token> keyword_{
            //class return if else def print or None and not True False
            {"class"sv, token_type::Class{}},
            {"return"sv, token_type::Return{}},
            {"if"sv, token_type::If{}},
            {"else"sv, token_type::Else{}},
            {"def"sv, token_type::Def{}},
            {"print"sv, token_type::Print{}},
            {"or"sv, token_type::Or{}},
            {"None"sv, token_type::None{}},
            {"and"sv, token_type::And{}},
            {"not"sv, token_type::Not{}},
            {"True"sv, token_type::True{}},
            {"False"sv, token_type::False{}}
        };

        const std::vector<char> chars_{ '.', ',', '(', '+', ')', '-', '+', '*', '/', ':' };
        const std::vector<char> equals_{ '=', '<', '>', '!' };
    };

}  // namespace parse