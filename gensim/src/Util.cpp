/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <string>
#include <sstream>
#include <fstream>

#include <string.h>

#include "define.h"
#include "Util.h"

#include <antlr3.h>

namespace gensim
{
	namespace util
	{

		uint8_t Util::Verbose_Level = 0;

		expression *expression::CreateID(const std::string &id)
		{
			expression *exp = new expression();
			exp->node_str = id;
			exp->nodetype = EXPNODE_ID;

			return exp;
		}

		const expression *expression::Parse(const ArchC::AstNode &node)
		{
			expression *exp = new expression();

			switch(node.GetType()) {
				case ArchCNodeType::Expression: {
					// op, lhs, rhs
					exp->nodetype = EXPNODE_OPERATOR;
					std::string opstring = node[0].GetString();
					if(opstring == "=") {
						opstring = "==";
					}
					exp->node_str = opstring;

					exp->subexpressions.push_back(Parse(node[1]));
					exp->subexpressions.push_back(Parse(node[2]));

					break;
				}

				case ArchCNodeType::INT: {
					exp->nodetype = EXPNODE_VAL;
					exp->node_val = node.GetInt();
					break;
				}

				case ArchCNodeType::Identifier: {
					exp->nodetype = EXPNODE_ID;
					exp->node_str = node[0].GetString();
					break;
				}
				default: {
					UNEXPECTED;
				}
			}

			return exp;
		}

		std::string expression::ToString(std::string this_str, std::string (*fn_process_fn)(std::string, std::string, const std::vector<const expression *>&)) const
		{
			std::stringstream str;
			str << "(";
			if (nodetype == EXPNODE_FNCALL) {
				str << fn_process_fn(this_str, node_str, subexpressions);
			} else {
				if (subexpressions.size() == 2)  // if we have left and right subexprs
					str << subexpressions[0]->ToString(this_str, fn_process_fn);
				switch (nodetype) {
					case EXPNODE_ID:
						str << this_str << "." << node_str;
						break;
					case EXPNODE_OPERATOR:
						str << node_str;
						break;
					case EXPNODE_VAL:
						str << node_val;
						break;
					default:
						GASSERT(false);
				}
				if (subexpressions.size() == 2)  // if we have left and right subexprs
					str << subexpressions[1]->ToString(this_str, fn_process_fn);
			}
			str << ")";
			return str.str();
		}

		uint32_t Util::parse_binary(std::string str)
		{
			uint32_t val = 0;
			uint32_t bit = 1 << (str.length() - 1);
			for (uint32_t i = 0; i < str.length(); i++) {
				if (str[i] == '1') val += bit;
				bit >>= 1;
			}
			return val;
		}

		std::string Util::FormatBinary(uint32_t x, int width)
		{
			uint32_t i = 1 << (width - 1);
			int w = 0;
			char *str = new char[width + 1];
			str[width] = 0;
			while (w < width) {
				if (x & i) {
					str[w] = '1';
					x -= i;
				} else
					str[w] = '0';

				i >>= 1;
				w++;
			}
			std::string out(str);
			delete[] str;
			return out;
		}

		std::string Util::StrDup(std::string todup, uint32_t n)
		{
			std::stringstream str;
			while (n--) str << todup;
			return str.str();
		}

		std::string Util::Trim_Quotes(std::string r)
		{
			return r.substr(1, r.length() - 2);
		}

		std::string Util::trim_whitespace(std::string &r)
		{
			r.erase(0, r.find_first_not_of(' '));
			r.erase(r.find_last_not_of(' ') + 1);
			return r;
		}

		std::string Util::trim_front(std::string s, std::string trims)
		{
			const char *c = s.c_str();
			while (trims.find(*c) != trims.npos) c++;
			return std::string(c);
		}

		std::string Util::trim_back(std::string s, std::string trims)
		{
			const char *c = s.c_str() + s.length();

			while (trims.find(*c) != trims.npos) c--;

			std::string out;
			out.insert(out.begin(), s.c_str(), c);
			return out;
		}

		std::string Util::trim(std::string str, std::string trims)
		{
			std::string out = trim_front(str, trims);
			out = trim_back(out, trims);
			return out;
		}

		void Util::InlineFile(std::stringstream &stream, std::string filename)
		{
			std::ifstream file(filename.c_str());
			if (file.bad()) {
				fprintf(stderr, "Bad file\n");
				exit(1);
			}
			char buffer;
			while (!file.eof()) {
				file.read(&buffer, 1);
				stream.write(&buffer, 1);
			}
		}

		std::vector<std::string> Util::Tokenize(const std::string source, const std::string breaks, bool match_brackets)
		{
			std::string cpy = source;
			std::vector<std::string> out;
			int brackets = 0;

			std::ostringstream curr;

			std::string::iterator last_break = cpy.begin();

			for (std::string::iterator ch = cpy.begin(); ch != cpy.end(); ++ch) {
				if (match_brackets) {
					if (*ch == '(') {
						brackets++;
						curr << *ch;
						continue;
					}
					if (*ch == ')') {
						brackets--;
						curr << *ch;
						continue;
					}

				}
				if (breaks.find(*ch) != breaks.npos && (!match_brackets || brackets == 0)) {
					out.push_back(std::string(curr.str()));
					curr.str("");
					last_break = ch;
					last_break++;
				} else
					curr << *ch;
			}
			if (cpy.length() > 0) out.push_back(std::string(last_break, cpy.end()));

			return out;
		}

