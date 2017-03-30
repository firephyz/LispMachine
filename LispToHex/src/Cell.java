
public class Cell {
	
	int cell_index;
	
	public CellType type;
	public Cell car, cdr;
	public int data;
	public String string_data;
	
	public char char1, char2;
	
	public Cell(int index) {
		this(index, CellType.General, null, null);
	}
	public Cell(int index, Cell car, Cell cdr) {
		this(index, CellType.General, car, cdr);
	}
	public Cell(int index, CellType type, Cell car, Cell cdr) {
		this(index, type, car, cdr, 0, null, ' ', ' ');
	}
	public Cell(int index, CellType type, Cell car, Cell cdr, int data, String string_data, char char1, char char2) {
		this.cell_index = index;
		this.car = car;
		this.cdr = cdr;
		this.type = type;
		this.data = data;
		this.string_data = string_data;
		this.char1 = char1;
		this.char2 = char2;
	}
	public Cell(int index, CellType type, int data) {
		this(index, type, null, null, data, null, ' ', ' ');
	}
	public Cell(int index, CellType type, String data) {
		this(index, type, null, null, 0, data, ' ', ' ');
	}
	public Cell(int index, CellType type, char data0, char data1) {
		this(index, type, null, null, 0, null, data0, data1);
	}
}