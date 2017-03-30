public enum CellType {
	
	General(0),
	Opcode(1),
	Symbol(2),
	Number(3),
	Boolean(4);
	
	private final int value;
	
	private CellType(int value) {
		this.value = value;
	}
	
	public int getValue() {return value;}
}