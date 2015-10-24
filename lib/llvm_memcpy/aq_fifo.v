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
`timescale 1ps / 1ps
module aq_fifo
#(
	parameter FIFO_DEPTH	= 10,
	parameter FIFO_WIDTH	= 64
)
(
	input							RST_N,

	input							FIFO_WR_CLK,
	input							FIFO_WR_ENA,
	input [FIFO_WIDTH -1:0]		FIFO_WR_DATA,
	input							FIFO_WR_LAST,
	output 						FIFO_WR_FULL,
	output 						FIFO_WR_ALM_FULL,
	input [FIFO_DEPTH -1:0]		FIFO_WR_ALM_COUNT,

	input 							FIFO_RD_CLK,
	input 							FIFO_RD_ENA,
	output [FIFO_WIDTH -1:0]	FIFO_RD_DATA,
	output 						FIFO_RD_EMPTY,
	output 						FIFO_RD_ALM_EMPTY,
	input [FIFO_DEPTH -1:0]		FIFO_RD_ALM_COUNT
);

	wire 					wr_ena, wr_full;
	reg 					wr_ena_req_pre, wr_ena_req;
	reg						wr_rd_ena;
	reg [2:0]				wr_rd_ena_d;
	wire 					wr_rd_ena_ack;
	reg						wr_rd_empty;
	reg [FIFO_DEPTH -1:0]	wr_adrs;
	reg [FIFO_DEPTH :0]		wr_count, wr_alm_count, wr_count_req_pre, wr_count_req, wr_rd_count;

	wire					rd_ena, rd_empty;
	reg						rd_ena_d;
	reg 					rd_ena_req_pre, rd_ena_req;
	reg 					rd_wr_ena;
	reg [2:0]				rd_wr_ena_d;
	wire					rd_wr_ena_ack;
	reg						rd_wr_full;
	reg [FIFO_DEPTH -1:0]	rd_adrs;
	reg [FIFO_DEPTH :0]		rd_count, rd_alm_count, rd_count_req_pre, rd_count_req, rd_wr_count;

	wire						rsv_ena;
	reg							rsv_empty;
	reg [FIFO_WIDTH -1:0]	rsv_data;

	wire [FIFO_WIDTH -1:0]	rd_fifo;

	/////////////////////////////////////////////////////////////////////
	// Write Block
	assign wr_full	= wr_count[FIFO_DEPTH];
	assign wr_ena	= (!wr_full)?(FIFO_WR_ENA):1'b0;

	// Write Address
	always @(posedge FIFO_WR_CLK or negedge RST_N) begin
		if(!RST_N) begin
			wr_adrs				<= 0;
		end else begin
			if(wr_ena) wr_adrs	<= wr_adrs + 1;
		end
	end

	// make a full and almost full signal
	always @(posedge FIFO_WR_CLK or negedge RST_N) begin
		if(!RST_N) begin
			wr_count		<= 0;
			wr_alm_count	<= 0;
		end else begin
			if(wr_ena) begin
				if(wr_rd_ena) begin
					wr_count	<= wr_count - {1'b0, wr_rd_count} + 1;
				end else begin
					wr_count	<= wr_count + 1;
				end
			end else if(wr_rd_ena) begin
				wr_count	<= wr_count - {1'b0, wr_rd_count};
			end
			wr_alm_count	<= wr_count + {1'b0, FIFO_WR_ALM_COUNT};
		end
	end

	// Read Control signal from Read Block
	always @(posedge FIFO_WR_CLK or negedge RST_N) begin
		if(!RST_N) begin
			wr_rd_count		<= {FIFO_DEPTH{1'b0}};
			wr_rd_ena_d[2:0]	<= 3'd0;
			wr_rd_empty			<= 1'b0;
			wr_rd_ena			<= 1'b0;
		end else begin
			wr_rd_ena_d[2:0]	<= {wr_rd_ena_d[1:0], rd_ena_req};
			if(wr_rd_ena_d[2:1] == 2'b01) begin
				wr_rd_ena	<= 1'b1;
				wr_rd_count	<= rd_count_req;
				wr_rd_empty	<= rd_empty;
			end else begin
				wr_rd_ena	<= 1'b0;
			end
		end
	end
	assign wr_rd_ena_ack	= wr_rd_ena_d[2] & wr_rd_ena_d[1];

	// Send a write enable signal for Read Block
	reg [2:0] wr_rd_ack_d;
	always @(posedge FIFO_WR_CLK or negedge RST_N) begin
		if(!RST_N) begin
			wr_ena_req_pre		<= 1'b0;
			wr_ena_req		<= 1'b0;
			wr_count_req_pre		<= 0;
			wr_count_req		<= 0;
			wr_rd_ack_d		<= 3'd0;
		end else begin
			wr_rd_ack_d[2:0]	<= {wr_rd_ack_d[1:0], rd_wr_ena_ack};
			if(wr_ena & FIFO_WR_LAST) begin
				wr_ena_req_pre	<= 1'b1;
			end else if(~wr_ena_req & (wr_rd_ack_d[2:1] == 2'b00)) begin
				wr_ena_req_pre	<= 1'b0;
			end
			if(~wr_ena_req & wr_ena_req_pre & (wr_rd_ack_d[2:1] == 2'b00)) begin
				if(wr_ena) begin
					wr_count_req_pre		<= 1;
				end else begin
					wr_count_req_pre		<= 0;
				end
			end else if(wr_ena) begin
				wr_count_req_pre		<= wr_count_req_pre + 1;
			end
			if(~wr_ena_req & wr_ena_req_pre & (wr_rd_ack_d[2:1] == 2'b00)) begin
				wr_ena_req	<= 1'b1;
				wr_count_req	<= wr_count_req_pre;
			end else if(wr_rd_ack_d[2:1] == 2'b01) begin
				wr_ena_req	<= 1'b0;
			end
		end
	end

	/////////////////////////////////////////////////////////////////////
	// Read Block
	assign rd_empty	= (rd_count == 0)?1'b1:1'b0;
	assign rsv_ena	= rsv_empty & ~rd_empty;
	assign rd_ena	= rsv_ena | (FIFO_RD_ENA & ~rd_empty);

	// Read Address
	always @(posedge FIFO_RD_CLK or negedge RST_N) begin
		if(!RST_N) begin
			rd_adrs		<= 0;
		end else begin
			if(rd_ena) begin
				rd_adrs	<= rd_adrs + 1;
			end
		end
	end

	// make a empty and almost empty signal
	always @(posedge FIFO_RD_CLK or negedge RST_N) begin
		if(!RST_N) begin
			rd_count		<= 0;
			rd_alm_count	<= 0;
		end else begin
			if(rd_ena) begin
				if(rd_wr_ena) begin
					rd_count	<= rd_count + {1'b0, rd_wr_count} - 1;
				end else begin
					rd_count	<= rd_count - 1;
				end
			end else if(rd_wr_ena) begin
				rd_count	<= rd_count + {1'b0, rd_wr_count};
			end
			rd_alm_count	<= rd_count - {1'b0, FIFO_RD_ALM_COUNT};
		end
	end

	// Write Control signal from Write Block
	always @(posedge FIFO_RD_CLK or negedge RST_N) begin
		if(!RST_N) begin
			rd_wr_ena_d[2:0]		<= 3'd0;
			rd_wr_count		<= {FIFO_DEPTH{1'b0}};
			rd_wr_full		<= 1'b0;
			rd_wr_ena		<= 1'b0;
		end else begin
			rd_wr_ena_d[2:0]	<= {rd_wr_ena_d[1:0], wr_ena_req};
			if(rd_wr_ena_d[2:1] == 2'b01) begin
				rd_wr_ena	<= 1'b1;
				rd_wr_count	<= wr_count_req;
				rd_wr_full	<= wr_count[FIFO_DEPTH];
			end else begin
				rd_wr_ena	<= 1'b0;
			end
		end
	end

	// Write enable signal from write block
	assign rd_wr_ena_ack	= rd_wr_ena_d[2] & rd_wr_ena_d[1];

	// Send a read enable signal for Write Block
	reg [2:0] rd_wr_ack_d;
	always @(posedge FIFO_RD_CLK or negedge RST_N) begin
		if(!RST_N) begin
			rd_ena_req_pre	<= 1'b0;
			rd_ena_req		<= 1'b0;
			rd_count_req	<= {FIFO_DEPTH{1'b0}};
			rd_count_req_pre	<= {FIFO_DEPTH{1'b0}};
			rd_wr_ack_d[2:0]	<= 3'd0;
		end else begin
			rd_wr_ack_d[2:0]	<= {rd_wr_ack_d[1:0], wr_rd_ena_ack};
			if(rd_ena) begin
				rd_ena_req_pre	<= 1'b1;
			end else if(~rd_ena_req & (rd_wr_ack_d[2:1] == 2'd00)) begin
				rd_ena_req_pre	<= 1'b0;
			end
			if(~rd_ena_req & rd_ena_req_pre & (rd_wr_ack_d[2:1] == 2'd00)) begin
				if(rd_ena) begin
					rd_count_req_pre	<= 1;
				end else begin
					rd_count_req_pre	<= 0;
				end
			end else if(rd_ena) begin
				rd_count_req_pre	<= rd_count_req_pre + 1;
			end
			if(~rd_ena_req & rd_ena_req_pre & (rd_wr_ack_d[2:1] == 2'd00)) begin
				rd_ena_req		<= 1'b1;
				rd_count_req	<= rd_count_req_pre;
			end else if(rd_wr_ack_d[2:1] == 2'b01) begin
				rd_ena_req		<= 1'b0;
			end
		end
	end

	/////////////////////////////////////////////////////////////////////
	// rsv Block
	always @(posedge FIFO_RD_CLK or negedge RST_N) begin
		if(!RST_N) begin
			rsv_data			<= {FIFO_WIDTH{1'b0}};
			rsv_empty			<= 1'b1;
		end else begin
			rd_ena_d			<= FIFO_RD_ENA;
			if(rd_ena | rd_ena_d) begin
				rsv_data		<= rd_fifo;
			end

			if(FIFO_RD_ENA & rd_empty) begin
				rsv_empty	<= 1'b1;
			end else if(rd_ena) begin
				rsv_empty	<= 1'b0;
			end
		end
	end

	assign rsv_alm_empty = (rd_empty & ~rsv_empty);
	/////////////////////////////////////////////////////////////////////
	// output signals
	assign FIFO_WR_FULL			= wr_count[FIFO_DEPTH];
	assign FIFO_WR_ALM_FULL		= wr_alm_count[FIFO_DEPTH];
	assign FIFO_RD_EMPTY		= rsv_empty;
	assign FIFO_RD_ALM_EMPTY	= rd_alm_count[FIFO_DEPTH];
	assign FIFO_RD_DATA			= (rd_ena_d)?rd_fifo:rsv_data;

	/////////////////////////////////////////////////////////////////////
	// RAM
	fifo_ram #(FIFO_DEPTH,FIFO_WIDTH) u_fifo_ram(
		.WR_CLK  ( FIFO_WR_CLK  ),
		.WR_ENA  ( wr_ena  ),
		.WR_ADRS ( wr_adrs ),
		.WR_DATA ( FIFO_WR_DATA ),

		.RD_CLK  ( FIFO_RD_CLK  ),
		.RD_ADRS ( rd_adrs ),
		.RD_DATA ( rd_fifo )
		);

endmodule

module fifo_ram
#(
	parameter DEPTH	= 12,
	parameter WIDTH	= 32
)
(
	input					WR_CLK,
	input					WR_ENA,
	input [DEPTH -1:0] 	WR_ADRS,
	input [WIDTH -1:0]	WR_DATA,

	input 					RD_CLK,
	input [DEPTH -1:0] 	RD_ADRS,
	output [WIDTH -1:0]	RD_DATA
);
	reg [WIDTH -1:0]		ram [0:(2**DEPTH) -1];
	reg [WIDTH -1:0]		rd_reg;

	always @(posedge WR_CLK) begin
		if(WR_ENA) ram[WR_ADRS] <= WR_DATA;
	end

	always @(posedge RD_CLK) begin
		rd_reg <= ram[RD_ADRS];
	end

	assign RD_DATA = rd_reg;
endmodule
