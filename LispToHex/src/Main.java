import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.Scanner;
import java.util.Stack;


public class Main {
	
	public static void main(String[] args) throws FileNotFoundException {
		
		//String string = "((lambda (x) (eq? x (quote a))) (quote a))";
		String string = "(if (eq? (quote a) (quote a)) (cdr (+ 1 1)) (cdr (+ 2 2)))";
		//String string = "((quote true) (quote false))";
		//String string = "(quote tes)";
		//String string = "(if a b c)";
		
		Cell result = parseString(string);
		
		outputTree(result);
		
		outputHexFile();
		
		printDataReadable();
	}
	
	public static void outputHexFile() throws FileNotFoundException {
		
		File file = new File("data.txt");
		File out_file = new File("output.hex");
		Scanner in = new Scanner(file);
		PrintWriter out = new PrintWriter(out_file);
		int count = 0;
		
		while(count < 1024) {
			int num = 0;
			if(in.hasNextInt()) {
				num = in.nextInt();
			}
			
			int byteCount = 3;
			int byte1 = (num >> 16) & 0xFF;
			int byte2 = (num >> 8) & 0xFF;
			int byte3 = num & 0xFF;
			
			int sum = byteCount + (count >> 8) + (count & 0xFF) + byte1 + byte2 + byte3;
			int twosComp = ~sum + 1;
			out.printf(":%02X%04X00%02X%02X%02X%02X\n", byteCount, count, byte1, byte2, byte3, twosComp & 0xFF);
			
			++count;
		}
		
		out.printf(":00000001FF\n");
		
		out.close();
	}
	
	public static void printDataReadable() throws FileNotFoundException {
		
		File file = new File("data.txt");
		Scanner in = new Scanner(file);
		int count = 0;
		
		while(in.hasNextInt()) {
			int num = in.nextInt();
			System.out.printf("%-6d, %d, %d, %d\n", count, num >> 20, (num >> 10) & 0x3FF, num & 0x3FF);
			++count;
		}
	}
	
	public static void outputTree(Cell cell) throws FileNotFoundException {
		
		int cellCount = 0;
		ArrayList<Integer> nums = new ArrayList<>();
		File data = new File("data.txt");
		PrintWriter out = new PrintWriter(data);
		
		Stack<Cell> stack = new Stack<>();
		stack.push(cell);
		
		while(!stack.isEmpty()) {
			
			cell = stack.pop();
			
			int finalResult = formatData(cell);
			
			nums.add(finalResult);
			++cellCount;
			
			if(cell.cdr != null && cell.cdr.cell_index != 0) {
				stack.push(cell.cdr);
			}
			
			if(cell.car != null && cell.car.cell_index != 0) {
				stack.push(cell.car);
			}
		}
		
		out.println(cellCount + 2);
		for(Integer i : nums) {
			out.printf("%d\n", i);
		}
		
		// Write the return cell onto the stack. This will
		// be the only thing on the system stack when the machine starts.
		// Tells the machine that the calling function for SYS_EVAL was
		// the repl so it should stop once it reaches this point.
		// (haven't implemented the repl yet)
		// (actually repl should be software so this calling function just needs to be renamed)
		out.printf("%d\n", 0x503C00);
		
		out.close();
	}
	
	public static int formatData(Cell cell) {
		
		switch(cell.type) {
		case General:
			if(cell.cdr == null) cell.cdr = new Cell(0);
			return (0 << 20) | (cell.car.cell_index << 10) | cell.cdr.cell_index;
		case Opcode:
			return (1 << 20) | cell.data;
		case Symbol:
			if(cell.cdr == null) cell.cdr = new Cell(0);
			return (2 << 20) | ((int)cell.char1 << 10) | cell.cdr.cell_index;
		case Number:
			return (3 << 20) | cell.data;
		case Boolean:
			return (4 << 20) | cell.data;
		}
		
		return -1;
	}
	
	static int cellIndex;
	
	public static Cell parseString(String string) {
		
		cellIndex = 1;
		
		Tokenizer t = new Tokenizer(string);
		Stack<Cell> stack = new Stack<>();
		
		boolean had_symbol = false;
		Cell cell = new Cell(0);
		stack.push(cell);
		
		do {
			Token token = t.next();
			
			switch(token.type) {
				case L_Parens: {
					if(had_symbol || cell.car != null) {
						cell.cdr = new Cell(cellIndex);
						++cellIndex;
						stack.pop();
						cell = cell.cdr;
						stack.push(cell);
					}
					
					had_symbol = false;
					cell.car = new Cell(cellIndex);
					++cellIndex;
					cell = cell.car;
					stack.push(cell);
					break;
				}
				case R_Parens: {
					had_symbol = false;
					
					stack.pop();
					cell = stack.peek();
					break;
				}
				case Atom: {
					if(had_symbol || cell.car != null) {
						cell.cdr = new Cell(cellIndex);
						++cellIndex;
						stack.pop();
						cell = cell.cdr;
						stack.push(cell);
					}
					else {
						had_symbol = true;
					}
					cell.car = makeCorrectCell(token);
					++cellIndex;
					break;
				}
			}
		} while(stack.size() != 1);
		
		return stack.pop().car;
	}
	
	private static Cell makeSymbolCells(String string) {
		
		int index = 0;
		Cell result = null;
		Cell prev = null;
		Cell cell = null;
		
		while(index < string.length()) {
			char char1 = string.charAt(index);
			++index;
			
			if(result == null) {
				result = new Cell(cellIndex, CellType.Symbol, char1, '\0');
				cell = result;
			}
			else {
				prev = cell;
				cell = new Cell(cellIndex, CellType.Symbol, char1, '\0');
				prev.cdr = cell;
			}
			
			++cellIndex;
		}
		
		--cellIndex;
		return result;
	}
	
	private static Cell makeCorrectCell(Token token) {
		switch(token.symbol_type) {
		case Opcode:
			return new Cell(cellIndex, CellType.Opcode, getOpcode(token.data));
		case Symbol:
			return makeSymbolCells(token.data);
		case Number:
			return new Cell(cellIndex, CellType.Number, Integer.parseInt(token.data));
		case Boolean:
			if(token.data.equals("true")) {
				return new Cell(cellIndex, CellType.Boolean, 1);
			}
			else {
				return new Cell(cellIndex, CellType.Boolean, 0);
			}
		}
		
		return null;
	}
	
	public static int getOpcode(String string) {
		
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