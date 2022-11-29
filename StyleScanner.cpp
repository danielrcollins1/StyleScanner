/*
	Name: StyleScanner
	Copyright: 2021
	Author: Daniel R. Collins
	Date: 04/08/21 00:05
	Description: Scans approved style for student C++ assignment submissions.
		Broadly aligned with Gaddis C++ textbook styling.
		Also expect Dev-C++ style comment header.
*/
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>
using namespace std;

// StyleScanner class
class StyleScanner {
	public:
		void printBanner();
		void printUsage();
		void parseArgs(int argc, char** argv);
		bool getExitAfterArgs();
		bool readFile();
		void writeFile();
		void checkErrors();
		void showTokens();

	protected:

		// Initial file scanning
		void scanCommentLines();
		void scanScopeLevels();

		// Utility functions
		int getLength(const string &line);
		int getSize(const vector<string> &vec);
		int getSize(const vector<int> &vec);

		// Helper functions
		void printError(const string &error);
		void printErrors(const string &error, const vector<int> &lines);
		int getFirstCommentLine();
		int getFirstNonspacePos(const string &line);
		int getLastNonspacePos(const string &line);
		int getStartTabCount(const string &line);
		bool isIndentTabs(int line);
		bool isBlank(int line);
		bool isBlank(const string &line);
		bool isCommentLine(int line);
		bool isCommentBeforeCase(int line);
		bool isPunctuation(char c);
		bool isPunctuationChaser(char c);
		bool isMidBlockComment(int line);
		bool isLineLabel(const string &line);
		bool isLineStartOpenBrace(const string &line);
		bool isLineStartCloseBrace(const string &line);
		bool isLineEndingSemicolon(const string &line);
		bool isOkayIndentLevel(int line);
		bool isSameScope (int startLine, int numLines);
		bool isLeadInCommentHere(int line);
		bool mayBeRunOnLine(int line);

		// Token-based helper functions
		string getNextToken(const string &s, int &pos);
		string getFirstToken(const string &s);
		string getLastToken(const string &s);
		bool isBasicType(const string &s);
		bool isOkConstant(const string &s);
		bool isOkVariable(const string &s);
		bool isOkFunction(const string &s);
		bool isOkStructure(const string &s);
		bool isOkClass(const string &s);
		bool isSpacedOperator(const string &s);
		bool isStartParen(const string &s);
		bool isFunctionHeader (const string &s);
		bool isFunctionHeader (const string &s, string &name);
		bool isClassHeader (const string &s);
		bool stringStartsWith(const string &s, const string &t);
		bool stringEndsWith(const string &s, const string &t);

		// Readability items
		void checkLineLength();
		void checkTabUsage();
		void checkIndentLevels();
		void checkExtraneousBlanks();
		void checkVariableNames();
		void checkConstantNames();
		void checkFunctionNames();
		void checkStructureNames();
		void checkClassNames();
		void checkPunctuationSpacing();
		void checkSpacedOperators();
		void checkFunctionLength();

		// Documentation items
		void checkAnyComments();
		void checkHeaderStart();
		void checkHeaderFormat();
		void checkEndLineComments();
		void checkBlanksBeforeComments();
		void checkTooFewComments();
		void checkTooManyComments();
		void checkEndlineRunonComments();
		void checkStartSpaceComments();
		void checkFunctionLeadComments();

	private:
		string fileName;
		bool exitAfterArgs = false;
		bool doFunctionLengthCheck = true;
		vector<string> fileLines;
		vector<int> commentLines;
		vector<int> scopeLevels;
};

// Character codes to avoid confusing checker on this file
const char COMMA = 44;
const char SEMICOLON = 59;
const char QUESTION_MARK = 63;
const char LEFT_BRACE = 123;
const char RIGHT_BRACE = 125;

// String codes to avoid confusing checker on this file
const char C_COMMENT_START[] = {'/', '*', '\0'};
const char C_COMMENT_END[] = {'*', '/', '\0'};
const char DOUBLE_SLASH[] = {'/', '/', '\0'};
const char START_BLOCK[] = {LEFT_BRACE};

// Print program banner
void StyleScanner::printBanner() {
	cout << "StyleScanner\n";
	cout << "------------\n";
}

