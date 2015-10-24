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
module aq_axi_memcpy32
  (
    input           ARESETN,

    // --------------------------------------------------
    // AXI4 Master
    // --------------------------------------------------
    // Reset, Clock
    input         M_AXI_ACLK,

    // Master Write Address
    output [0:0]  M_AXI_AWID,
    output [31:0] M_AXI_AWADDR,
    output [7:0]  M_AXI_AWLEN,
    output [2:0]  M_AXI_AWSIZE,
    output [1:0]  M_AXI_AWBURST,
    output        M_AXI_AWLOCK,
    output [3:0]  M_AXI_AWCACHE,
    output [2:0]  M_AXI_AWPROT,
    output [3:0]  M_AXI_AWQOS,
    output [0:0]  M_AXI_AWUSER,
    output        M_AXI_AWVALID,
    input         M_AXI_AWREADY,

    // Master Write Data
    output [63:0] M_AXI_WDATA,
    output [7:0]  M_AXI_WSTRB,
    output        M_AXI_WLAST,
    output [0:0]  M_AXI_WUSER,
    output        M_AXI_WVALID,
    input         M_AXI_WREADY,

    // Master Write Response
    input [0:0]   M_AXI_BID,
    input [1:0]   M_AXI_BRESP,
    input [0:0]   M_AXI_BUSER,
    input         M_AXI_BVALID,
    output        M_AXI_BREADY,

    // Master Read Address
    output [0:0]  M_AXI_ARID,
    output [31:0] M_AXI_ARADDR,
    output [7:0]  M_AXI_ARLEN,
    output [2:0]  M_AXI_ARSIZE,
    output [1:0]  M_AXI_ARBURST,
    output [1:0]  M_AXI_ARLOCK,
    output [3:0]  M_AXI_ARCACHE,
    output [2:0]  M_AXI_ARPROT,
    output [3:0]  M_AXI_ARQOS,
    output [0:0]  M_AXI_ARUSER,
    output        M_AXI_ARVALID,
    input         M_AXI_ARREADY,

    // Master Read Data
    input [0:0]   M_AXI_RID,
    input [63:0]  M_AXI_RDATA,
    input [1:0]   M_AXI_RRESP,
    input         M_AXI_RLAST,
    input [0:0]   M_AXI_RUSER,
    input         M_AXI_RVALID,
    output        M_AXI_RREADY,

    // Command I/F
	input			CMD_REQ,
	output			CMD_READY,
	output			CMD_DONE,
	input [31:0]	CMD_DST,
	input [31:0]	CMD_SRC,
	input [31:0]	CMD_LEN
);

  wire           local_cs;
  wire           local_rnw;
  wire           local_ack;
  wire [31:0]    local_addr;
  wire [3:0]     local_be;
  wire [31:0]    local_wdata;
  wire [31:0]    local_rdata;

  wire           wr_start;

  wire [31:0]    wr_adrs;
  wire [31:0]    wr_len;
  wire           wr_ready;
  wire           wr_fifo_re;
  wire           wr_fifo_empty;
  wire           wr_fifo_aempty;
  wire [63:0]    wr_fifo_data;

  wire           rd_start;
  wire [31:0]    rd_adrs;
  wire [31:0]    rd_len;
  wire           rd_ready;
  wire           rd_fifo_we;
  wire           rd_fifo_full;
  wire           rd_fifo_afull;
  wire [63:0]    rd_fifo_data;

  wire           rd_start_fsync;

  wire [31:0]    master_status;

  reg [31:0]      wr_fifo_wrcnt, wr_fifo_rdcnt, rd_fifo_wrcnt, rd_fifo_rdcnt;

  wire [31:0]    debug_slave, debug_ctl, debug_master;

  wire fifo_rst_w;

  aq_axi_master32 u_aq_axi_master32
    (
      .ARESETN(ARESETN),
      .ACLK(M_AXI_ACLK),

      .M_AXI_AWID(M_AXI_AWID),
      .M_AXI_AWADDR(M_AXI_AWADDR),
      .M_AXI_AWLEN(M_AXI_AWLEN),
      .M_AXI_AWSIZE(M_AXI_AWSIZE),
      .M_AXI_AWBURST(M_AXI_AWBURST),
      .M_AXI_AWLOCK(M_AXI_AWLOCK),
      .M_AXI_AWCACHE(M_AXI_AWCACHE),
      .M_AXI_AWPROT(M_AXI_AWPROT),
      .M_AXI_AWQOS(M_AXI_AWQOS),
      .M_AXI_AWUSER(M_AXI_AWUSER),
      .M_AXI_AWVALID(M_AXI_AWVALID),
      .M_AXI_AWREADY(M_AXI_AWREADY),

      .M_AXI_WDATA(M_AXI_WDATA),
      .M_AXI_WSTRB(M_AXI_WSTRB),
      .M_AXI_WLAST(M_AXI_WLAST),
      .M_AXI_WUSER(M_AXI_WUSER),
      .M_AXI_WVALID(M_AXI_WVALID),
      .M_AXI_WREADY(M_AXI_WREADY),

      .M_AXI_BID(M_AXI_BID),
      .M_AXI_BRESP(M_AXI_BRESP),
      .M_AXI_BUSER(M_AXI_BUSER),
      .M_AXI_BVALID(M_AXI_BVALID),
      .M_AXI_BREADY(M_AXI_BREADY),

      .M_AXI_ARID(M_AXI_ARID),
      .M_AXI_ARADDR(M_AXI_ARADDR),
      .M_AXI_ARLEN(M_AXI_ARLEN),
      .M_AXI_ARSIZE(M_AXI_ARSIZE),
      .M_AXI_ARBURST(M_AXI_ARBURST),
      .M_AXI_ARLOCK(M_AXI_ARLOCK),
      .M_AXI_ARCACHE(M_AXI_ARCACHE),
      .M_AXI_ARPROT(M_AXI_ARPROT),
      .M_AXI_ARQOS(M_AXI_ARQOS),
      .M_AXI_ARUSER(M_AXI_ARUSER),
      .M_AXI_ARVALID(M_AXI_ARVALID),
      .M_AXI_ARREADY(M_AXI_ARREADY),

      .M_AXI_RID(M_AXI_RID),
      .M_AXI_RDATA(M_AXI_RDATA),
      .M_AXI_RRESP(M_AXI_RRESP),
      .M_AXI_RLAST(M_AXI_RLAST),
      .M_AXI_RUSER(M_AXI_RUSER),
      .M_AXI_RVALID(M_AXI_RVALID),
      .M_AXI_RREADY(M_AXI_RREADY),

      .MASTER_RST(FIFO_RST),

      .WR_START(wr_start_fsync),
      .WR_ADRS(wr_adrs),
      .WR_LEN(wr_len),
      .WR_READY(wr_ready),
      .WR_FIFO_RE(wr_fifo_re),
      .WR_FIFO_EMPTY(wr_fifo_empty),
      .WR_FIFO_AEMPTY(wr_fifo_aempty),
      .WR_FIFO_DATA(wr_fifo_data),

      .RD_START(rd_start_fsync),
      .RD_ADRS(rd_adrs),
      .RD_LEN(rd_len),
      .RD_READY(rd_ready),
      .RD_FIFO_WE(rd_fifo_we),
      .RD_FIFO_FULL(rd_fifo_full),
      .RD_FIFO_AFULL(rd_fifo_afull),
      .RD_FIFO_DATA(rd_fifo_data)
    );

