#ifndef EXPR_PARSER_INCLUDED
	#define EXPR_PARSER_INCLUDED

	void parseExpr(char *string);
	int nextToken(int startIndex);
	int endOfToken(int startingIndex);
	int findMatchingParens(int firstIndex);

#endif