// Print program usage
void StyleScanner::printUsage() {
	cout << "Usage: StyleScanner file [options]\n";
	cout << "  where options include:\n";
	cout << "\t-f suppress function length check\n";
	cout << endl;
}

// Parse arguments
void StyleScanner::parseArgs(int argc, char** argv) {
	for (int count = 1; count < argc; count++) {
		if (argv[count][0] == '-') {
			switch (argv[count][1]) {
				case 'f': doFunctionLengthCheck = false; break;
				default: exitAfterArgs = true;
			}
		}
		else if (fileName == "") {
			fileName = argv[count];
		}
		else {
			exitAfterArgs = true;
		}
	}
	if (fileName == "") {
		exitAfterArgs = true;
	}
}

// Get exit after args flag
bool StyleScanner::getExitAfterArgs() {
	return exitAfterArgs;
}

// Combined check-errors function
//   Prioritize these (by what visually bugs me most)
//   since after 4 errors per section
//   we'll cut off the rest for student focus.
void StyleScanner::checkErrors() {

	// Readability items
	cout << "\n# Readability #\n";
	checkFunctionLength();
	checkLineLength();
	checkIndentLevels();
	checkTabUsage();
	checkClassNames();
	checkStructureNames();
	checkFunctionNames();
	checkConstantNames();
	checkVariableNames();
	checkExtraneousBlanks();
	checkPunctuationSpacing();
	checkSpacedOperators();

	// Documentation items
	cout << "\n# Documentation #\n";
	checkAnyComments();
	checkHeaderStart();
	checkHeaderFormat();
	checkEndlineRunonComments();
	checkEndLineComments();
	checkFunctionLeadComments();
	checkBlanksBeforeComments();
	checkTooFewComments();
	checkTooManyComments();
	checkStartSpaceComments();
}

// Read a code file
bool StyleScanner::readFile() {

	// Open the file
	ifstream inFile(fileName);
	if (!inFile) {
		cerr << "Error: File not found.\n";
		return false;
	}

	// Read the file
	string nextLine;
	while (!inFile.eof()) {
		getline(inFile, nextLine);
		fileLines.push_back(nextLine);
	}
	inFile.close();

	// Post-processing
	scanCommentLines();
	scanScopeLevels();
	return true;
}

// Print the read file (for testing)
void StyleScanner::writeFile() {
	for (string line: fileLines) {
		cout << line << endl;
	}
	cout << endl;
}

// Get integer-length of a string
//   (Silences compiler warnings on conversion.)
int StyleScanner::getLength(const string &line) {
	return (int) line.length();	
}

// Get integer-size of a vector of strings
//   (Silences compiler warnings on conversion.)
int StyleScanner::getSize(const vector<string> &vec) {
	return (int) vec.size();	
}

// Get integer-size of a vector of ints
//   (Silences compiler warnings on conversion.)
int StyleScanner::getSize(const vector<int> &vec) {
	return (int) vec.size();	
}

// Get first nonspace position in a string
//   Returns -1 if none such.
int StyleScanner::getFirstNonspacePos(const string &line) {
	for (int i = 0; i < getLength(line); i++) {
		if (!isspace(line[i])) {
			return i;
		}
	}
	return -1;
}

// Get last nonspace position in a string
//   Returns -1 if none such.
int StyleScanner::getLastNonspacePos(const string &line) {
	for (int i = getLength(line) - 1; i >= 0; i--) {
		if (!isspace(line[i])) {
			return i;
		}
	}
	return -1;
}

// Is this line a blank (all whitespace)?
bool StyleScanner::isBlank(const string &line) {
	return getFirstNonspacePos(line) == -1;
}

// Is this line number a blank?
bool StyleScanner::isBlank(int line) {
	return isBlank(fileLines[line]);
}

// Does string s start with string t?
bool StyleScanner::stringStartsWith(const string &s, const string &t) {
	if (getLength(s) >= getLength(t)) {
		for (int i = 0; i < getLength(t); i++) {
			if (s[i] != t[i]) {
				return false;
			}
		}
		return true;
	}
	return false;
}

