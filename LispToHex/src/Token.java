
public class Token {
	
	public TokenType type;
	public SymbolType symbol_type;
	public String data;
	
	public Token(TokenType type, SymbolType sym_type, String string) {
		data = string;
		this.type = type;
		this.symbol_type = sym_type;
	}
}