		std::vector<std::string> Util::Tokenize(const std::string source, const std::string breaks, bool include_breaks, bool merge)
		{
			std::string cpy = source;
			std::vector<std::string> out = std::vector<std::string>();
			while (cpy.length() > 0) {

				size_t pos = cpy.find_first_of(breaks);
				if (pos == cpy.npos) break;
				std::string tok = cpy.substr(0, pos);
				char brk = cpy[pos++];

				if (tok.length()) out.push_back(tok);

				if (include_breaks) {
					if (!merge) {
						out.push_back(std::string(1, brk));
					} else {
						int l = 1;
						while (cpy[pos] == brk) pos++, l++;

						out.push_back(std::string(l, brk));
					}
				}
				cpy = cpy.substr(pos, cpy.length());
			}
			out.push_back(cpy);
			return out;
		}

		std::string Util::clean(const std::string str)
		{
			std::vector<std::string> tokens = Util::Tokenize(str, "\"\n", true, false);
			std::stringstream stream;
			for (std::vector<std::string>::iterator i = tokens.begin(); i != tokens.end(); ++i) {
				if (*i == "\\") stream << "\\\\";
				if (*i == "\n")
					stream << "\\n";
				else if (*i == "\"")
					stream << "\\\"";
				else
					stream << *i;
			}
			return stream.str();
		}

		std::string Util::FindReplace(const std::string &input, const std::string &find, const std::string &replace)
		{
			size_t pos;
			std::string in = std::string(input);
			std::ostringstream out;
			while ((pos = in.find(find)) != in.npos) {
				out << in.substr(0, pos) << replace;
				in = in.substr(pos + find.length());
			}
			out << in;
			return out.str();
		}

		size_t Util::Match(const std::string str, size_t begin, char inc, char dec)
		{
			int level = 0;
			size_t pos = 0;
			for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {

				if (begin > 0) {
					begin--;
					pos++;
					continue;
				}
				if (*i == inc)
					level++;
				else if (*i == dec)
					level--;
				if (level == 0) return pos;
				pos++;
			}
			return str.npos;
		}

		std::string Util::TypeString(uint32_t width)
		{
			if (width > 32) return "uint64_t";
			if (width > 16) return "uint32_t";
			if (width > 8) return "uint16_t";
			if (width > 1) return "uint8_t";
			return "bool";
		}

		std::string cppformatstream::str() const
		{
			std::string str = std::ostringstream::str();

			std::stringstream out;

			// Split string into lines when:
			// we hit a linebreak in the original source
			// we hit a { or a }
			// we hit a semicolon

			std::stringstream line;
			std::vector<std::string> lines;

			bool in_string = false;
			int in_parenths = false;
			bool in_line_comment = false;
			for (std::string::iterator i = str.begin(); i != str.end(); ++i) {
				switch (*i) {
					case '(':
						in_parenths++;
						line << *i;
						break;
					case ')':
						in_parenths--;
						line << *i;
						break;
					case '\n':
						lines.push_back(line.str());
						line.str("");
						in_line_comment = false;
						break;
					case ';':
						if (in_string || in_parenths)
							line << *i;
						else {
							line << *i;
							lines.push_back(line.str());
							line.str("");
						}
						break;
					case '{':
					case '}':
						if (in_string || in_line_comment)
							line << *i;
						else {
							lines.push_back(line.str());
							line.str("");
							line << *i;
							lines.push_back(line.str());
							line.str("");
						}
						break;
					case '"':
						in_string = !in_string;
						line << *i;
						break;
					case ':':
						if (*(i+1) == ':') {
							line << ':';
							line << ':';
							i++;
							break;
						} else if((line.str().find("case ") != std::string::npos) || (line.str().find("default:") != std::string::npos)) {
							line << ':';
							lines.push_back(line.str());
							line.str("");
							break;
						} else {
							line << *i;
							break;
						}
					case '/':
						if(*(i+1) == '/') {
							line << "//";
							i++;
							in_line_comment = true;
						} else {
							line << *i;
						}
						break;
					default:
						line << *i;
						break;
				}
			}
			lines.push_back(line.str());

			int indentation = 0;
			for (std::vector<std::string>::iterator i = lines.begin(); i != lines.end(); ++i) {
				std::string s = Util::trim(*i, " \t");

				if (s == "") {
					continue;
				}

				if (s == "}") indentation--;
				if (indentation < 0) indentation = 0;

				if(((s.find("case ") != std::string::npos) || (s.find("default:") != std::string::npos)) && indentation > 0)
					indentation--;

				out << Util::StrDup("  ", indentation) << s << "\n";

				if((s.find("case ") != std::string::npos) || (s.find("default:") != std::string::npos))
					indentation++;

				if (s == "{") indentation++;
			}

			return out.str();
		}
	}
}