// Does string s end with string t?
bool StyleScanner::stringEndsWith(const string &s, const string &t) {
	if (getLength(s) >= getLength(t)) {
		int offset = getLength(s) - t.length();
		for (int i = 0; i < getLength(t); i++) {
			if (s[i + offset] != t[i]) {
				return false;
			}
		}
		return true;
	}
	return false;
}

// Find where the comment lines are
//    Assumes comments are full lines (no end-line comments, etc.)
//    Records lines as 0: no comment, 1: C-style, 2: C++-style
void StyleScanner::scanCommentLines() {
	commentLines.resize(getSize(fileLines));
	bool inCstyleComment = false;
	for (int i = 0; i < getSize(fileLines); i++) {
		string firstToken = getFirstToken(fileLines[i]);
		string lastToken = getLastToken(fileLines[i]);

		// Check C-style comment
		if (stringStartsWith(firstToken, C_COMMENT_START)) {
			inCstyleComment = true;
		}
		if (inCstyleComment) {
			commentLines[i] = 1;
		}
		if (stringEndsWith(lastToken, C_COMMENT_END)) {
			inCstyleComment = false;
		}

		// Check C++-style comment
		if (stringStartsWith(firstToken, DOUBLE_SLASH)) {
			commentLines[i] = 2;
		}
	}
}

// Is the line a (full-line) comment?
bool StyleScanner::isCommentLine(int line) {
	assert(0 <= line && line < getSize(fileLines));
	return (bool) commentLines[line];
}

// Is the line a comment before a case or default label?
bool StyleScanner::isCommentBeforeCase (int line) {
	assert(0 <= line && line < getSize(fileLines));
	if (isCommentLine(line)) {
		for (int cLine = line + 1; cLine < getSize(fileLines); cLine++) {
			if (isCommentLine(cLine)) continue;
			string firstToken = getFirstToken(fileLines[cLine]);
			if (firstToken == "case" || firstToken == "default")
				return true;
			else
				return false;
		}
	}
	return false;
}

// Does this line start with an opening brace?
bool StyleScanner::isLineStartOpenBrace(const string &line) {
	int firstPos = getFirstNonspacePos(line);
	return firstPos >= 0 && line[firstPos] == LEFT_BRACE;
}

// Does this line start with a closing brace?
bool StyleScanner::isLineStartCloseBrace(const string &line) {
	int firstPos = getFirstNonspacePos(line);
	return firstPos >= 0 && line[firstPos] == RIGHT_BRACE;
}

// Does this line start with a label of interest?
bool StyleScanner::isLineLabel(const string &line) {
	const string LABELS[] = {"case", "default",
		"public", "private", "protected"};
	string firstToken = getFirstToken(line);
	for (string label: LABELS) {
		if (firstToken == label) {
			return true;
		}
	}
	return false;
}

// Does this line end with a semicolon?
bool StyleScanner::isLineEndingSemicolon(const string &line) {
	int lastPos = getLastNonspacePos(line);
	return lastPos != -1 && line[lastPos] == SEMICOLON;
}

// Scan for scope level at each line
//   Increments at each brace
//   Also counts labels as incremented scope
void StyleScanner::scanScopeLevels() {
	scopeLevels.resize(getSize(fileLines));
	int scopeLevel = 0;
	int labelLevel = -1;
	bool inLabel = false;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (commentLines[i]) {
			scopeLevels[i] = scopeLevel;
		}
		else {
			string line = fileLines[i];
			bool lineLabel = isLineLabel(line);

			// Decrement for closing brace
			if (isLineStartCloseBrace(line)) {
				scopeLevel--;
				if (inLabel && scopeLevel == labelLevel) {
					scopeLevel--;
					inLabel = false;
				}
			}

			// Decrement for label-in-label
			if (inLabel && lineLabel) {
				scopeLevel--;
			}

			// Set that level
			scopeLevels[i] = scopeLevel;

			// Increment for a label
			if (lineLabel) {
				inLabel = true;
				labelLevel = scopeLevel;
				scopeLevel++;
			}

			// Scan rest of line for more braces to adjust
			int firstPos = getFirstNonspacePos(line);
			if (firstPos >= 0) {
				for (int i = firstPos; i < getLength(line); i++) {
					switch (line[i]) {
						case LEFT_BRACE: scopeLevel++; break;
						case RIGHT_BRACE: if (i > firstPos) {
								scopeLevel--; break;
							}
					}
				}
			}
		}
	}
}

