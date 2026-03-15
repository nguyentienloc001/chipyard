package myrom

import chisel3._
import org.chipsalliance.cde.config.Parameters

class RoCCControllerIO (implicit p: Parameters) extends Bundle {
  // Request
  val rocc_req_addr = Input(UInt(3.W))

}