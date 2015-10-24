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
module aq_axi_master32_single(
  // Reset, Clock
  input           ARESETN,
  input           ACLK,

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
  output [31:0] M_AXI_WDATA,
  output [3:0]  M_AXI_WSTRB,
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
  input [31:0]  M_AXI_RDATA,
  input [1:0]   M_AXI_RRESP,
  input         M_AXI_RLAST,
  input [0:0]   M_AXI_RUSER,
  input         M_AXI_RVALID,
  output        M_AXI_RREADY,

  // Local Bus
  input         MASTER_RST,

  input         WR_START,
  input [31:0]  WR_ADRS,
  input [31:0]  WR_LEN,
  output        WR_READY,
  output        WR_DONE,
  input [31:0]  WR_DATA,

  input         RD_START,
  input [31:0]  RD_ADRS,
  input [31:0]  RD_LEN,
  output        RD_READY,
  output        RD_DONE,
  output [31:0] RD_DATA
);

  localparam S_WR_IDLE  = 3'd0;
  localparam S_WA_WAIT  = 3'd1;
  localparam S_WA_START = 3'd2;
  localparam S_WD_WAIT  = 3'd3;
  localparam S_WD_PROC  = 3'd4;
  localparam S_WR_WAIT  = 3'd5;

  reg [2:0]   wr_state;
  reg [31:0]  reg_wr_adrs;
  reg [31:0]  reg_wr_len;
  reg         reg_awvalid, reg_wvalid, reg_w_last;
  reg [7:0]   reg_w_len;
  reg [7:0]   reg_w_stb;
  reg [1:0]   reg_wr_status;
  reg [3:0]   reg_w_count, reg_r_count;
  reg [31:0]    reg_wr_data;

  reg [7:0]   rd_chkdata, wr_chkdata;
  reg [1:0]   resp;

  // Write State
  always @(posedge ACLK or negedge ARESETN) begin
    if(!ARESETN) begin
      wr_state            <= S_WR_IDLE;
      reg_wr_adrs[31:0]   <= 32'd0;
      reg_wr_len[31:0]    <= 32'd0;
      reg_awvalid         <= 1'b0;
      reg_wvalid          <= 1'b0;
      reg_w_last          <= 1'b0;
      reg_w_len[7:0]      <= 8'd0;
      reg_w_stb[7:0]      <= 8'd0;
      reg_wr_status[1:0]  <= 2'd0;
      reg_w_count[3:0]    <= 4'd0;
      reg_r_count[3:0]  <= 4'd0;
      wr_chkdata          <= 8'd0;
        rd_chkdata <= 8'd0;
      resp <= 2'd0;
  end else begin
    if(MASTER_RST) begin
      wr_state <= S_WR_IDLE;
    end else begin
      case(wr_state)
        S_WR_IDLE: begin
          if(WR_START) begin
            wr_state          <= S_WA_WAIT;
            reg_wr_adrs[31:0] <= WR_ADRS[31:0];
            reg_wr_len[31:0]  <= WR_LEN[31:0] -32'd1;
            reg_wr_data[31:0]	<= WR_DATA[31:0];
          end
          reg_awvalid         <= 1'b0;
          reg_wvalid          <= 1'b0;
          reg_w_last          <= 1'b0;
          reg_w_len[7:0]      <= 8'd0;
          reg_w_stb[7:0]      <= 8'd0;
          reg_wr_status[1:0]  <= 2'd0;
        end
        S_WA_WAIT: begin
          if(reg_wr_len[31:11] == 21'd0) begin
            wr_state          <= S_WA_START;
          end
        end
        S_WA_START: begin
          wr_state            <= S_WD_WAIT;
          reg_awvalid         <= 1'b1;
          reg_wr_len[31:11]    <= reg_wr_len[31:11] - 21'd1;
          if(reg_wr_len[31:11] != 21'd0) begin
            reg_w_len[7:0]  <= 8'hFF;
            reg_w_last      <= 1'b0;
            reg_w_stb[3:0]  <= 4'hF;
          end else begin
            reg_w_len[7:0]  <= reg_wr_len[10:3];
            reg_w_last      <= 1'b1;
            case(reg_wr_len[2:0])
              3'd0: reg_w_stb[3:0]  <= 4'b0000;
              3'd1: reg_w_stb[3:0]  <= 4'b0001;
              3'd2: reg_w_stb[3:0]  <= 4'b0011;
              3'd3: reg_w_stb[3:0]  <= 4'b0111;
              3'd4: reg_w_stb[3:0]  <= 4'b1111;
              3'd5: reg_w_stb[3:0]  <= 4'b1111;
              3'd6: reg_w_stb[3:0]  <= 4'b1111;
              3'd7: reg_w_stb[3:0]  <= 4'b1111;
              default:   reg_w_stb[3:0]  <= 4'b1111;
            endcase
          end
        end
        S_WD_WAIT: begin
          if(M_AXI_AWREADY) begin
            wr_state        <= S_WD_PROC;
            reg_awvalid     <= 1'b0;
            reg_wvalid      <= 1'b1;
          end
        end
        S_WD_PROC: begin
          if(M_AXI_WREADY) begin
            if(reg_w_len[7:0] == 8'd0) begin
              wr_state        <= S_WR_WAIT;
              reg_wvalid      <= 1'b0;
              reg_w_stb[3:0]  <= 4'h0;
            end else begin
              reg_w_len[7:0]  <= reg_w_len[7:0] -8'd1;
            end
          end
        end
        S_WR_WAIT: begin
          if(M_AXI_BVALID) begin
            reg_wr_status[1:0]  <= reg_wr_status[1:0] | M_AXI_BRESP[1:0];
            if(reg_w_last) begin
              wr_state          <= S_WR_IDLE;
            end else begin
              wr_state          <= S_WA_WAIT;
              reg_wr_adrs[31:0] <= reg_wr_adrs[31:0] + 32'd2048;
            end
          end
        end
        default: begin
          wr_state <= S_WR_IDLE;
        end
      endcase
      end
    end
  end

  assign M_AXI_AWID         = 1'b0;
  assign M_AXI_AWADDR[31:0] = reg_wr_adrs[31:0];
  assign M_AXI_AWLEN[7:0]   = reg_w_len[7:0];// Burst Length: 0-255
  assign M_AXI_AWSIZE[2:0]  = 2'b011;// Burst Size: Fixed 2'b011
  assign M_AXI_AWBURST[1:0] = 2'b01;// Burst Type: Fixed 2'b01(Incremental Burst)
  assign M_AXI_AWLOCK       = 1'b0;// Lock: Fixed 2'b00
  assign M_AXI_AWCACHE[3:0] = 4'b0011;// Cache: Fiex 2'b0011
  assign M_AXI_AWPROT[2:0]  = 3'b000;// Protect: Fixed 2'b000
  assign M_AXI_AWQOS[3:0]   = 4'b0000;// QoS: Fixed 2'b0000
  assign M_AXI_AWUSER[0]    = 1'b1;// User: Fixed 32'd0
  assign M_AXI_AWVALID      = reg_awvalid;

assign M_AXI_WDATA[ 7: 0] =	reg_wr_data[7:0];
assign M_AXI_WDATA[15: 8] =	((reg_wr_adrs[1:0] == 2'd0)?reg_wr_data[15: 8]:8'd0) |
					((reg_wr_adrs[1:0] == 2'd1)?reg_wr_data[ 7: 0]:8'd0);
assign M_AXI_WDATA[23:16] =	((reg_wr_adrs[1:0] == 2'd0)?reg_wr_data[23:16]:8'd0) |
					((reg_wr_adrs[1:0] == 2'd1)?reg_wr_data[15: 8]:8'd0) |
					((reg_wr_adrs[1:0] == 2'd2)?reg_wr_data[ 7: 0]:8'd0);
assign M_AXI_WDATA[31:24] =	((reg_wr_adrs[1:0] == 2'd0)?reg_wr_data[31:24]:8'd0) |
					((reg_wr_adrs[1:0] == 2'd1)?reg_wr_data[23:16]:8'd0) |
					((reg_wr_adrs[1:0] == 2'd2)?reg_wr_data[15: 8]:8'd0) |
					((reg_wr_adrs[1:0] == 2'd3)?reg_wr_data[ 7: 0]:8'd0);

//  assign M_AXI_WSTRB[7:0]   = (reg_w_len[7:0] == 8'd0)?reg_w_stb[7:0]:8'hFF;
//  assign M_AXI_WSTRB[7:0]   = (wr_state == S_WD_PROC)?8'hFF:8'h00;
  assign M_AXI_WSTRB[3:0]   = (reg_wvalid)?reg_w_stb:4'h0;
  assign M_AXI_WLAST        = (reg_w_len[7:0] == 8'd0)?1'b1:1'b0;
  assign M_AXI_WUSER        = 1;
  assign M_AXI_WVALID       = reg_wvalid;

  assign M_AXI_BREADY       = M_AXI_BVALID;

  assign WR_READY           = (wr_state == S_WR_IDLE)?1'b1:1'b0;
  assign WR_DONE            = ((wr_state == S_WR_WAIT) && M_AXI_BVALID)?1'b1:1'b0;

  localparam S_RD_IDLE  = 3'd0;
  localparam S_RA_WAIT  = 3'd1;
  localparam S_RA_START = 3'd2;
  localparam S_RD_WAIT  = 3'd3;
  localparam S_RD_PROC  = 3'd4;

  reg [2:0]   rd_state;
  reg [31:0]  reg_rd_adrs;
  reg [31:0]  reg_rd_len;
  reg         reg_arvalid, reg_r_last;
  reg [7:0]   reg_r_len;

  // Read State
  always @(posedge ACLK or negedge ARESETN) begin
    if(!ARESETN) begin
      rd_state          <= S_RD_IDLE;
      reg_rd_adrs[31:0] <= 32'd0;
      reg_rd_len[31:0]  <= 32'd0;
      reg_arvalid       <= 1'b0;
      reg_r_len[7:0]    <= 8'd0;
    end else begin
      case(rd_state)
        S_RD_IDLE: begin
          if(RD_START) begin
            rd_state          <= S_RA_WAIT;
            reg_rd_adrs[31:0] <= RD_ADRS[31:0];
            reg_rd_len[31:0]  <= RD_LEN[31:0] -32'd1;
          end
          reg_arvalid     <= 1'b0;
          reg_r_len[7:0]  <= 8'd0;
        end
        S_RA_WAIT: begin
            rd_state          <= S_RA_START;
        end
        S_RA_START: begin
          rd_state          <= S_RD_WAIT;
          reg_arvalid       <= 1'b1;
          reg_rd_len[31:11] <= reg_rd_len[31:11] -21'd1;
          if(reg_rd_len[31:11] != 21'd0) begin
            reg_r_last      <= 1'b0;
            reg_r_len[7:0]  <= 8'd255;
          end else begin
            reg_r_last      <= 1'b1;
            reg_r_len[7:0]  <= reg_rd_len[10:3];
          end
        end
        S_RD_WAIT: begin
          if(M_AXI_ARREADY) begin
            rd_state        <= S_RD_PROC;
            reg_arvalid     <= 1'b0;
          end
        end
        S_RD_PROC: begin
          if(M_AXI_RVALID) begin
            if(M_AXI_RLAST) begin
              if(reg_r_last) begin
                rd_state          <= S_RD_IDLE;
              end else begin
                rd_state          <= S_RA_WAIT;
                reg_rd_adrs[31:0] <= reg_rd_adrs[31:0] + 32'd2048;
              end
            end else begin
              reg_r_len[7:0] <= reg_r_len[7:0] -8'd1;
            end
          end
        end
      endcase
    end
  end

  // Master Read Address
  assign M_AXI_ARID         = 1'b0;
  assign M_AXI_ARADDR[31:0] = reg_rd_adrs[31:0];
  assign M_AXI_ARLEN[7:0]   = reg_r_len[7:0];
  assign M_AXI_ARSIZE[2:0]  = 3'b011;
  assign M_AXI_ARBURST[1:0] = 2'b01;
  assign M_AXI_ARLOCK       = 1'b0;
  assign M_AXI_ARCACHE[3:0] = 4'b0011;
  assign M_AXI_ARPROT[2:0]  = 3'b000;
  assign M_AXI_ARQOS[3:0]   = 4'b0000;
  assign M_AXI_ARUSER[0]    = 1'b1;
  assign M_AXI_ARVALID      = reg_arvalid;

  assign M_AXI_RREADY       = M_AXI_RVALID;

  assign RD_READY           = (rd_state == S_RD_IDLE)?1'b1:1'b0;
  assign RD_DATA[31:0]		= M_AXI_RDATA[31:0];
  assign RD_DONE            = ((wr_state == S_RD_PROC) && M_AXI_RVALID)?1'b1:1'b0;

endmodule