// Print basic error
void StyleScanner::printError(const string &error) {
	cout << error << "\n";
}

// Format an error report with line numbers
//   If lines vector is empty, then nothing is printed.
//   Line numbers are incremented for user display.
void StyleScanner::printErrors(const string &error, const vector<int> &lines) {

	// Singular error
	if (lines.size() == 1) {
		cout << error << " (line " << lines[0] + 1 << ").\n";
	}

	// Multiple errors
	else if (lines.size() > 1) {
		const int MAX_SHOWN = 3;
		cout << error << " (lines " << lines[0] + 1;
		for (int i = 1; i < getSize(lines) && i < MAX_SHOWN; i++) {
			cout << ", " << lines[i] + 1;
		}
		if (lines.size() > MAX_SHOWN) {
			cout << ", etc";
		}
		cout << ").\n";
	}
}

// Get index of first comment line
//   Returns -1 if none whatsoever
int StyleScanner::getFirstCommentLine() {
	for (int i = 0; i < getSize(fileLines); i++) {
		if (commentLines[i]) {
			return i;
		}
	}
	return -1;
}

// Check if the file has any comment lines at all
void StyleScanner::checkAnyComments() {
	if (getFirstCommentLine() == -1) {
		printError("File lacks any comment lines!");
	}
}

// Check header start
//   File should start with a comment
void StyleScanner::checkHeaderStart() {
	if (getFirstCommentLine() != 0) {
		printError("Misplaced file comment header (line 1).");
	}
}

// Check file header
void StyleScanner::checkHeaderFormat() {
	vector<int> errorLines;
	const string HEADER[] = {C_COMMENT_START, "\tName:", "\tCopyright:",
		"\tAuthor:", "\tDate:", "\tDescription:"};
	int currLine = getFirstCommentLine();
	if (currLine >= 0) {
		for (string headPrefix: HEADER) {
			if (currLine >= getSize(fileLines)
				|| !stringStartsWith(fileLines[currLine], headPrefix))
			{
				errorLines.push_back(currLine);
			}
			currLine++;
		}
	}
	printErrors("Invalid comment header", errorLines);
}

// Check line lengths
void StyleScanner::checkLineLength() {
	const int MAX_LENGTH = 80;
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (fileLines[i].length() > MAX_LENGTH) {
			errorLines.push_back(i);
		}
	}
	printErrors("Line is too long", errorLines);
}

// How many tabs are at the start of this line?
int StyleScanner::getStartTabCount(const string &line) {
	int count = 0;
	while (count < getLength(line) && line[count] == '\t')
		count++;
	return count;
}

// Is the indent in this line using tabs?
bool StyleScanner::isIndentTabs(int line) {
	string lineStr = fileLines[line];	
	int checkToPos = getFirstNonspacePos(lineStr);

	// Artistic Style uses spaces for continuation lines;
	// so in these cases, check no further than scope level
	if (mayBeRunOnLine(line)) {
		checkToPos = min(checkToPos, scopeLevels[line]);
	}
	
	// Check for all-tabs here
	for (int i = 0; i < checkToPos; i++) {
		if (lineStr[i] != '\t') {
			return false;
		}
	}
	return true;
}

// Check end-line comments
void StyleScanner::checkEndLineComments() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!commentLines[i]
			&& (fileLines[i].find(DOUBLE_SLASH) != string::npos
			|| fileLines[i].find(C_COMMENT_START) != string::npos))
		{
			errorLines.push_back(i);
		}
	}
	printErrors("End-line comments shouldn't be used", errorLines);
}

// Check tab usage for indents
void StyleScanner::checkTabUsage() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!isIndentTabs(i)) {
			errorLines.push_back(i);
		}
	}
	printErrors("Tabs should be used for indents", errorLines);
}

// Is this line in the middle of a C-style block comment?
bool StyleScanner::isMidBlockComment(int line){
	return commentLines[line] == 1
		&& (line > 0 && commentLines[line - 1] == 1)
		&& (line < getSize(commentLines) - 1 && commentLines[line + 1] == 1);
}

