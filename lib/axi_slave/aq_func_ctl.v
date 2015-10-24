/*
 * Copyright (C)2005-2015 AQUAXIS TECHNOLOGY.
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
module aq_func_ctl(
	input			RST_N,
	input			CLK,

	input			LOCAL_CS,
	input			LOCAL_RNW,
	output			LOCAL_ACK,
	input [31:0]	LOCAL_ADDR,
	input [3:0]		LOCAL_BE,
	input [31:0]	LOCAL_WDATA,
	output [31:0]	LOCAL_RDATA,

	output			FUNC_START,
	input			FUNC_READY,
	input			FUNC_DONE
);

localparam A_FUNC_START		= 8'h00;
localparam A_FUNC_STATUS	= 8'h04;
localparam A_FUNC_ARGS_00	= 8'h10;

wire		wr_ena, rd_ena, wr_ack;
reg			rd_ack;

reg             reg_func_start, reg_func_start_d;
reg [31:0]	reg_func_args_00;

reg [31:0]	reg_rdata;

assign wr_ena = (LOCAL_CS & ~LOCAL_RNW)?1'b1:1'b0;
assign rd_ena = (LOCAL_CS &  LOCAL_RNW)?1'b1:1'b0;
assign wr_ack = wr_ena;

// Write Register
always @(posedge CLK or negedge RST_N) begin
	if(!RST_N) begin
		reg_func_start		<= 1'b0;
		reg_func_start_d	<= 1'b0;
		reg_func_args_00	<= 32'd0;
	end else begin
		if(wr_ena & ((LOCAL_ADDR[7:0] & 8'hFC) == A_FUNC_START)) begin
			reg_func_start	<= 1'b1;
		end else begin
			reg_func_start	<= 1'b0;
		end
		reg_func_start_d <= reg_func_start;

		if(wr_ena) begin
			case(LOCAL_ADDR[7:0] & 8'hFC)
				A_FUNC_ARGS_00: begin
					reg_func_args_00[31:0] <= LOCAL_WDATA[31:0];
				end
				default: begin
				end
			endcase
		end
	end
end

// Read Register
always @(posedge CLK or negedge RST_N) begin
	if(!RST_N) begin
		reg_rdata[31:0] <= 32'd0;
		rd_ack <= 1'b0;
	end else begin
		rd_ack <= rd_ena;
		if(rd_ena) begin
			case(LOCAL_ADDR[7:0] & 8'hFC)
				A_FUNC_START: begin
					reg_rdata[31:0] <= 32'd0;
				end
				A_FUNC_STATUS: begin
					reg_rdata[31:0] <= {30'd0, FUNC_READY, FUNC_DONE};
				end
				A_FUNC_ARGS_00: begin
					reg_rdata[31:0] <= reg_func_args_00[31:0];
				end
				default: begin
					reg_rdata[31:0] <= 32'd0;
				end
			endcase
		end else begin
			reg_rdata[31:0] <= 32'd0;
		end
	end
end

assign LOCAL_ACK			= (wr_ack | rd_ack);
assign LOCAL_RDATA[31:0]	= reg_rdata[31:0];

assign FUNC_START	= (reg_func_start & ~reg_func_start_d)?1'b1:1'b0;
assign FUNC_ARGS_00	= reg_func_args_00;

endmodule
