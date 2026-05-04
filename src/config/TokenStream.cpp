#include "config/Config.hpp"
#include "utils/StringUtils.hpp"
#include <stdexcept>

Config::TokenStream::TokenStream(const std::string &fileName) : currentTokenizationLine(1), currentTokenizationColumn(0), filePath(fileName), pos_(0) { }

void Config::TokenStream::push(const Token &token)		{ tokens_.push_back(token); }
void Config::TokenStream::push(const std::string &word)	{ Token t(word); t.line = currentTokenizationLine; t.column = currentTokenizationColumn; tokens_.push_back(t); }
void Config::TokenStream::push(Token::Type type)		{ Token t(type); t.line = currentTokenizationLine; t.column = currentTokenizationColumn; tokens_.push_back(t); }
bool Config::TokenStream::done() const					{ return pos_ >= tokens_.size(); }

const Config::TokenStream::Token &Config::TokenStream::peek() const
{
	if (done())
		throw std::runtime_error(StringUtils::currentTime() + filePath + ":" + StringUtils::toString(tokens_.back().line) +
											":" + StringUtils::toString(tokens_.back().column) +
											": error: unexpected end of file");
	return tokens_.at(pos_);
}

static std::string typeName(Config::TokenStream::Token::Type t);

const std::string &Config::TokenStream::expect(Token::Type type)
{
	if (done())
		throw std::runtime_error(StringUtils::currentTime() + filePath + ":" + StringUtils::toString(tokens_.back().line) +
											":" + StringUtils::toString(tokens_.back().column) +
											": error: unexpected end of file, expected " + typeName(type));
	if (tokens_.at(pos_).type != type)
		throw std::runtime_error(StringUtils::currentTime() + filePath + ":" + StringUtils::toString(tokens_.at(pos_).line) +
											":" + StringUtils::toString(tokens_.at(pos_).column) +
											": error: unexpected token '" + tokens_.at(pos_).content + "', expected " + typeName(type));
	return tokens_.at(pos_++).content;
}

void Config::TokenStream::expect(const std::string &value)
{
	if (done())
		throw std::runtime_error(StringUtils::currentTime() + filePath + ":" + StringUtils::toString(tokens_.back().line) +
											":" + StringUtils::toString(tokens_.back().column) +
											": error: unexpected end of file, expected '" + value + "'");
	if (tokens_.at(pos_).content != value)
		throw std::runtime_error(StringUtils::currentTime() + filePath + ":" + StringUtils::toString(tokens_.at(pos_).line) +
											":" + StringUtils::toString(tokens_.at(pos_).column) +
											": error: unexpected token '" + tokens_.at(pos_).content + "', expected '" + value + "'");
	++pos_;
}

bool Config::TokenStream::accept(Token::Type type)
{
	if (done() || tokens_.at(pos_).type != type)
		return false;
	++pos_;
	return true;
}

bool Config::TokenStream::accept(const std::string &value)
{
	if (done() || tokens_.at(pos_).content != value)
		return false;
	++pos_;
	return true;
}

static std::string typeName(Config::TokenStream::Token::Type t)
{
	switch (t)
	{
	case Config::TokenStream::Token::DirectiveWord:			return "word";
	case Config::TokenStream::Token::DirectiveDelimiter:	return "';'";
	case Config::TokenStream::Token::BlockOpen:				return "'{'";
	case Config::TokenStream::Token::BlockClose:			return "'}'";
	default:												return "unknown";
	}
}

void Config::TokenStream::skipBlock()
{
	int depth = 0;
	while (!done())
	{
		if (tokens_.at(pos_).type == Token::BlockOpen)
			++depth;
		else if (tokens_.at(pos_).type == Token::BlockClose && depth-- == 1)
		{
			++pos_;
			return ;
		}
		++pos_;
	}
	throw std::runtime_error(StringUtils::currentTime() + filePath + ":" + StringUtils::toString(tokens_.back().line) +
										":" + StringUtils::toString(tokens_.back().column) +
										": error: unexpected end of file, unclosed block");
}

void Config::TokenStream::skipDirective()
{
	while (!done() && tokens_.at(pos_).type != Token::DirectiveDelimiter)
		++pos_;
	if (done())
		throw std::runtime_error(StringUtils::currentTime() + filePath + ":" + StringUtils::toString(tokens_.back().line) +
											":" + StringUtils::toString(tokens_.back().column) +
											": error: unexpected end of file, missing ';'");
	++pos_;
}

size_t Config::TokenStream::getPosition() const		{ return pos_; }
void Config::TokenStream::setPosition(size_t pos)	{ pos_ = pos; }

Config::TokenStream::Token::Token() : type(DirectiveWord), line(0), column(0) { }
Config::TokenStream::Token::Token(const std::string &inputContent) : content(inputContent), type(DirectiveWord), line(0), column(0) { }
Config::TokenStream::Token::Token(Type inputType) : type(inputType), line(0), column(0) { }