// Is this line possibly a run-on (continuation) statement?
bool StyleScanner::mayBeRunOnLine(int line) {
	if (line > 0 && scopeLevels[line] == scopeLevels[line - 1]
		&& !commentLines[line - 1] && !isBlank(line - 1))
	{
		string priorLine = fileLines[line - 1];
		int lastPriorChar = getLastNonspacePos(priorLine);
		if (priorLine[lastPriorChar] != SEMICOLON) {
			return true;
		}
	}
	return false;
}

// Is this line at an acceptable indent level?
bool StyleScanner::isOkayIndentLevel(int line) {

	// Ignore some cases
	if (isBlank(line) || isMidBlockComment(line)
		|| !isIndentTabs(line))
	{
		return true;
	}

	// Get scope & tab levels
	int scopeLevel = scopeLevels[line];
	int numStartTabs = getStartTabCount(fileLines[line]);

	// Comments before a case permit one less indent
	//   (This is sketchy before the first case)
	if (isCommentBeforeCase(line)) {
		return numStartTabs	== scopeLevel
			|| numStartTabs == scopeLevel - 1;
	}

	// Run-on (continuation) lines handling:
	//   Artistic Style uses spaces for extra indents
	//   So require at least scope level tabs
	if (mayBeRunOnLine(line)) {
		return numStartTabs >= scopeLevel;
	}

	// Standard indent case
	return numStartTabs == scopeLevel;
}

// Check indent levels
void StyleScanner::checkIndentLevels() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!isOkayIndentLevel(i)) {
			errorLines.push_back(i);
		}
	}
	printErrors("Indent level errors", errorLines);
}

// Check blanks before comments (required)
void StyleScanner::checkBlanksBeforeComments() {
	vector<int> errorLines;
	for (int i = 1; i < getSize(fileLines); i++) {
		if (commentLines[i]) {
			string priorLine = fileLines[i - 1];
			int priorEndIndent = getFirstNonspacePos(priorLine);
			if (!isBlank(i - 1)
				&& !commentLines[i - 1]
				&& !(priorLine[priorEndIndent] == LEFT_BRACE))
			{
				errorLines.push_back(i);
			}
		}
	}
	printErrors("Missing blank line before comment", errorLines);
}

// Check for no comments in long stretch of statements
void StyleScanner::checkTooFewComments() {
	const int LONG_STRETCH = 25;
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (commentLines[i]) {
			int end = i + 1;
			while (end < getSize(fileLines) && !commentLines[end++]);
			if (end - i > LONG_STRETCH) {
				errorLines.push_back(i + LONG_STRETCH / 2);
			}
		}
	}
	printErrors("Too few comments", errorLines);
}

// Check that next N lines all in same scope
bool StyleScanner::isSameScope (int startLine, int numLines) {
	if (startLine >= 0
		&& startLine + numLines < getSize(fileLines))
	{
		int startLevel = scopeLevels[startLine];
		for (int i = 1; i <= numLines; i++) {
			if (scopeLevels[startLine + i] != startLevel) {
				return false;
			}
		}
		return true;
	}
	return false;
}

// Check for commenting multiple single-line statements
void StyleScanner::checkTooManyComments() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines) - 5; i++) {
		if (commentLines[i]
			&& !commentLines[i + 1]
			&& isBlank(i + 2)
			&& commentLines[i + 3]
			&& !commentLines[i + 4]
			&& isBlank(i + 5)
			&& isSameScope(i, 5))
		{
			errorLines.push_back(i + 3);
		}
	}
	printErrors("Too many comments", errorLines);
}

// Is this character an operator that expects spacing?
//    Note we must skip many symbols used for other purposes, e.g.:
//   '<', '>' used for brackets in #includes, templates
//   "++", "--" pre/postfix to variable
//   "-" used for unary negation (start number)
//   "*" used for pointer operator
//   "/" used in units (e.g., ft/sec)
bool StyleScanner::isSpacedOperator(const string &s) {
	const string SPACE_OPS[] = {"+", "%", "<<", ">>", "<=", ">=",
		"==", "!=", "&&", "||", "=", "+=", "-=", "*=", "/="};
	for (string op: SPACE_OPS) {
		if (s == op) {
			return true;
		}
	}
	return false;
}