aq_fifo
#(
	.FIFO_DEPTH(12),
	.FIFO_WIDTH(32)
)
u_aq_fifo(
	.RST_N				( ~fifo_rst_w		),

	.FIFO_WR_CLK		(M_AXI_ACLK),
	.FIFO_WR_ENA		(rd_fifo_we),
	.FIFO_WR_DATA		(rd_fifo_data),
	.FIFO_WR_LAST		(1'b1),
	.FIFO_WR_FULL		(rd_fifo_full),
	.FIFO_WR_ALM_FULL	(rd_fifo_afull),
	.FIFO_WR_ALM_COUNT	(12'h102),

	.FIFO_RD_CLK		(M_AXI_ACLK),
	.FIFO_RD_ENA		(wr_fifo_re),
	.FIFO_RD_DATA		(wr_fifo_data),
	.FIFO_RD_EMPTY		(wr_fifo_empty),
	.FIFO_RD_ALM_EMPTY	(wr_fifo_aempty),
	.FIFO_RD_ALM_COUNT	(12'h102)
);


  aq_axi_memcpy32_ctl u_aq_axi_memcpy32_ctl
    (
      .RST_N(ARESETN),
      .CLK(S_AXI_ACLK),

      .CMD_REQ(CMD_REQ),
      .CMD_READY(CMD_READY),
      .CMD_DONE(CMD_DONE),
      .CMD_DST(CMD_DST),
      .CMD_SRC(CMD_SRC),
      .CMD_LEN(CMD_LEN),

      .CMD_CLK(M_AXI_ACLK),

      .WR_START(wr_start),
      .WR_ADRS(wr_adrs),
      .WR_COUNT(wr_len),
      .WR_READY(wr_ready),

      .RD_START(rd_start),
      .RD_ADRS(rd_adrs),
      .RD_COUNT(rd_len),
      .RD_READY(rd_ready),

      .FIFO_RST(fifo_rst_w)
    );

endmodule

