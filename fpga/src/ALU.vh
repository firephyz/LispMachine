`ifndef alu_include
`define alu_include

`define alu_data_width 20
`define alu_opcode_width 3

`define SYS_ADD 		`alu_opcode_width'h0
`define SYS_SUB 		`alu_opcode_width'h1
`define SYS_LESS 		`alu_opcode_width'h2
`define SYS_EQUAL 	`alu_opcode_width'h3
`define SYS_GREATER 	`alu_opcode_width'h4
`define SYS_AND 		`alu_opcode_width'h5
`define SYS_OR 		`alu_opcode_width'h6
`define SYS_NOT 		`alu_opcode_width'h7

`endif