// Check spaces around operators
void StyleScanner::checkSpacedOperators() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!isCommentLine(i)) {
			int pos = 0;
			string line = fileLines[i];
			string token = getNextToken(line, pos);
			while (token != "") {
				if (isSpacedOperator(token)) {

					// Check for space before & after
					int startPos = pos - token.length();
					if ((startPos > 0 && !isspace(line[startPos - 1]))
						|| (pos < getLength(line) && !isspace(line[pos])))
					{
						errorLines.push_back(i);
						break;
					}
				}
				token = getNextToken(line, pos);
			}
		}
	}
	printErrors("Operators should have surrounding spaces", errorLines);
}

// Check for end-line C-style comments that continue to next line
//   Never seen this, but it would foil all our other comment logic.
void StyleScanner::checkEndlineRunonComments() {
	vector<int> errorLines;
	for (int i = 1; i < getSize(fileLines); i++) {
		if (!isCommentLine(i)
			&& fileLines[i].find(C_COMMENT_START) != string::npos
			&& fileLines[i].find(C_COMMENT_END) == string::npos)
		{
			errorLines.push_back(i);
		}
	}
	printErrors("End-line run-on comments used!", errorLines);
}

// Is this a punctuation character?
//   Can't do colons, b/c of time, scope-resolution operator.
//   Commas in big numbers problematic (but retain check for now).
bool StyleScanner::isPunctuation(char c) {
	const char PUNCT[] = {COMMA, SEMICOLON, QUESTION_MARK};
	for (char punct: PUNCT) {
		if (c == punct) {
			return true;
		}
	}
	return false;
}

// Is this an acceptable post-punctuation character?
//   Quotes or escapes may follow in a string literal.
bool StyleScanner::isPunctuationChaser(char c) {
	const char CHASERS[] = {' ', '\n', '\t', '\"', '\\'};
	for (char chaser: CHASERS) {
		if (c == chaser) {
			return true;
		}
	}
	return false;
}

// Check for spaces after punctuation, but not before
void StyleScanner::checkPunctuationSpacing() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		string line = fileLines[i];
		for (int j = 0; j < getLength(line); j++) {
			if (isPunctuation(line[j])) {
				if (((j > 1 && isspace(line[j - 1]))
					|| (j < getLength(line) - 1
					&& !isPunctuationChaser(line[j + 1]))))
				{
					errorLines.push_back(i);
					break;
				}
			}
		}
	}
	printErrors("Punctuation should have space afterward", errorLines);
}

// Get next token from a line
//   Splits on spaces, identifiers, numbers, and punctuation.
//   Simplistic: Gets blocks of punctuation, glues grouping symbols, etc.
//   Starts at pos; updates pos to after found token.
string StyleScanner::getNextToken(const string &s, int &pos) {

	// Eat spaces
	while (pos < getLength(s) && isspace(s[pos])) {
		pos++;
	}

	// Get nothing
	int start = pos;
	if (pos >= getLength(s)) {
		return "";
	}

	// Get a word
	if (isalpha(s[pos]) || s[pos] == '_') {
		while (pos < getLength(s) && (isalnum(s[pos]) || s[pos] == '_')) {
			pos++;
		}
	}

	// Get a number
	else if (isdigit(s[pos])) {
		while (pos < getLength(s) && (isdigit(s[pos]) || s[pos] == '.')) {
			pos++;
		}
	}

	// Get punctuation
	else {
		while (pos < getLength(s) && ispunct(s[pos])) {
			pos++;
		}
	}

	// Return the found token
	return s.substr(start, pos - start);
}

// Get first token on a line
string StyleScanner::getFirstToken(const string &s) {
	int pos = 0;
	return getNextToken(s, pos);
}

// Get last token on a line
string StyleScanner::getLastToken(const string &s) {
	int pos = 0;
	string lastToken = "";
	string nextToken = getNextToken(s, pos);
	while (nextToken != "") {
		lastToken = nextToken;
		nextToken = getNextToken(s, pos);
	}
	return lastToken;
}

// Show all tokens in file (for testing)
void StyleScanner::showTokens() {
	for (string line: fileLines) {
		int pos = 0;
		string token = getNextToken(line, pos);
		while (token != "") {
			cout << token << endl;
			token = getNextToken(line, pos);
		}
		cout << endl;
	}
	cout << endl;
}

