/*
 * Copyright (C)2014-2015 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * For further information please contact.
 *  URI:    http://www.aquaxis.com/
 *  E-Mail: info(at)aquaxis.com
 */
module aq_axi_memcpy32_ctl(
	input			RST_N,
	input			CLK,

	input			CMD_REQ,
	output			CMD_READY,
	output			CMD_DONE,
	input [31:0]	CMD_DST,
	input [31:0]	CMD_SRC,
	input [31:0]	CMD_LEN,

	input			CMD_CLK,

	output			WR_START,
	output [31:0]	WR_ADRS,
	output [31:0]	WR_COUNT,
	input			WR_READY,

	output			RD_START,
	output [31:0]	RD_ADRS,
	output [31:0]	RD_COUNT,
	input			RD_READY,

	output			FIFO_RST
);

localparam S_IDLE		= 0;
localparam S_READ_PRE	= 1;
localparam S_READ		= 2;
localparam S_READ_WAIT	= 3;
localparam S_WRITE_PRE	= 4;
localparam S_WRITE		= 5;
localparam S_WRITE_WAIT	= 6;

reg [31:0]	reg_wr_adrs, reg_rd_adrs;
reg [31:0]	reg_wr_count, reg_rd_count;

reg [2:0] state;

always @(posedge CLK or negedge RST_N) begin
	if(!RST_N) begin
		state 				<= S_IDLE;
		reg_wr_adrs[31:0]	<= 32'd0;
		reg_wr_count[31:0]	<= 32'd0;
		reg_rd_adrs[31:0]	<= 32'd0;
		reg_rd_count[31:0]	<= 32'd0;
	end else begin
		case(state)
			S_IDLE: begin
				if(CMD_REQ) begin
					state	<= S_READ_PRE;
					reg_wr_adrs[31:0]	<= CMD_DST;
					reg_wr_count[31:0]	<= CMD_LEN;
					reg_rd_adrs[31:0]	<= CMD_SRC;
					reg_rd_count[31:0]	<= CMD_LEN;
				end
			end
			S_READ_PRE: begin
				if(WR_READY) begin
					state <= S_READ;
				end
			end
			S_READ: begin
				state <= S_READ_WAIT;
			end
			S_READ_WAIT: begin
				if(WR_READY) begin
					state <= S_WRITE_PRE;
				end
			end
			S_WRITE_PRE: begin
				if(RD_READY) begin
					state <= S_WRITE;
				end
			end
			S_WRITE: begin
				state <= S_WRITE_WAIT;
			end
			S_WRITE_WAIT: begin
				if(RD_READY) begin
					state <= S_IDLE;
				end
			end
		endcase
	end
end

assign WR_START			= (state == S_WRITE)?1'b1:1'b0;
assign WR_ADRS[31:0]	= reg_wr_adrs[31:0];
assign WR_COUNT[31:0]	= reg_wr_count[31:0];
assign RD_START			= (state == S_READ)?1'b1:1'b0;
assign RD_ADRS[31:0]	= reg_rd_adrs[31:0];
assign RD_COUNT[31:0]	= reg_rd_count[31:0];

assign CMD_READY		= (state == S_IDLE)?1'b1:1'b0;
assign CMD_DONE			= (state == S_IDLE)?1'b1:1'b0;

assign FIFO_RST			= (state == S_IDLE)?1'b1:1'b0;

endmodule

