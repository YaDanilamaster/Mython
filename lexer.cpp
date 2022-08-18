#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse {

	bool operator==(const Token& lhs, const Token& rhs) {
		using namespace token_type;

		if (lhs.index() != rhs.index()) {
			return false;
		}
		if (lhs.Is<Char>()) {
			return lhs.As<Char>().value == rhs.As<Char>().value;
		}
		if (lhs.Is<Number>()) {
			return lhs.As<Number>().value == rhs.As<Number>().value;
		}
		if (lhs.Is<String>()) {
			return lhs.As<String>().value == rhs.As<String>().value;
		}
		if (lhs.Is<Id>()) {
			return lhs.As<Id>().value == rhs.As<Id>().value;
		}
		return true;
	}

	bool operator!=(const Token& lhs, const Token& rhs) {
		return !(lhs == rhs);
	}

	std::ostream& operator<<(std::ostream& os, const Token& rhs) {
		using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

		VALUED_OUTPUT(Number);
		VALUED_OUTPUT(Id);
		VALUED_OUTPUT(String);
		VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

		UNVALUED_OUTPUT(Class);
		UNVALUED_OUTPUT(Return);
		UNVALUED_OUTPUT(If);
		UNVALUED_OUTPUT(Else);
		UNVALUED_OUTPUT(Def);
		UNVALUED_OUTPUT(Newline);
		UNVALUED_OUTPUT(Print);
		UNVALUED_OUTPUT(Indent);
		UNVALUED_OUTPUT(Dedent);
		UNVALUED_OUTPUT(And);
		UNVALUED_OUTPUT(Or);
		UNVALUED_OUTPUT(Not);
		UNVALUED_OUTPUT(Eq);
		UNVALUED_OUTPUT(NotEq);
		UNVALUED_OUTPUT(LessOrEq);
		UNVALUED_OUTPUT(GreaterOrEq);
		UNVALUED_OUTPUT(None);
		UNVALUED_OUTPUT(True);
		UNVALUED_OUTPUT(False);
		UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

		return os << "Unknown token :("sv;
	}

	Lexer::Lexer(std::istream& input)
		: isit_(input)
		, curent_indent_(0)
		, curent_tiken_id_(0)
		, line_count_(0)
	{
		// Реализуйте конструктор самостоятельно
		//++isit_;
		tokens_.reserve(15);
		NextToken();
	}

	const Token& Lexer::CurrentToken() const {
		return tokens_[curent_tiken_id_];
	}

	Token Lexer::NextToken() {
		if (curent_tiken_id_ + 1 < tokens_.size()) {
			return tokens_[++curent_tiken_id_];
		}

		curent_tiken_id_ = 0;
		tokens_.clear();
		while (!ParseString());

		return tokens_[0];
	}

	bool Lexer::ParseString()
	{
		while (isit_ != end_ && *isit_ != '\n')
		{
			// Ожидаем комментарии
			if (ExpectComment()) continue;

			// Ожидаем отступы
			if (tokens_.size() == 0 && ExpectIndent()) continue;

			// Пропускаем пробелы
			if (SkipSpace()) continue;

			// Ожидаем строку
			if (*isit_ == '\"' || *isit_ == '\'') {
				if (ExpectString()) continue;
			}

			// Ожидаем идентификатор или ключевое слово
			if (ExpectIdKeyWord()) continue;

			// Ожидаем символы
			if (ExpectChars()) continue;

			// Ожидаем цифры
			if (ExpectNums()) continue;

		}
		if (isit_ == end_) {
			// Перед окончанием закрываем все открытые отступы
			if (curent_indent_ > 0) {
				while (curent_indent_ > 0) {
					tokens_.push_back(token_type::Dedent{});
					curent_indent_ -= 2;
				}
			}

			// Если строка не пустая и она единственная - зачем-то надо имитировать конец строки
			if (line_count_ == 0 && tokens_.size() != 0) {
				tokens_.push_back(token_type::Newline{});
			}

			tokens_.push_back(token_type::Eof{});
		}
		else if (*isit_ == '\n') {
			if (tokens_.size() > 0) {
				tokens_.push_back(token_type::Newline{});
				++line_count_;
				++isit_;
			}
			else {
				++isit_;
				return false;
			}
		}
		return true;
	}

	bool Lexer::ExpectComment()
	{
		if (*isit_ == '#') {
			while (isit_ != end_ && *isit_ != '\n') {
				isit_++;
			}
			return true;
		}
		return false;
	}

	bool Lexer::SkipSpace()
	{
		bool skiped = false;
		while (*isit_ == ' ') {
			++isit_;
			skiped = true;
		}
		return skiped;
	}

	// Вызывается только в начале строки
	bool Lexer::ExpectIndent()
	{
		int indent = 0;
		while (*isit_ == ' ') {
			++indent; ++isit_;
		}
		int diff_indent = indent - curent_indent_;
		if (diff_indent == 0) {
			return false;
		}
		else if (diff_indent == 2) {
			curent_indent_ += 2;
			tokens_.push_back(token_type::Indent{});
			return true;
		}
		else if (diff_indent < 0 && diff_indent % 2 == 0) {
			while (diff_indent < 0) {
				tokens_.push_back(token_type::Dedent{});
				diff_indent += 2;
			}
			curent_indent_ = indent;
			return true;
		}
		return false;
	}

	bool Lexer::ExpectKeyWord(const string_view str)
	{
		auto it = keyword_.find(str);
		if (it != keyword_.end()) {
			tokens_.push_back(it->second);
			return true;
		}
		return false;
	}

	bool Lexer::ExpectIdKeyWord()
	{
		// идентификатор или ключевое слово

		auto idChar = [](const char c, const bool is_first) {
			return ((c >= 'a' && c <= 'z')
				|| (c >= 'A' && c <= 'Z')
				|| (c == '_')
				|| (!is_first && c >= '0' && c <= '9'));
		};

		bool first = true;
		string str;

		while (isit_ != end_ && idChar(*isit_, first)) {
			str += *isit_++;
			first = false;
		}

		if (!ExpectKeyWord(str) && str.size() > 0) {
			tokens_.push_back(token_type::Id{ move(str) });
		}
		return !first;
	}

	bool Lexer::ExpectChars()
	{
		// операторы сравнения, состоящие из нескольких символов: ==, >=, <=, !=;
		// отдельные символы, такие как: '=', '.', ',', '(', '+', '<', '>', ')';
		char first = *isit_;

		for (const char c : equals_) {
			if (c == first) {
				char second = *(++isit_);
				if (second == '=') {
					switch (first)
					{
					case '=':
						tokens_.push_back(token_type::Eq{});
						++isit_;
						return true;
					case '>':
						tokens_.push_back(token_type::GreaterOrEq{});
						++isit_;
						return true;
					case '<':
						tokens_.push_back(token_type::LessOrEq{});
						++isit_;
						return true;
					case '!':
						tokens_.push_back(token_type::NotEq{});
						++isit_;
						return true;
					default:
						throw LexerError("Not equal char"s);
						break;
					}
				}
				else {
					tokens_.push_back(token_type::Char{ first });
					return true;
				}
			}
		}

		for (const char c : chars_) {
			if (c == first) {
				tokens_.push_back(token_type::Char{ first });
				++isit_;
				return true;
			}
		}
		return false;
	}

	bool Lexer::ExpectNums()
	{
		using namespace std::literals;

		std::string parsed_num;

		// Считывает в parsed_num очередной символ из input
		auto read_char = [&parsed_num, this] {
			if (isit_ == end_) {
				throw LexerError("Failed to read number from stream"s);
			}
			parsed_num += static_cast<char>(*isit_++);
		};

		// Считывает одну или более цифр в parsed_num из input
		auto read_digits = [read_char, this] {
			if (!std::isdigit(*isit_)) {
				return;
			}
			while (isit_ != end_ && std::isdigit(*isit_)) {
				read_char();
			}
		};

		read_digits();

		// Пробуем преобразовать строку в int
		try {
			tokens_.push_back(token_type::Number{ std::stoi(parsed_num) });
			return true;
		}
		catch (...) {
			// В случае неудачи, например, при переполнении,
			return false;
		}
		return false;
	}

	bool Lexer::ExpectString()
	{
		// Функцию следует использовать после считывания открывающего символа ":
		using namespace std::literals;
		std::string s;
		const char openChar = *isit_;
		++isit_;
		while (true) {
			if (isit_ == end_) {
				// Поток закончился до того, как встретили закрывающую кавычку?
				throw LexerError("String parsing error");
			}
			const char ch = *isit_;
			if (ch == openChar) {
				// Встретили закрывающую кавычку
				++isit_;
				break;
			}
			else if (ch == '\\') {
				// Встретили начало escape-последовательности
				++isit_;
				if (isit_ == end_) {
					// Поток завершился сразу после символа обратной косой черты
					throw LexerError("String parsing error");
				}
				const char escaped_char = *(isit_);
				// Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
				switch (escaped_char) {
				case 'n':
					s.push_back('\n');
					break;
				case 't':
					s.push_back('\t');
					break;
				case 'r':
					s.push_back('\r');
					break;
				case '"':
					s.push_back('"');
					break;
				case '\'':
					s.push_back('\'');
					break;
				case '\\':
					s.push_back('\\');
					break;
				default:
					// Встретили неизвестную escape-последовательность
					throw LexerError("Unrecognized escape sequence \\"s + escaped_char);
				}
			}
			else if (ch == '\n' || ch == '\r') {
				// Строковый литерал внутри не может прерываться символами \r или \n
				throw LexerError("Unexpected end of line"s);
			}
			else {
				// Просто считываем очередной символ и помещаем его в результирующую строку
				s.push_back(ch);
			}
			++isit_;
		}
		tokens_.push_back(token_type::String{ move(s) });
		return true;
	}

}  // namespace parse