// Is this string a fundamental type?
bool StyleScanner::isBasicType(const string &s) {
	const string TYPES[] = {"int", "float", "double",
		"char", "bool", "string", "void"};
	for (string type: TYPES) {
		if (s == type) {
			return true;
		}
	}
	return false;
}

// Is this string an acceptable constant name?
bool StyleScanner::isOkConstant(const string &s) {
	if (getLength(s) < 2) {
		return false;
	}
	for (char c: s) {
		if (!isupper(c) && c != '_') {
			return false;
		}
	}
	return true;
}

// Check constant names
void StyleScanner::checkConstantNames() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!isCommentLine(i)) {
			int pos = 0;
			string line = fileLines[i];
			string prefix = getNextToken(line, pos);
			if (prefix == "const") {
				string type = getNextToken(line, pos);
				if (isBasicType(type)) {
					string name = getNextToken(line, pos);
					if (!isOkConstant(name)) {
						errorLines.push_back(i);
					}
				}
			}
		}
	}
	printErrors("Constants should be all-caps name", errorLines);
}

// Is this string an acceptable variable name?
bool StyleScanner::isOkVariable(const string &s) {
	if (getLength(s) < 2
		|| !islower(s[0]))
	{
		return false;
	}
	for (int i = 1; i < getLength(s); i++) {
		if (!isalnum(s[i])
			|| (isupper(s[i]) && isupper(s[i - 1])))
		{
			return false;
		}
	}
	return true;
}

// Does this token start with an open parenthesis?
bool StyleScanner::isStartParen(const string &s) {
	return getLength(s) > 0 && s[0] == '(';
}

// Check variable names
//   Note we check only first variable declared on a line.
void StyleScanner::checkVariableNames() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!isCommentLine(i)) {
			int pos = 0;
			string line = fileLines[i];
			string type = getNextToken(line, pos);
			if (isBasicType(type)) {

				// Get the variable name
				string name = getNextToken(line, pos);
				while (name == "*") {
					name = getNextToken(line, pos);
				}

				// Check only non-function names
				string nextSymbol = getNextToken(line, pos);
				if (!isStartParen(nextSymbol) 
						&& nextSymbol != "::"
						&& nextSymbol != "<") 
				{
					if (!isOkVariable(name)) {
						errorLines.push_back(i);
					}
				}
			}
		}
	}
	printErrors("Variables should be camel-case name", errorLines);
}

// Is this string an acceptable function name?
//   Currently same rule as for variable names.
bool StyleScanner::isOkFunction(const string &s) {
	return isOkVariable(s);
}

// Check function names
void StyleScanner::checkFunctionNames() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!isCommentLine(i)) {
			string name;
			if (isFunctionHeader(fileLines[i], name)) {
				if (!isOkFunction(name)) {
					errorLines.push_back(i);
				}
			}
		}
	}
	printErrors("Functions should be camel-case name", errorLines);
}

// Is there a lead-in comment to the function here?
bool StyleScanner::isLeadInCommentHere(int line) {
	if (line < 2)
		return false;
	return (isCommentLine(line - 1)
		|| (isBlank(line - 1) && isCommentLine(line - 2)));
}

// Check for lead-in comments before functions
void StyleScanner::checkFunctionLeadComments() {
	if (doFunctionLengthCheck) {
		vector<int> errorLines;
		for (int i = 0; i < getSize(fileLines); i++) {
			if (!isCommentLine(i)
				&& scopeLevels[i] == 0
				&& isFunctionHeader(fileLines[i])
				&& !isLeadInCommentHere(i)) 
			{
				errorLines.push_back(i);
			}
		}
		printErrors("Functions should have a lead-in comment", errorLines);
	}
}

// Is the given line a function header?
bool StyleScanner::isFunctionHeader (const string &s) {
	string dummyName;
	return isFunctionHeader(s, dummyName);
}

