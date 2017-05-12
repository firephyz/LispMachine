import java.util.ArrayList;


public class Tokenizer {

	String string;
	int index;
	
	public Tokenizer(String string) {
		this.string = string;
		index = 0;
	}
	
	public Token next() {
		
		if(string.charAt(index) == '(') {
			index++;
			return new Token(TokenType.L_Parens, null, "(");
		}
		else if (string.charAt(index) == ')') {
			index++;
			return new Token(TokenType.R_Parens, null, ")");
		}
		else if (string.charAt(index) == ' ') {
			index++;
			return next();
		}
		else {
			int index_of_delim = -1;
			for(int i = index; i < string.length(); ++i) {
				char character = string.charAt(i);
				if(character == ' ' || character == ')' || character == '(') {
					index_of_delim = i;
					break;
				}
			}
			
			String sub = string.substring(index, index_of_delim);
			index = index_of_delim;
			return new Token(TokenType.Atom, determineType(sub), sub);
		}
	}
	
	private SymbolType determineType(String string) {
		
		int opcode = getOpcode(string);
		
		if(opcode >= 0) {
			return SymbolType.Opcode;
		}
		else if (string.equals("true") || string.equals("false")) {
			return SymbolType.Boolean;
		}
		else if (isNumber(string)) {
			return SymbolType.Number;
		}
		else {
			return SymbolType.Symbol;
		}
	}
	
	private boolean isNumber(String string) {
		
		try{
			int test = Integer.parseInt(string);
		}
		catch(NumberFormatException e) {
			return false;
		}
		
		return true;
	}
	
	private int getOpcode(String string) {
		
		if(string.equals("+")) {
			return 0;
		}
		else if(string.equals("-")) {
			return 1;
		}
		else if(string.equals("<")) {
			return 2;
		}
		else if(string.equals("=")) {
			return 3;
		}
		else if(string.equals(">")) {
			return 4;
		}
		else if(string.equals("and")) {
			return 5;
		}
		else if(string.equals("or")) {
			return 6;
		}
		else if(string.equals("not")) {
			return 7;
		}
		else if(string.equals("car")) {
			return 8;
		}
		else if(string.equals("cdr")) {
			return 9;
		}
		else if(string.equals("cons")) {
			return 10;
		}
		else if(string.equals("eq?")) {
			return 11;
		}
		else if(string.equals("atom?")) {
			return 12;
		}
		else if(string.equals("if")) {
			return 13;
		}
		else if(string.equals("lambda")) {
			return 14;
		}
		else if(string.equals("quote")) {
			return 15;
		}
		else if(string.equals("define")) {
			return 16;
		}
		else if(string.equals("begin")) {
			return 17;
		}
		else return -1;
	}
}
