package myrom

import chisel3._
import org.chipsalliance.cde.config.Parameters
import freechips.rocketchip.tile.{RoCCIO}

class RoCCDecouplerIO (implicit p: Parameters) extends Bundle {
  // Control signals
  val clock = Input(Clock())
  val reset = Input(UInt(1.W))

  // RoCC Interface
  val rocc_io = new RoCCIO(0, 0)

  // Decouple-Controller interface


}