// Is the given line a function header?
//   If so, return function name in parameter.
bool StyleScanner::isFunctionHeader (const string &s, string &name) {
	int pos = 0;
	string type = getNextToken(s, pos);
	if (isBasicType(type) && !isLineEndingSemicolon(s))
	{
		name = getNextToken(s, pos);
		while (name == "*") {
			name = getNextToken(s, pos);
		}
		string nextSymbol = getNextToken(s, pos);
		if (nextSymbol == "::") {
			name = getNextToken(s, pos);
			nextSymbol = getNextToken(s, pos);
		}
		if (isStartParen(nextSymbol)) {
			return true;
		}
	}
	return false;
}

// Is this string an acceptable structure name?
bool StyleScanner::isOkStructure(const string &s) {
	if (getLength(s) < 2
		|| !isupper(s[0]))
	{
		return false;
	}
	for (int i = 1; i < getLength(s); i++) {
		if (!isalnum(s[i])
			|| (isupper(s[i]) && isupper(s[i - 1])))
		{
			return false;
		}
	}
	return true;
}

// Check structure names
void StyleScanner::checkStructureNames() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!isCommentLine(i)) {
			int pos = 0;
			string line = fileLines[i];
			string prefix = getNextToken(line, pos);
			if (prefix == "struct") {
				string name = getNextToken(line, pos);
				if (!isOkStructure(name)) {
					errorLines.push_back(i);
				}
			}
		}
	}
	printErrors("Structures should start caps camel-case", errorLines);
}

// Is this string an acceptable class name?
//   Currently same rule as for variable names.
bool StyleScanner::isOkClass(const string &s) {
	return isOkStructure(s);
}

// Check structure names
void StyleScanner::checkClassNames() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		if (!isCommentLine(i)) {
			int pos = 0;
			string line = fileLines[i];
			string prefix = getNextToken(line, pos);
			if (prefix == "class") {
				string name = getNextToken(line, pos);
				if (!isOkClass(name)) {
					errorLines.push_back(i);
				}
			}
		}
	}
	printErrors("Classes should start caps camel-case", errorLines);
}

// Is this a class header line?
bool StyleScanner::isClassHeader (const string &s) {
	return getFirstToken(s) == "class";
}

// Check extraneous blank lines
//    Blank lines should only occur:
//    before comment, label, function, or class header
void StyleScanner::checkExtraneousBlanks() {
	vector<int> errorLines;
	for (int i = 0; i < (int) getSize(fileLines) - 2; i++) {
		if (isBlank(i)) {
			int next = i + 1;
			string nextLine = fileLines[next];
			if (!commentLines[next]
				&& !isLineLabel(nextLine)
				&& !isFunctionHeader(nextLine)
				&& !isClassHeader(nextLine))
			{
				errorLines.push_back(i);
			}
		}
	}
	printErrors("Extraneous blank lines", errorLines);
}

// C++-style comments should have a space after slashes.
void StyleScanner::checkStartSpaceComments() {
	vector<int> errorLines;
	for (int i = 0; i < getSize(fileLines); i++) {
		int pos = 0;
		string line = fileLines[i];
		string token = getNextToken(line, pos);
		if (token == DOUBLE_SLASH
			&& pos < getLength(line)
			&& !isspace(line[pos]))
		{
			errorLines.push_back(i);
		}
	}
	printErrors("Comments need space after slashes", errorLines);
}

// Check for overly long functions.
void StyleScanner::checkFunctionLength() {
	if (doFunctionLengthCheck) {
		const int LONG_FUNC = 25;
		vector<int> errorLines;
		for (int i = 0; i < getSize(fileLines); i++) {
			if (isFunctionHeader(fileLines[i])) {
				int startScope = scopeLevels[i];
				int startScan = i++;

				// Search for end of function
				while (i < getSize(fileLines) &&
					(isLineStartOpenBrace(fileLines[i])
					|| scopeLevels[i] > startScope))
				{
					i++;
				}
				int funcLength = i - startScan;
				if (funcLength > LONG_FUNC) {
					errorLines.push_back(startScan);
				}
			}
		}
		printErrors("Function is too long!", errorLines);
	}
}

// Main test driver
int main(int argc, char** argv) {
	StyleScanner checker;
	checker.printBanner();
	checker.parseArgs(argc, argv);
	if (checker.getExitAfterArgs()) {
		checker.printUsage();
	}
	else {
		if (checker.readFile()) {
			checker.checkErrors();
		}
	}
	return 0;
}
